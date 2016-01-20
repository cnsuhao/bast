#pragma once
using namespace client;


struct client_io_service
{
	client_io_service()
		: context_(boost::asio::ssl::context::sslv2_client)
		, resolver_(io_service_)
		, work_(new boost::asio::io_service::work(io_service_))
	{
	}

	void renew_socket()
	{
		socket_.reset(new ssl_socket(io_service_, context_));
		socket_->set_verify_mode(boost::asio::ssl::verify_peer);
	}

	void stop_io()
	{
		work_.reset();
	}

	boost::asio::ssl::context context_;
	boost::asio::io_service io_service_;
	boost::asio::ip::tcp::resolver resolver_;
	boost::shared_ptr<ssl_socket> socket_;
	boost::shared_ptr<boost::asio::io_service::work> work_;
};


class CBoostNetwork : public INetwork
{
public:
	CBoostNetwork();

	virtual ~CBoostNetwork();

	virtual BOOL Initialize(LPCTSTR *szError, INetworkSink *pSink);
	virtual BOOL LoadCert(LPCTSTR path);
	virtual BOOL Connect(LPCSTR addr, int port);
	virtual void Destroy();
	virtual BOOL IsConnected();
	virtual void Close();
	virtual BOOL Send(LPCSTR szCmd, LPCSTR szContent);
	virtual BOOL SendEx(void *ctx, LPCSTR szCmd, LPCSTR szContent);


	bool assure_buffer_size(message *&msg, size_t size, LPCTSTR when);
	void handle_write(const boost::system::error_code &ec,
		size_t bytes_transferred, message *msg, size_t offset);
	void handle_write_ex(const boost::system::error_code &ec,
		size_t bytes_transferred, message *msg, size_t offset, void *ctx);
	void handle_read(const boost::system::error_code& error,
		size_t bytes_transferred, size_t offset);


public:
	bool verify_certificate(bool preverified,
		boost::asio::ssl::verify_context& ctx);

protected:
	static DWORD __stdcall _ThreadProc(CBoostNetwork *pthis);

protected:
	message *msg_read_;
	HANDLE io_thread_;
	boost::shared_ptr<client_io_service> io_;
	INetworkSink *sink_;
	bool connected_;
};

INetwork *CreateClientInstance();

