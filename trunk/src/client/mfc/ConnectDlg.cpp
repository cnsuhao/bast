// ConnectDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "MainApp.h"
#include "ConnectDlg.h"
#include "../../common/pugixml.hpp"

// CConnectDlg 对话框

IMPLEMENT_DYNAMIC(CConnectDlg, CDialogEx)

CConnectDlg::CConnectDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_CONNECT_SETS, pParent)
{
	TCHAR szModule[MAX_PATH] = { 0 };
	GetModuleFileName(NULL, szModule, MAX_PATH);
	_tcsrchr(szModule, _T('\\'))[1] = 0;
	_tcscat_s(szModule, _T("client.dat"));
	pugi::xml_document doc;
	doc.load_file(szModule);
	pugi::xpath_variable_set vars;
	pugi::xpath_node_set _set = doc.select_nodes(_T("/root/serverlist/server"),
		&vars);
	for (pugi::xpath_node_set::const_iterator it = _set.begin(); it != _set.end(); ++it)
	{
		CConnectOpt opt;
		opt.m_strName = (*it).node().attribute(_T("name")).value();
		opt.m_strAddr = (*it).node().attribute(_T("addr")).value();
		opt.m_strPort = (*it).node().attribute(_T("port")).value();
		opt.m_strCert = (*it).node().attribute(_T("cert")).value();
		m_servers.AddTail(opt);
	}

	m_bSaved = TRUE;
}

CConnectDlg::~CConnectDlg()
{
}

void CConnectDlg::DoDataExchange(CDataExchange* pDX)
{
	int nCurItem = m_pListBox->GetCurSel();
	if (nCurItem != -1)
	{
		CConnectOpt &opt = m_servers.GetAt((POSITION)
			m_pListBox->GetItemDataPtr(nCurItem));

		DDX_Text(pDX, IDC_EDIT_NAME, opt.m_strName);
		DDX_Text(pDX, IDC_EDIT_ADDR, opt.m_strAddr);
		DDX_Text(pDX, IDC_EDIT_PORT, opt.m_strPort);
		DDX_Text(pDX, IDC_EDIT_CERT, opt.m_strCert);
		if (pDX->m_bSaveAndValidate)
		{
			CString str;
			m_pListBox->GetText(m_pListBox->GetCurSel(), str);
			if (str != opt.m_strName)
			{
				POSITION pos = (POSITION)m_pListBox->GetItemDataPtr(nCurItem);
				m_pListBox->DeleteString(nCurItem);
				nCurItem = m_pListBox->InsertString(nCurItem, opt.m_strName);
				m_pListBox->SetItemDataPtr(nCurItem, pos);
				m_pListBox->SetCurSel(nCurItem);
			}
		}

		GetDlgItem(IDC_EDIT_NAME)->EnableWindow();
		GetDlgItem(IDC_EDIT_ADDR)->EnableWindow();
		GetDlgItem(IDC_EDIT_PORT)->EnableWindow();
		GetDlgItem(IDC_EDIT_CERT)->EnableWindow();
	}
	else if (!pDX->m_bSaveAndValidate)
	{
		SetDlgItemText(IDC_EDIT_NAME, _T(""));
		SetDlgItemText(IDC_EDIT_ADDR, _T(""));
		SetDlgItemText(IDC_EDIT_PORT, _T(""));
		SetDlgItemText(IDC_EDIT_CERT, _T(""));
		GetDlgItem(IDC_EDIT_NAME)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDIT_ADDR)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDIT_PORT)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDIT_CERT)->EnableWindow(FALSE);
	}
	CDialogEx::DoDataExchange(pDX);
}


