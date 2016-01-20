
// MainFrm.cpp : CMainFrame ���ʵ��
//

#include "stdafx.h"
#include "MainApp.h"
#include <afxcview.h>
#include <afxrich.h>

#include "MainFrm.h"
#include "../../common/Utils.h"

#define WM_NET_MSG WM_USER+100

struct NetworkMsg
{
	NetworkMsg(int tp, void*m1=NULL, void*m2=NULL)
	{
		type=(_TYPE)tp;
		msg1=m1;
		msg2=m2;
		t=time(NULL);
	}
	~NetworkMsg(){
		switch (type)
		{
		case _OnException:
		case _OnConnectFailed:free(msg1);break;
		case _OnReceive:delete [] (LPTSTR)msg1;delete [] (LPTSTR)msg2;break;
		default:{} 
		}
	}
	enum _TYPE{
		_OnConnect = 1,
		_OnConnectFailed,
		_OnDisconnect,
		_OnReceive,
		_OnSendComplete,
		_OnException
	} type;
	time_t t;
	void *msg1;
	void *msg2;
};

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	ON_WM_CREATE()
	ON_WM_SETFOCUS()
	ON_UPDATE_COMMAND_UI(ID_CONNECT_MANAGE, &CMainFrame::OnUpdateConnectManage)
	ON_COMMAND(ID_CONNECT_MANAGE, &CMainFrame::OnConnectManage)
	ON_UPDATE_COMMAND_UI_RANGE(ID_CONNECT_1, ID_CONNECT_1+1000, &CMainFrame::OnUpdateConnect)
	ON_COMMAND_RANGE(ID_CONNECT_1, ID_CONNECT_1+1000, &CMainFrame::OnConnectServer)
	ON_MESSAGE(WM_NET_MSG, OnNetworkMsg)
	ON_COMMAND(ID_CLS, &CMainFrame::OnCls)
	ON_COMMAND(ID_DATA, &CMainFrame::OnData)
	ON_UPDATE_COMMAND_UI(ID_CONNECT_CLOSE, &CMainFrame::OnUpdateConnectClose)
	ON_COMMAND(ID_CONNECT_CLOSE, &CMainFrame::OnConnectClose)
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // ״̬��ָʾ��
	ID_INDICATOR_IDX,
	ID_INDICATOR_CNT,
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

// CMainFrame ����/����

CMainFrame::CMainFrame() : m_lPkgIndex(0)
{
	extern INetwork *CreateClientInstance();
	m_pNetwork = CreateClientInstance();
}

CMainFrame::~CMainFrame()
{
	if (m_pNetwork)
	{
		m_pNetwork->Close();
		m_pNetwork->Destroy();
		m_pNetwork = NULL;
	}
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	LPCTSTR szError;
	if (!m_pNetwork)
	{
		MessageBox(_T("���������ʧ��!"), NULL, MB_ICONERROR);
		return -1;
	}

	if (!m_pNetwork->Initialize(&szError, this))
	{
		MessageBox(_T("��ʼ���������ʧ��!"), NULL, MB_ICONERROR);
		return -1;
	}

	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("δ�ܴ���������\n");
		return -1;      // δ�ܴ���
	}

	if (!m_wndStatusBar.Create(this))
	{
		TRACE0("δ�ܴ���״̬��\n");
		return -1;      // δ�ܴ���
	}
	m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT));

	m_wndStatusBar.SetPaneInfo(1, ID_INDICATOR_IDX, SBPS_NORMAL, 80);
	m_wndStatusBar.SetPaneInfo(2, ID_INDICATOR_CNT, SBPS_NORMAL, 80);
	// TODO: �������Ҫ��ͣ������������ɾ��������
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);

	UpdateConnectMenu();

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	// TODO: �ڴ˴�ͨ���޸�
	//  CREATESTRUCT cs ���޸Ĵ��������ʽ

	cs.dwExStyle &= ~WS_EX_CLIENTEDGE;
	cs.lpszClass = AfxRegisterWndClass(0);
	return TRUE;
}

BOOL CMainFrame::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	if (((LPNMHDR)lParam)->code == EN_SELCHANGE
		&& ((LPNMHDR)lParam)->hwndFrom == m_pCtrl->m_hWnd)
		OnLogSelChange((LPNMHDR)lParam, pResult);

	switch (((LPNMHDR)lParam)->code)
	{
	case EN_SELCHANGE:
		if (((LPNMHDR)lParam)->hwndFrom == m_pCtrl->m_hWnd)
			OnLogSelChange((LPNMHDR)lParam, pResult);
		break;
	case NM_DBLCLK:
		if (((LPNMHDR)lParam)->hwndFrom == m_wndSplitter.GetPane(0,0)->m_hWnd)
			OnNMClickData(((LPNMITEMACTIVATE)lParam)->iItem);
	}

	return __super::OnNotify(wParam, lParam, pResult);
}

