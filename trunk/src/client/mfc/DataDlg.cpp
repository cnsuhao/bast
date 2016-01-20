// DataDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "MainApp.h"
#include "DataDlg.h"
#include "../../common/pugixml.hpp"


// CDataDlg 对话框

IMPLEMENT_DYNAMIC(CDataDlg, CDialog)

CDataDlg::CDataDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDataDlg::IDD, pParent)
	, m_nSaveListItem(-1)
	, m_bAdding(FALSE)
{
	GetModuleFileName(NULL, m_szModule, MAX_PATH);
	_tcsrchr(m_szModule, _T('\\'))[1] = 0;
	_tcscat_s(m_szModule, _T("client.dat"));
}

CDataDlg::~CDataDlg()
{
}

void CDataDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


void CDataDlg::EnableEdits(BOOL bEnable /*= TRUE*/)
{
	GetDlgItem(IDC_EDIT_NAME)->EnableWindow(bEnable);
	GetDlgItem(IDC_EDIT_COMMAND)->EnableWindow(bEnable);
	GetDlgItem(IDC_EDIT_CONTENT)->EnableWindow(bEnable);
}

BOOL CDataDlg::SaveNode()
{
	if (m_nSaveListItem != -1)
	{
		CString strName, strCommand, strContent;
		pugi::xml_document doc;
		pugi::xpath_variable_set vars;
		pugi::xpath_node xpnode_;
		pugi::xml_node node_;

		// input validation
		GetDlgItemText(IDC_EDIT_NAME, strName);
		GetDlgItemText(IDC_EDIT_COMMAND, strCommand);
		GetDlgItemText(IDC_EDIT_CONTENT, strContent);
		if (strName.IsEmpty())
		{
			MessageBox(_T("名称不能为空"), NULL, MB_ICONINFORMATION);
			m_pListBox->SetCurSel(m_nSaveListItem);
			GetDlgItem(IDC_EDIT_NAME)->SetFocus();
			return FALSE;
		}
		if (strCommand.IsEmpty())
		{
			MessageBox(_T("命令不能为空"), NULL, MB_ICONINFORMATION);
			m_pListBox->SetCurSel(m_nSaveListItem);
			GetDlgItem(IDC_EDIT_COMMAND)->SetFocus();
			return FALSE;
		}

		// setting file node preparation
		doc.load_file(m_szModule);
		vars.set(_T("v"), strName);
		xpnode_ = doc.select_single_node(_T("/root/datalist/data[@name=string($v)]"),
			&vars);

		// node writing
		if (!xpnode_)
		{
			// new node
			node_ = doc.child(_T("root"));
			if (!node_)
				node_ = doc.append_child(_T("root"));

			if (!node_.child(_T("datalist")))
				node_ = node_.append_child(_T("datalist"));
			else
				node_ = node_.child(_T("datalist"));

			node_ = node_.append_child(_T("data"));

			node_.append_attribute(_T("name")).set_value(strName);
			node_.append_attribute(_T("command")).set_value(strCommand);
			node_.append_attribute(_T("content")).set_value(strContent);
		}
		else
		{
			if (m_bAdding)
			{
				MessageBox(_T("相同名称的数据包已存在!"), NULL, MB_ICONWARNING);
				GetDlgItem(IDC_EDIT_NAME)->SetFocus();
				return FALSE;
			}
			xpnode_.node().attribute(_T("name")).set_value(strName);
			xpnode_.node().attribute(_T("command")).set_value(strCommand);
			xpnode_.node().attribute(_T("content")).set_value(strContent);
		}
		doc.save_file(m_szModule);

		// update listbox
		m_pListBox->GetText(m_nSaveListItem, strCommand);
		if (strCommand != strName)
		{
			// update item
			m_pListBox->DeleteString(m_nSaveListItem);
			m_pListBox->InsertString(m_nSaveListItem, strName);
		}

		m_nSaveListItem = -1;
	}
	return TRUE;
}

struct CMD_ENUM
{
	pugi::xml_document doc;
	pugi::xpath_node_set set;
	pugi::xpath_node_set::const_iterator iter;
	pugi::xpath_variable_set vars;
};


HANDLE CDataDlg::StartEnumCommands()
{
	CMD_ENUM *p = new CMD_ENUM;
	p->doc.load_file(m_szModule);
	p->set = p->doc.select_nodes(_T("/root/datalist/data"), &p->vars);
	p->iter = p->set.begin();
	return (HANDLE)p;
}


BOOL CDataDlg::EnumNextCommand(HANDLE h, CString &cmd)
{
	if (!h)
		return FALSE;

	CMD_ENUM *p = (CMD_ENUM*)h;
	if (p->iter == p->set.end())
		return FALSE;

	cmd = (*p->iter).node().attribute(_T("name")).value();
	++p->iter;
	return TRUE;
}

void CDataDlg::CompleteEnumCommands(HANDLE h)
{
	if (!h)
		return;
	delete (CMD_ENUM*)h;
}

BOOL CDataDlg::GetCommand(LPCTSTR szName, CString &cmd, CString &content)
{
	pugi::xml_document doc;
	pugi::xpath_node _node;
	pugi::xpath_variable_set vars;
	if (!doc.load_file(m_szModule))
		return FALSE;

	vars.set(_T("v"), szName);
	_node = doc.select_single_node(_T("/root/datalist/data[@name=string($v)]"),
		&vars);

	if (!_node)
		return FALSE;

	cmd = _node.node().attribute(_T("command")).value();
	content = _node.node().attribute(_T("content")).value();
	return !cmd.IsEmpty();
}