BOOL CConnectDlg::PreClose()
{
	if (m_bSaved)
		return TRUE;

	switch (GetFocus()->GetDlgCtrlID())
	{
	case IDC_EDIT_NAME:
	case IDC_EDIT_ADDR:
	case IDC_EDIT_PORT:
	case IDC_EDIT_CERT:
		KillFocusUpdates(GetFocus()->GetDlgCtrlID());
	}

	TCHAR szModule[MAX_PATH] = { 0 };
	GetModuleFileName(NULL, szModule, MAX_PATH);
	_tcsrchr(szModule, _T('\\'))[1] = 0;
	_tcscat_s(szModule, _T("client.dat"));
	pugi::xml_document doc;
	doc.load_file(szModule);
	pugi::xpath_variable_set vars;
	pugi::xpath_node _n = doc.select_single_node(_T("/root/serverlist"));
	if (!!_n)
		while (_n.node().remove_child(_T("server")));
	else
	{
		if (!(doc.child(_T("root"))))
			doc.append_child(_T("root"));
		doc.child(_T("root")).append_child(_T("serverlist"));
		_n = doc.select_single_node(_T("/root/serverlist"));
	}
	for (int n = 0; n < m_pListBox->GetCount(); ++n)
	{
		pugi::xml_node tmp = _n.node().append_child(_T("server"));
		CConnectOpt &opt = m_servers.GetAt((POSITION)m_pListBox->GetItemDataPtr(n));
		tmp.append_attribute(_T("name")).set_value(opt.m_strName);
		tmp.append_attribute(_T("addr")).set_value(opt.m_strAddr);
		tmp.append_attribute(_T("port")).set_value(opt.m_strPort);
		tmp.append_attribute(_T("cert")).set_value(opt.m_strCert);
	}
	doc.save_file(szModule);
	m_bSaved = TRUE;
	return TRUE;
}

BEGIN_MESSAGE_MAP(CConnectDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_ADD, &CConnectDlg::OnBnClickedButtonAdd)
	ON_LBN_SELCHANGE(IDC_LIST_SERVER, &CConnectDlg::OnLbnSelchangeListServer)
	ON_WM_CLOSE()
	ON_CONTROL_RANGE(EN_KILLFOCUS, IDC_EDIT_NAME, IDC_EDIT_CERT, KillFocusUpdates)
	ON_EN_CHANGE(IDC_EDIT_CERT, OnEnChangeCert)
	ON_BN_CLICKED(IDC_CHECK_RELATIVE, &CConnectDlg::OnBnClickedCheckRelative)
	ON_BN_CLICKED(IDC_BUTTON_DEL, &CConnectDlg::OnBnClickedButtonDel)
END_MESSAGE_MAP()


// CConnectDlg 消息处理程序


BOOL CConnectDlg::OnInitDialog()
{
	m_pListBox = (CListBox*)GetDlgItem(IDC_LIST_SERVER);
	CDialogEx::OnInitDialog();
	POSITION pos = m_servers.GetHeadPosition();

	while (pos)
	{
		m_pListBox->SetItemDataPtr(
			m_pListBox->AddString(m_servers.GetNext(pos).m_strName), pos);
	}

	if (m_pListBox->GetCount())
		m_pListBox->SetCurSel(0);
	UpdateData(FALSE);
	OnEnChangeCert();
	return TRUE;
}


void CConnectDlg::OnBnClickedButtonAdd()
{
	CConnectOpt opt;
	opt.m_strName = _T("新项");
	m_pListBox->SetItemDataPtr(m_pListBox->AddString(opt.m_strName), m_servers.AddTail(opt));
	m_pListBox->SelectString(m_pListBox->GetCount() - 1, opt.m_strName);
	OnLbnSelchangeListServer();
	GetDlgItem(IDC_BUTTON_ADD)->SetFocus();
	m_bSaved = FALSE;
}


void CConnectDlg::OnLbnSelchangeListServer()
{
	UpdateData(FALSE);
}


void CConnectDlg::OnClose()
{
	PreClose();
	CDialogEx::OnClose();
}