// CMainFrame ���

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG



void CMainFrame::UpdateConnectMenu()
{
	CMenu *pMenu = GetMenu()->GetSubMenu(0)->GetSubMenu(0);
	while (pMenu->DeleteMenu(4, MF_BYPOSITION));
	POSITION pos = m_connectMgr.GetList().GetHeadPosition();
	int i = 0;
	CString str;
	while (pos)
	{
		CConnectOpt &opt = m_connectMgr.GetList().GetNext(pos);
		str.Format(_T("&%d %s(%s:%s)\t%s"), i+1, opt.m_strName,
			opt.m_strAddr, opt.m_strPort, opt.m_strCert);
		pMenu->AppendMenu(MF_STRING, ID_CONNECT_1 + i++, str);
	}
}


void CMainFrame::UpdateDataList()
{
	CListCtrl &listctrl = ((CListView*)m_wndSplitter.GetPane(0,0))->GetListCtrl();
	listctrl.DeleteAllItems();
	HANDLE h = m_dataDlg.StartEnumCommands();
	CString str;
	for (int i=0; m_dataDlg.EnumNextCommand(h, str); ++i)
	{
		listctrl.InsertItem(i, str);
	}
	m_dataDlg.CompleteEnumCommands(h);
}

BOOL CMainFrame::DoConnect(UINT uIndex)
{
	m_pNetwork->Close();
	POSITION pos = m_connectMgr.GetList().GetHeadPosition();
	UINT i = 0;
	COLORINFO clr = {CLR_INFO, -1};
	while (pos)
	{
		CConnectOpt &opt = m_connectMgr.GetList().GetNext(pos);
		if (i++ == uIndex)
		{
			AppendScrollLog(time(NULL), &clr, _T("��������%s:%s...\n"),
				(LPCTSTR)opt.m_strAddr, (LPCTSTR)opt.m_strPort);
			LPTSTR pszCertAbsPath = GetAbsPath(opt.m_strCert);
			if (!m_pNetwork->LoadCert(pszCertAbsPath))
			{
				LPTSTR szError = ErrorText(GetLastError());
				if (pszCertAbsPath)
					delete [] pszCertAbsPath;
				clr.clr = CLR_FATAL;
				AppendScrollLog(time(NULL), &clr, _T("����֤��%sʧ�ܣ�%s\n"),
					(LPCTSTR)opt.m_strCert, szError);
				LocalFree(szError);
				return FALSE;
			}
			delete [] pszCertAbsPath;

			CStringA strAddr;
			strAddr = opt.m_strAddr;
			if (!m_pNetwork->Connect(strAddr, _ttoi(opt.m_strPort)))
			{
				LPTSTR szError = ErrorText(GetLastError());
				clr.clr = CLR_FATAL;
				AppendScrollLog(time(NULL), &clr, _T("����ʧ�ܣ�%s\n"), szError);
				LocalFree(szError);
				return FALSE;
			}
			AppendScrollLog(time(NULL), &clr, _T("��������\n"));
			return TRUE;
		}
	}
	clr.clr = CLR_FATAL;
	AppendScrollLog(time(NULL), &clr, _T("δ�ҵ�Ҫ���ӵķ�����\n"));
	return TRUE;
}

// CMainFrame ��Ϣ�������

void CMainFrame::OnSetFocus(CWnd* /*pOldWnd*/)
{
	// ������ǰ�Ƶ���ͼ����
//	m_wndView.SetFocus();
}

BOOL CMainFrame::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
// 	// ����ͼ��һ�γ��Ը�����
// 	if (m_wndView.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
// 		return TRUE;

	// ����ִ��Ĭ�ϴ���
	return CFrameWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

void CMainFrame::OnConnect()
{
	NetworkMsg *pMsg = new NetworkMsg(NetworkMsg::_OnConnect);
	if (!pMsg)
		return;
	if (!m_hWnd || !IsWindow(m_hWnd)
		|| !PostMessage(WM_NET_MSG, (WPARAM)pMsg))
		delete pMsg;
}

void CMainFrame::OnConnectFailed(LPCTSTR szError)
{
	NetworkMsg *pMsg = new NetworkMsg(NetworkMsg::_OnConnectFailed);
	if (!pMsg)
		return;
	pMsg->msg1 = _tcsdup(szError);
	if (!m_hWnd || !IsWindow(m_hWnd)
		|| !PostMessage(WM_NET_MSG, (WPARAM)pMsg))
		delete pMsg;
}

