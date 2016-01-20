
// MainFrm.h : CMainFrame 类的接口
//

#pragma once
#include "ConnectDlg.h"
#include "DataDlg.h"
#include "FlatSplitterWnd.h"
#include "../../common/interface.h"

using namespace client;

class CLogCtrlSink
{
#define CLR_FATAL	RGB(255, 0, 0)
#define CLR_INFO	RGB(0,128,0)
#define CLR_STAT	RGB(64,64,64)
#define CLR_CTT		RGB(134, 0, 0)

public:
	BOOL InitializeLogCtrl(CRichEditCtrl *pRichEdit)
	{
		m_pCtrl = pRichEdit;
		return (m_pCtrl != NULL);
	}

	void AppendLog(COLORREF rgb, LPCTSTR fmt, ...)
	{
		int nOldLines = 0, nNewLines = 0, nScroll = 0;
		long nInsertionPoint = 0;
		CHARFORMAT cf;
		va_list ap;
		va_start(ap, fmt);
		CString str;
		str.FormatV(fmt, ap);

		// Save number of lines before insertion of new text
		nOldLines = m_pCtrl->GetLineCount();

		// Initialize character format structure
		cf.cbSize = sizeof(CHARFORMAT);
		cf.dwMask = CFM_COLOR;
		cf.dwEffects = 0;	// To disable CFE_AUTOCOLOR
		cf.crTextColor = rgb;

		// Set insertion point to end of text
		nInsertionPoint = m_pCtrl->GetWindowTextLength();
		nInsertionPoint = -1;
		m_pCtrl->SetSel(nInsertionPoint, -1);

		//  Set the character format
		m_pCtrl->SetSelectionCharFormat(cf);

		// Replace selection. Because we have nothing selected, this will simply insert
		// the string at the current caret position.
		m_pCtrl->ReplaceSel(str);

		// Get new line count
		nNewLines = m_pCtrl->GetLineCount();

		// Scroll by the number of lines just inserted
		nScroll = nNewLines - nOldLines;
		m_pCtrl->LineScroll(nScroll);
	}

	struct COLORINFO
	{
		COLORREF clr;
		int length;
	};

	void AppendScrollLog(time_t t, COLORINFO *info, LPCTSTR fmt, ...)
	{
		long nVisible = 0;
		long nInsertionPoint = 0;
		CHARFORMAT cf;
		va_list ap;
		CString str;
		CTime tt(t);
		str = tt.Format("%H:%M:%S> ");
		int nTimeLength = str.GetLength();
		va_start(ap, fmt);
		str.AppendFormatV(fmt, ap);

		// Initialize character format structure
		cf.cbSize = sizeof(CHARFORMAT);
		cf.dwMask = CFM_COLOR;
		cf.dwEffects = 0; // To disable CFE_AUTOCOLOR

#ifdef LOG_TRACE
		CString strOutput;
#endif
		// Set insertion point to end of text
		nInsertionPoint = m_pCtrl->GetWindowTextLength();
		m_pCtrl->SetSel(nInsertionPoint, -1);

		nInsertionPoint -= m_pCtrl->GetLineCount()-1;
#ifdef LOG_TRACE
		strOutput.Format(_T("%s [%d, %d]"), (LPCTSTR)str, nInsertionPoint, str.GetLength());
#endif

		// Replace selection. Because we have nothing 
		// selected, this will simply insert
		// the string at the current caret position.
		m_pCtrl->ReplaceSel(str);

		nTimeLength += nInsertionPoint;
		m_pCtrl->SetSel(nInsertionPoint, nTimeLength);
		cf.crTextColor = RGB(140,140,140);
		m_pCtrl->SetSelectionCharFormat(cf);

		for (size_t i=0; ; ++i)
		{
			cf.crTextColor = info[i].clr;
			if (info[i].length == -1)
			{
				m_pCtrl->SetSel(nTimeLength, nInsertionPoint+str.GetLength());
				m_pCtrl->SetSelectionCharFormat(cf);
				break;
			}
#ifdef LOG_TRACE
			strOutput.AppendFormat(_T("[%d]"), info[i].length);
#endif
			m_pCtrl->SetSel(nTimeLength, nTimeLength + info[i].length);
			m_pCtrl->SetSelectionCharFormat(cf);
			nTimeLength += info[i].length;
		}
#ifdef LOG_TRACE
		OutputDebugString(strOutput);
#endif

		m_pCtrl->SetSel(-1, -1);
// 		m_pCtrl->SetSel(nInsertionPoint + str.GetLength(), -1);
// 
// 		// Get number of currently visible lines or maximum number of visible lines
// 		// (We must call GetNumVisibleLines() before the first call to LineScroll()!)
// 		nVisible = GetNumVisibleLines();
// 
// 		// Now this is the fix of CRichEditCtrl's abnormal behaviour when used
// 		// in an application not based on dialogs. Checking the focus prevents
// 		// us from scrolling when the CRichEditCtrl does so automatically,
// 		// even though ES_AUTOxSCROLL style is NOT set.
// 		if (m_pCtrl != m_pCtrl->GetFocus())
// 		{
// 			m_pCtrl->LineScroll(INT_MAX);
// 			m_pCtrl->LineScroll(1 - nVisible);
// 		}
	}

