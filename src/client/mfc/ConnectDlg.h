#pragma once

class CConnectOpt
{
public:
	CString m_strName;
	CString m_strAddr;
	CString m_strPort;
	CString m_strCert;
};

typedef CList<CConnectOpt> ConnectOpts;

// CConnectDlg 对话框

class CConnectDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CConnectDlg)

public:
	CConnectDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CConnectDlg();

	ConnectOpts &GetList() { return m_servers; }

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CONNECT_SETS };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

	BOOL PreClose();


protected:
	CListBox *m_pListBox;
	ConnectOpts m_servers;
	BOOL m_bSaved;

public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedButtonAdd();
	afx_msg void OnBnClickedButtonDel();
	afx_msg void OnLbnSelchangeListServer();
	afx_msg void OnClose();
	afx_msg void KillFocusUpdates(UINT uID);
	afx_msg void OnEnChangeCert();
	afx_msg void OnBnClickedCheckRelative();
protected:
	virtual void OnOK();
	virtual void OnCancel();
};