void CMainFrame::OnDisconnect()
{
	NetworkMsg *pMsg = new NetworkMsg(NetworkMsg::_OnDisconnect);
	if (!pMsg)
		return;
	if (!m_hWnd || !IsWindow(m_hWnd)
		|| !PostMessage(WM_NET_MSG, (WPARAM)pMsg))
		delete pMsg;
}

void CMainFrame::OnReceive(LPCSTR szCmd, LPCSTR szContent)
{
	NetworkMsg *pMsg = new NetworkMsg(NetworkMsg::_OnReceive);
	if (!pMsg)
		return;
	pMsg->msg1 = tempA2T(szCmd, CP_ACP).Detach();
	pMsg->msg2 = tempA2T(szContent, CP_ACP).Detach();
	if (!m_hWnd || !IsWindow(m_hWnd)
		|| !PostMessage(WM_NET_MSG, (WPARAM)pMsg))
		delete pMsg;
}

void CMainFrame::OnSendComplete(void *ctx)
{
	NetworkMsg *pMsg = new NetworkMsg(NetworkMsg::_OnSendComplete);
	if (!pMsg)
		return;
	pMsg->msg1 = ctx;
	if (!m_hWnd || !IsWindow(m_hWnd)
		|| !PostMessage(WM_NET_MSG, (WPARAM)pMsg))
		delete pMsg;
}

void CMainFrame::OnException(LPCTSTR szError)
{
	NetworkMsg *pMsg = new NetworkMsg(NetworkMsg::_OnException);
	if (!pMsg)
		return;
	pMsg->msg1 = _tcsdup(szError);
	if (!m_hWnd || !IsWindow(m_hWnd)
		|| !PostMessage(WM_NET_MSG, (WPARAM)pMsg))
		delete pMsg;
}

BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext)
{
	if (!m_wndSplitter.CreateStatic(this, 1, 2))
		return FALSE;

	if (!m_wndSplitter.CreateView(0, 0, RUNTIME_CLASS(CListView), CSize(200, 0), pContext))
		return FALSE;

	if (!m_wndSplitter.CreateView(0, 1, RUNTIME_CLASS(CRichEditView), CSize(0,0), pContext))
		return FALSE;

	CRichEditCtrl &rich = ((CRichEditView*)m_wndSplitter.GetPane(0, 1))->GetRichEditCtrl();
	rich.SetReadOnly();
	InitializeLogCtrl(&rich);
	LOGFONT lf = { 0 };
	HFONT hFont;
	SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &lf, 0);
	hFont = CreateFontIndirect(&lf);
	rich.SetFont(CFont::FromHandle(hFont), TRUE);
	rich.SetEventMask(ENM_SELCHANGE);

	CListCtrl &listctrl = ((CListView*)m_wndSplitter.GetPane(0,0))->GetListCtrl();
	listctrl.SetExtendedStyle(LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES|LVS_EX_DOUBLEBUFFER);
	listctrl.ModifyStyle(LVS_SMALLICON, LVS_SINGLESEL|LVS_NOCOLUMNHEADER|LVS_REPORT);
	listctrl.InsertColumn(0, _T("���ݰ�"), LVCFMT_LEFT, 200);
	UpdateDataList();
	return TRUE;
}

void CMainFrame::OnUpdateConnectManage(CCmdUI *pCmdUI)
{
	pCmdUI->Enable();
}

void CMainFrame::OnConnectManage()
{
	m_connectMgr.DoModal();
	UpdateConnectMenu();
}

void CMainFrame::OnUpdateConnect(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_pNetwork != NULL);
}

void CMainFrame::OnUpdateConnectClose(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_pNetwork && m_pNetwork->IsConnected());
}

void CMainFrame::OnConnectServer(UINT uID)
{
	DoConnect(uID-ID_CONNECT_1);
}

