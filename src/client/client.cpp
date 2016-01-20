#include "stdafx.h"
#include "../common/boostssl.hpp"
#include <WTypes.h>
#include "../common/interface.h"
#include "../common/Utils.h"
#include <boost/shared_ptr.hpp>
#include "client.h"


INetwork *CreateClientInstance()
{
	return new CBoostNetwork;
}

void Dump(LPCTSTR fmt, ...)
{
// 	CTime t = CTime::GetCurrentTime();
// 	CString str = t.Format(_T("%H:%M:%S "));
// 	va_list ap;
// 	va_start(ap, fmt);
// 	str.AppendFormatV(fmt, ap);
// 	OutputDebugString(str);
}

CBoostNetwork::CBoostNetwork() : io_thread_(NULL)
, connected_(false)
, msg_read_(NULL)
{

}

CBoostNetwork::~CBoostNetwork()
{
	if (msg_read_)
	{
		free(msg_read_);
		msg_read_ = NULL;
	}
}

BOOL CBoostNetwork::Initialize(LPCTSTR *szError, INetworkSink *pSink)
{
	sink_ = pSink;
	return TRUE;
}

BOOL CBoostNetwork::LoadCert(LPCTSTR path)
{
	// 		try
	// 		{
	// 			context_.set_password_callback(boost::bind(&CBoostNetwork::get_password, this));
	// 			context_.load_verify_file("servercert.pem");
	// 			context_.use_certificate_file("clientcert.pem", boost::asio::ssl::context_base::pem);
	// 			context_.use_private_key_file("clientkey.pem", boost::asio::ssl::context_base::pem);
	// 		}
	// 		catch (boost::system::error_code & ec)
	// 		{
	// 			printf("err> %s\n", ec.message().c_str());
	// 			return FALSE;
	// 		}
	// 		return TRUE;
	if (!path)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	io_.reset(new client_io_service());
	return load_pfx(io_->context_, path, "test");
}

BOOL CBoostNetwork::Connect(LPCSTR addr, int port)
{
	boost::system::error_code error = boost::asio::error::host_not_found;
	char sport[6];
	if (port <= 0 || port >= 65535)
		return FALSE;

	if (!io_.get())
		return FALSE;

	io_->renew_socket();
	io_->socket_->set_verify_callback(
		boost::bind(&CBoostNetwork::verify_certificate, this, _1, _2));

	_itoa_s(port, sport, 10);

	boost::asio::ip::tcp::resolver::query query(addr, sport);
	boost::asio::ip::tcp::resolver::iterator iterator =
		io_->resolver_.resolve(query);

	io_->socket_->lowest_layer().connect(*iterator, error);

	if (error)
		return FALSE;

	io_->socket_->handshake(boost::asio::ssl::stream_base::client, error);

	if (error)
		return FALSE;

	io_thread_ = CreateThread(NULL, 0,
		(LPTHREAD_START_ROUTINE)&CBoostNetwork::_ThreadProc,
		this, 0, NULL);

	if (!io_thread_)
	{
		sink_->OnException(_T("启动网络接收失败"));
		return FALSE;
	}

	DWORD dwExit = 0;
	if (WaitForSingleObject(io_thread_, 100) != WAIT_TIMEOUT
		&& GetExitCodeThread(io_thread_, &dwExit)
		&& dwExit != STILL_ACTIVE)
	{
		sink_->OnException(_T("网络启动后立即停止了"));
		CloseHandle(io_thread_);
		io_thread_ = NULL;
		io_.reset();
		return FALSE;
	}

	if (!assure_buffer_size(msg_read_, sizeof(size_t), _T("准备读取消息时")))
		return FALSE;

	connected_ = TRUE;
	Receive();
	return TRUE;
}

void CBoostNetwork::Destroy()
{
	delete this;
}

BOOL CBoostNetwork::IsConnected()
{
	if (!io_.get()
		|| !io_->socket_.get()
		|| !io_->socket_->lowest_layer().is_open())
		return FALSE;

	return TRUE;
}

void CBoostNetwork::Close()
{
	if (io_.get())
	{
		if (io_->socket_.get() && io_->socket_->lowest_layer().is_open())
			io_->socket_->lowest_layer().cancel();
		if (io_.get())
			io_->stop_io();
		io_.reset();
	}
	if (io_thread_)
	{
		if (WaitForSingleObject(io_thread_, 1000) == WAIT_TIMEOUT)
			TerminateThread(io_thread_, 0xdead);
		CloseHandle(io_thread_);
		io_thread_ = NULL;
	}
}

BOOL CBoostNetwork::Send(LPCSTR szCmd, LPCSTR szContent)
{
	message *msg = NULL;

	if (!io_.get()
		|| !io_->socket_.get()
		|| !io_->socket_->lowest_layer().is_open())
		return FALSE;

	if (!message::format(msg, szCmd, "%s", szContent))
		return FALSE;

	printf("将发送%u\n", msg->length);
	boost::asio::async_write(*io_->socket_, msg_buffer(msg),
		boost::asio::transfer_at_least(msg->length),
		boost::bind(&CBoostNetwork::handle_write, this,
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred, msg, 0));
	return TRUE;
}

BOOL CBoostNetwork::SendEx(void *ctx, LPCSTR szCmd, LPCSTR szContent)
{
	message *msg = NULL;

	if (!io_.get()
		|| !io_->socket_.get()
		|| !io_->socket_->lowest_layer().is_open())
		return FALSE;

	if (!message::format(msg, szCmd, "%s", szContent))
		return FALSE;
	printf("将发送%u\n", msg->length);
	boost::asio::async_write(*io_->socket_, msg_buffer(msg),
		boost::asio::transfer_at_least(msg->length),
		boost::bind(&CBoostNetwork::handle_write_ex, this,
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred, msg, 0, ctx));
	return TRUE;
}