void CConnectDlg::KillFocusUpdates(UINT uID)
{
	int nItem = m_pListBox->GetCurSel();
	if (nItem != -1)
	{
		CString str;
		GetDlgItemText(uID, str);
		CConnectOpt &opt = m_servers.GetAt(
			(POSITION)m_pListBox->GetItemDataPtr(nItem));
		switch (uID)
		{
		case IDC_EDIT_NAME:
			if (opt.m_strName != str)
			{
				POSITION pos = (POSITION)m_pListBox->GetItemDataPtr(nItem);
				opt.m_strName = str;
				m_pListBox->DeleteString(nItem);
				nItem = m_pListBox->InsertString(nItem, opt.m_strName);
				m_pListBox->SetItemDataPtr(nItem, pos);
				m_pListBox->SetCurSel(nItem);
				m_bSaved = FALSE;
			}
			break;
		case IDC_EDIT_ADDR:
			if (opt.m_strAddr != str)
			{
				opt.m_strAddr = str;
				m_bSaved = FALSE;
			}
			break;
		case IDC_EDIT_PORT:
			if (opt.m_strPort != str)
			{
				opt.m_strPort = str;
				m_bSaved = FALSE;
			}
			break;
		case IDC_EDIT_CERT:
			if (opt.m_strCert != str)
			{
				opt.m_strCert = str;
				m_bSaved = FALSE;
			}
		}
	}
	else
		UpdateData(TRUE);
}

void CConnectDlg::OnEnChangeCert()
{
	CString str;
	GetDlgItemText(IDC_EDIT_CERT, str);
	if (str.IsEmpty())
	{
		CheckDlgButton(IDC_CHECK_RELATIVE, BST_UNCHECKED);
		GetDlgItem(IDC_CHECK_RELATIVE)->EnableWindow(FALSE);
	}
	else
	{
		GetDlgItem(IDC_CHECK_RELATIVE)->EnableWindow(TRUE);
		CheckDlgButton(IDC_CHECK_RELATIVE, (PathIsRelative(str)));
	}
}


void CConnectDlg::OnBnClickedCheckRelative()
{
	CString str;
	TCHAR szModule[MAX_PATH];
	GetDlgItemText(IDC_EDIT_CERT, str);
	if (PathIsRelative(str))
	{
		TCHAR szFmt[MAX_PATH] = { 0 };
		_tcscpy_s(szFmt, str);

		LPTSTR lpCan = szFmt;
		while (lpCan = _tcschr(lpCan, _T('/')))
		{
			lpCan[0] = _T('\\');
		}

		GetModuleFileName(NULL, szModule, MAX_PATH);
		LPTSTR p = wcsrchr(szModule, _T('\\'));
		if (p)
		{
			p[1] = 0;
		}

		PathAppend(szModule, szFmt);
		SetDlgItemText(IDC_EDIT_CERT, szModule);
	}
	else
	{
		TCHAR szOut[MAX_PATH + 1];
		GetModuleFileName(NULL, szModule, MAX_PATH);
		PathRelativePathTo(szOut, szModule, FILE_ATTRIBUTE_NORMAL,
			str, FILE_ATTRIBUTE_NORMAL);
		SetDlgItemText(IDC_EDIT_CERT, szOut);
	}
	KillFocusUpdates(IDC_EDIT_CERT);
}

void CConnectDlg::OnBnClickedButtonDel()
{
	int nItem = m_pListBox->GetCurSel();
	if (nItem == -1)
		return;

	m_servers.RemoveAt((POSITION)m_pListBox->GetItemDataPtr(nItem));
	m_pListBox->DeleteString(nItem);
	m_bSaved = FALSE;
	if (m_pListBox->GetCount() > nItem || ((nItem = m_pListBox->GetCount() - 1) != -1))
	{
		m_pListBox->SetCurSel(nItem);
	}
	OnLbnSelchangeListServer();
}

void CConnectDlg::OnOK()
{
	PreClose();
	CDialogEx::OnOK();
}

void CConnectDlg::OnCancel()
{
	// aborting changes
	UpdateData(FALSE);
	CDialogEx::OnCancel();
}