LRESULT CMainFrame::OnNetworkMsg(WPARAM wParam, LPARAM lParam)
{
	NetworkMsg *pMsg = (NetworkMsg*)wParam;
	COLORINFO tbl[] = {
		{CLR_INFO, -1},
		{CLR_INFO, -1},
		{CLR_INFO, -1},
		{CLR_INFO, -1},
		{CLR_INFO, -1}
	};

	if (!pMsg)
		return 0;

	switch (pMsg->type)
	{
	case NetworkMsg::_OnConnect:
		tbl[0].clr = CLR_INFO;
		AppendScrollLog(pMsg->t, tbl, _T("������\n"));
		break;
	case NetworkMsg::_OnConnectFailed:
		tbl[0].clr = CLR_FATAL;
		AppendScrollLog(pMsg->t, tbl, _T("����ʧ�ܣ�%s\n"), pMsg->msg1);
		break;
	case NetworkMsg::_OnDisconnect:
		tbl[0].clr = CLR_FATAL;
		AppendScrollLog(pMsg->t, tbl, _T("�����ѶϿ�\n"));
		break;
	case NetworkMsg::_OnReceive:
		tbl[0].clr = CLR_INFO;
		tbl[0].length = 5;
		tbl[1].clr = CLR_CTT;
		tbl[1].length = _tcslen((LPCTSTR)pMsg->msg1);
		tbl[2].clr = CLR_INFO;
		tbl[2].length = 3;
		tbl[3].clr = CLR_CTT;
		tbl[3].length = _tcslen((LPCTSTR)pMsg->msg2);
		tbl[4].clr = CLR_INFO;
		AppendScrollLog(pMsg->t, tbl, _T("�յ���Ϣ[%s]:{%s}\n"), pMsg->msg1,
			pMsg->msg2);
		break;
	case NetworkMsg::_OnSendComplete:
		tbl[0].clr = CLR_CTT;
		tbl[0].length = _sctprintf(_T("[%ld]"), (LONG)pMsg->msg1);
		tbl[1].clr = CLR_INFO;
		AppendScrollLog(pMsg->t, tbl, _T("[%ld]�������\n"), (LONG)pMsg->msg1);
		break;
	case NetworkMsg::_OnException:
		tbl[0].clr = CLR_FATAL;
		AppendScrollLog(pMsg->t, tbl, _T("�쳣> %s\n"), pMsg->msg1);
	}

	delete pMsg;
	return 0;
}

void CMainFrame::OnCls()
{
	m_pCtrl->SetSel(0, -1);
	m_pCtrl->ReplaceSel(_T(""));
}

void CMainFrame::OnData()
{
	m_dataDlg.DoModal();
	UpdateDataList();
}

void CMainFrame::OnLogSelChange(LPNMHDR hdr, LRESULT *plResult)
{
	LONG lBegin, lEnd;
	m_pCtrl->GetSel(lBegin, lEnd);
	CString str;
	str.Format(_T("%ld"), lBegin);
	m_wndStatusBar.SetPaneText(1, str);
	str.Format(_T("%ld"), lEnd - lBegin);
	m_wndStatusBar.SetPaneText(2, str);
}

void CMainFrame::OnNMClickData(int nItem)
{
	COLORINFO clr[] = {
		{CLR_CTT, -1},
		{CLR_INFO, 4},
		{CLR_CTT, -1},
		{CLR_INFO, 2},
		{CLR_CTT, -1},
		{CLR_INFO, -1}
	};

	if (nItem == -1)
		return;

	if (!m_pNetwork)
	{
		clr[0].clr = CLR_FATAL;
		AppendScrollLog(time(NULL), clr, _T("����ģ��δ׼������,���ܷ���\n"));
		return;
	}

	if (!m_pNetwork->IsConnected())
	{
		clr[0].clr = CLR_FATAL;
		AppendScrollLog(time(NULL), clr, _T("δ���ӷ�����,���ܷ���\n"));
		return;
	}

	CString str = ((CListView*)m_wndSplitter.GetPane(0,0))->
		GetListCtrl().GetItemText(nItem, 0);

	CString cmd, content;
	if (!m_dataDlg.GetCommand(str, cmd, content))
	{
		clr[0].clr = CLR_FATAL;
		AppendScrollLog(time(NULL), clr, _T("δ�ҵ���Ϊ%s�����ݰ�\n"), (LPCTSTR)str);
		return;
	}

	LONG lIndex = InterlockedIncrement(&m_lPkgIndex);

	CString strIndex;
	strIndex.Format(_T("[%ld]"), lIndex);
	clr[0].length = strIndex.GetLength();
	clr[2].length = cmd.GetLength();
	clr[4].length = content.GetLength();

	AppendScrollLog(time(NULL), clr, _T("%s������<%s>{%s}\n"),
		(LPCTSTR)strIndex, (LPCTSTR)cmd, (LPCTSTR)content);

	if (!m_pNetwork->SendEx((void*)lIndex, tempT2A(cmd), tempT2A(content)))
	{
		clr[1].clr = CLR_FATAL;
		clr[1].length = -1;
		AppendScrollLog(time(NULL), clr, _T("%s����ʧ��\n"), (LPCTSTR)strIndex);
	}
}

void CMainFrame::OnConnectClose()
{
	m_pNetwork->Close();
}