bool CBoostNetwork::assure_buffer_size(message *&msg, size_t size, LPCTSTR when)
{
	message *msgold = msg;
	if (!message::reserve(msg, size))
	{
		TCHAR err_[128];
		_stprintf_s(err_, _T("%s分配[%u]内存失败"), when, size);
		sink_->OnException(err_);
		io_.reset();
		return false;
	}
	return true;
}

void CBoostNetwork::handle_write(const boost::system::error_code &ec, size_t bytes_transferred, message *msg, size_t offset)
{
	if (ec || bytes_transferred == 0)
	{
		if (msg)
			free(msg);
		io_.reset();
		return;
	}
	if (bytes_transferred < msg->length)
	{
		offset += bytes_transferred;
		boost::asio::transfer_at_least(msg->length);
		io_->socket_->async_write_some(msg_buffer_offset(msg, offset),
			boost::bind(&CBoostNetwork::handle_write, this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred, msg, offset));
	}
	else if (bytes_transferred == msg->length)
	{
		printf("message sending complete %u\n", msg->length);
		free(msg);
	}
}

void CBoostNetwork::handle_write_ex(const boost::system::error_code &ec, size_t bytes_transferred, message *msg, size_t offset, void *ctx)
{
	if (ec || bytes_transferred == 0)
	{
		if (msg)
			free(msg);
		io_.reset();
		return;
	}
	if (bytes_transferred < msg->length)
	{
		offset += bytes_transferred;
		boost::asio::transfer_at_least(msg->length);
		io_->socket_->async_write_some(msg_buffer_offset(msg, offset),
			boost::bind(&CBoostNetwork::handle_write_ex, this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred,
			msg, offset, ctx));
	}
	else if (bytes_transferred == msg->length)
	{
		sink_->OnSendComplete(ctx);
		free(msg);
	}
}


void CBoostNetwork::handle_read(const boost::system::error_code& error, size_t bytes_transferred, size_t offset)
{
	if (!error && bytes_transferred)
	{
		offset += bytes_transferred;
		if (offset < sizeof(size_t))
		{
			io_->socket_->async_read_some(
				msg_buffer_offset(msg_read_, offset),
				boost::bind(&CBoostNetwork::handle_read, this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred,
				offset));
			return;
		}

		if (offset - bytes_transferred < sizeof(size_t))
		{
			// 第一次超过size_t的时候申请内存
			if (!assure_buffer_size(msg_read_,
				msg_read_->length + 1, _T("读取数据时")))
				return;
		}

		if (offset < msg_read_->length)
		{
			io_->socket_->async_read_some(
				msg_buffer_offset(msg_read_, offset),
				boost::bind(&CBoostNetwork::handle_read, this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred,
				offset));
			return;
		}

		size_t nlen = strnlen(msg_read_->buffer,
			message::max_cmd_length + 1);

		if (nlen > message::max_cmd_length)
		{
			TCHAR szMsg[64];
			_stprintf_s(szMsg, _T("命令超长：%u/%u"),
				nlen, message::max_cmd_length);
			sink_->OnException(szMsg);

			Receive();
			return;
		}

		msg_read_->msg[msg_read_->length] = 0;

		// user should call Answer in OnCommand
		sink_->OnReceive(msg_read_->buffer,
			msg_read_->buffer + nlen + 1);

		Receive();
	}
	else
	{
		switch (error.value())
		{
		case boost::asio::error::eof:
			break;
		default:
			{
				tempA2T err(error.message().c_str(), CP_ACP);
				int len = _sctprintf(_T("读取网络数据异常：%s"), err);
				LPTSTR p = new TCHAR[len+1];
				if (p)
				{
					_stprintf_s(p, len+1, _T("读取网络数据异常：%s"), err);
					sink_->OnException(p);
					//Dump(_T("[%p]%s\n"), io_.get(), p);
					delete [] p;
				}
				if (connected_)
				{
					sink_->OnDisconnect();
					connected_ = false;
				}
				if (io_.get())
				{
					if (io_->socket_.get() && io_->socket_->lowest_layer().is_open())
						io_->socket_->lowest_layer().close();
					io_->stop_io();
				}
			}
		}
	}
}

bool CBoostNetwork::verify_certificate(bool preverified, boost::asio::ssl::verify_context& ctx)
{
	// The verify callback can be used to check whether the certificate that is
	// being presented is valid for the peer. For example, RFC 2818 describes
	// the steps involved in doing this for HTTPS. Consult the OpenSSL
	// documentation for more details. Note that the callback is called once
	// for each certificate in the certificate chain, starting from the root
	// certificate authority.

	// In this example we will simply print the certificate's subject name.
	char subject_name[256];
	X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
	X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
	std::cout << "Verifying " << subject_name << "\n";
	std::cout << "Result: " << preverified << "\n";

	return preverified;
}

DWORD __stdcall CBoostNetwork::_ThreadProc(CBoostNetwork *pthis)
{
	try
	{
		boost::shared_ptr<client_io_service> io_ = pthis->io_;
		io_->io_service_.run();
	}
	catch (boost::system::error_code &ec)
	{
		//Dump(_T("IO运行异常:%hs\n"), ec.message().c_str());
	}
	catch (boost::system::system_error const& e)
	{
		//if (e.code().value() != ERROR_ABANDONED_WAIT_0)
			//Dump(_T("IO系统运行异常:%hs\n"), e.what());
	}
	//Dump(_T("IO线程退出\n"));
	return 0;
}