	int GetNumVisibleLines()
	{
		CRect rect;
		long nFirstChar, nLastChar;
		long nFirstLine, nLastLine;

		// Get client rect of rich edit control
		m_pCtrl->GetClientRect(rect);

		// Get character index close to upper left corner
		nFirstChar = m_pCtrl->CharFromPos(CPoint(0, 0));

		// Get character index close to lower right corner
		nLastChar = m_pCtrl->CharFromPos(CPoint(rect.right, rect.bottom));
		if (nLastChar < 0)
		{
			nLastChar = m_pCtrl->GetTextLength();
		}

		// Convert to lines
		nFirstLine = m_pCtrl->LineFromChar(nFirstChar);
		nLastLine = m_pCtrl->LineFromChar(nLastChar);

		return (nLastLine - nFirstLine);
	}

protected:
	CRichEditCtrl *m_pCtrl;
};



class CMainFrame : public CFrameWnd, public INetworkSink, public CLogCtrlSink
{
	
public:
	CMainFrame();
protected: 
	DECLARE_DYNAMIC(CMainFrame)

// 属性
public:

// 操作
public:

// 重写
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);

	// INetworkSink
	virtual void OnConnect();
	virtual void OnConnectFailed(LPCTSTR szError);
	virtual void OnDisconnect();
	virtual void OnReceive(LPCSTR szCmd, LPCSTR szContent);
	virtual void OnSendComplete(void *ctx);
	virtual void OnException(LPCTSTR szError);

// 实现
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

public:
	void UpdateConnectMenu();
	void UpdateDataList();

public:
	// network
	BOOL DoConnect(UINT i);

protected:  // 控件条嵌入成员
	CToolBar			m_wndToolBar;
	CStatusBar			m_wndStatusBar;
	CFlatSplitterWnd	m_wndSplitter;
	CConnectDlg			m_connectMgr;
	CDataDlg			m_dataDlg;
	INetwork			*m_pNetwork;
	volatile LONG		m_lPkgIndex;

// 生成的消息映射函数
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSetFocus(CWnd *pOldWnd);
	DECLARE_MESSAGE_MAP()

	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
public:
	afx_msg void OnUpdateConnectManage(CCmdUI *pCmdUI);
	afx_msg void OnUpdateConnect(CCmdUI *pCmdUI);
	afx_msg void OnUpdateConnectClose(CCmdUI *pCmdUI);
	afx_msg void OnConnectManage();
	afx_msg void OnConnectServer(UINT uID);
	afx_msg LRESULT OnNetworkMsg(WPARAM wParam, LPARAM lParam);
	afx_msg void OnCls();
	afx_msg void OnData();
	/*afx_msg */void OnLogSelChange(LPNMHDR hdr, LRESULT *plResult);
	/*afx_msg */void OnNMClickData(int nItem);
	afx_msg void OnConnectClose();
};


