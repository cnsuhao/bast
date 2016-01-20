#pragma once


// CDataDlg 对话框

class CDataDlg : public CDialog
{
	DECLARE_DYNAMIC(CDataDlg)

public:
	CDataDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CDataDlg();

// 对话框数据
	enum { IDD = IDD_DATA };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	void EnableEdits(BOOL bEnable = TRUE);

	BOOL SaveNode();

public:
	HANDLE StartEnumCommands();
	BOOL EnumNextCommand(HANDLE h, CString &cmd);
	void CompleteEnumCommands(HANDLE h);

	BOOL GetCommand(LPCTSTR szName, CString &cmd, CString &content);

protected:
	TCHAR m_szModule[MAX_PATH];
	CListBox *m_pListBox;
	int m_nSaveListItem;
	BOOL m_bAdding;

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnLbnSelchangeListData();
	afx_msg void OnBnClickedButtonAdd();
	afx_msg void OnBnClickedButtonDel();
	afx_msg void OnEditsChange(UINT uID, LPNMHDR hdr, LRESULT *plResult);
	afx_msg void OnClose();
};