BEGIN_MESSAGE_MAP(CDataDlg, CDialog)
	ON_LBN_SELCHANGE(IDC_LIST_DATA, &CDataDlg::OnLbnSelchangeListData)
	ON_BN_CLICKED(IDC_BUTTON_ADD, &CDataDlg::OnBnClickedButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_DEL, &CDataDlg::OnBnClickedButtonDel)
	ON_NOTIFY_RANGE(EN_CHANGE, IDC_EDIT_NAME, IDC_EDIT_CONTENT, &CDataDlg::OnEditsChange)
	ON_WM_CLOSE()
END_MESSAGE_MAP()


// CDataDlg 消息处理程序

BOOL CDataDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	pugi::xml_document doc;
	doc.load_file(m_szModule);

	m_pListBox = (CListBox*)GetDlgItem(IDC_LIST_DATA);

	pugi::xpath_variable_set vars;
	pugi::xpath_node_set _set = doc.select_nodes(_T("/root/datalist/data"),
		&vars);
	for (pugi::xpath_node_set::const_iterator it = _set.begin(); it != _set.end(); ++it)
	{
		m_pListBox->AddString((*it).node().attribute(_T("name")).value());
	}

	if (_set.begin() == _set.end())
		EnableEdits(FALSE);
	else
	{
		m_pListBox->SetCurSel(0);
		SetDlgItemText(IDC_EDIT_NAME,
			(*_set.begin()).node().attribute(_T("name")).value());
		SetDlgItemText(IDC_EDIT_COMMAND,
			(*_set.begin()).node().attribute(_T("command")).value());
		SetDlgItemText(IDC_EDIT_CONTENT,
			(*_set.begin()).node().attribute(_T("content")).value());
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

void CDataDlg::OnLbnSelchangeListData()
{
	CString str;
	pugi::xml_document doc;
	pugi::xpath_variable_set vars;
	pugi::xpath_node _node;

	if (!SaveNode())
		return;

	m_bAdding = FALSE;
	doc.load_file(m_szModule);
	int nCurSel = m_pListBox->GetCurSel();
	if (nCurSel == -1)
	{
		SetDlgItemText(IDC_EDIT_NAME, _T(""));
		SetDlgItemText(IDC_EDIT_COMMAND, _T(""));
		SetDlgItemText(IDC_EDIT_CONTENT, _T(""));
		EnableEdits(FALSE);
		return;
	}
	m_pListBox->GetText(nCurSel, str);
	vars.set(_T("v"), str);

	_node = doc.select_single_node(_T("/root/datalist/data[@name=string($v)]"),
		&vars);

	if (_node)
	{
		EnableEdits();
		SetDlgItemText(IDC_EDIT_NAME, str);
		SetDlgItemText(IDC_EDIT_COMMAND, _node.node().attribute(_T("command")).value());
		SetDlgItemText(IDC_EDIT_CONTENT, _node.node().attribute(_T("content")).value());
	}
}

void CDataDlg::OnBnClickedButtonAdd()
{
	if (!SaveNode())
		return;

	m_nSaveListItem = m_pListBox->AddString(_T("新项"));
	m_pListBox->SetCurSel(m_nSaveListItem);
	EnableEdits();
	SetDlgItemText(IDC_EDIT_NAME, _T("新项"));
	SetDlgItemText(IDC_EDIT_COMMAND, _T(""));
	SetDlgItemText(IDC_EDIT_CONTENT, _T(""));
	GetDlgItem(IDC_EDIT_NAME)->SetFocus();
	((CEdit*)GetDlgItem(IDC_EDIT_NAME))->SetSel(0, -1);
	m_bAdding = TRUE;
}

void CDataDlg::OnBnClickedButtonDel()
{
	int nItem = m_pListBox->GetCurSel();
	if (!m_bAdding)
	{
		CString str;
		pugi::xml_document doc;
		pugi::xpath_variable_set vars;
		pugi::xpath_node _node;
		doc.load_file(m_szModule);
		m_pListBox->GetText(m_nSaveListItem, str);
		vars.set(_T("v"), str);
		_node = doc.select_single_node(_T("/root/datalist/data[@name=string($v)]"),
			&vars);
		if (_node)
		{
			_node.node().parent().remove_child(_node.node());
			doc.save_file(m_szModule);
		}
	}
	else
		m_bAdding = FALSE;
	m_nSaveListItem = -1;

	m_pListBox->DeleteString(nItem);

	if (m_pListBox->GetCount() > nItem)
		m_pListBox->SetCurSel(nItem);
	else if (m_pListBox->GetCount() == nItem)
		m_pListBox->SetCurSel(nItem-1);
	else if (m_pListBox->GetCount())
		m_pListBox->SetCurSel(0);

	OnLbnSelchangeListData();
}

void CDataDlg::OnEditsChange(UINT uID, LPNMHDR hdr, LRESULT *plResult)
{
	if (m_nSaveListItem == -1)
		m_nSaveListItem = m_pListBox->GetCurSel();
}

void CDataDlg::OnClose()
{
	if (!SaveNode())
		return;

	CDialog::OnClose();
}
