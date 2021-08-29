// MainDlg.cpp : implementation of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "about.h"
#include "MainDlg.h"


BOOL CMainDlg::PreTranslateMessage(MSG* pMsg) { return CWindow::IsDialogMessage(pMsg); }
BOOL CMainDlg::OnIdle() { return FALSE;}

LRESULT CMainDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// center the dialog on the screen
	CenterWindow();

	// set icons
	HICON hIcon = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME), 
		IMAGE_ICON, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
	SetIcon(hIcon, TRUE);
	HICON hIconSmall = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME), 
		IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
	SetIcon(hIconSmall, FALSE);

	// register object for message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	UIAddChildWindowContainer(m_hWnd);

	//add 'About' item to sys menu
	ATLASSERT(IDC_APPABOUT < 0xF000);
	CMenu pSysMenu=GetSystemMenu(FALSE);
	if (!pSysMenu.IsNull())
	{
		pSysMenu.AppendMenu(MF_SEPARATOR);
		pSysMenu.AppendMenu(MF_STRING, IDC_APPABOUT, _T("About"));
	}

	InitUI();

	//set focus to Cancel btn
	GotoDlgCtrl(GetDlgItem(IDCANCEL));
return FALSE;
}

void CMainDlg::InitUI()
{
	//Button_GetCheck   BST_CHECKED : BST_UNCHECKED equals TRUE: FALSE
	Button_SetCheck(GetDlgItem(IDC_CB_ZIP),  m_reg.HasTH(CBX_ZIP));
	Button_SetCheck(GetDlgItem(IDC_CB_EPUB), m_reg.HasTH(CBX_EPUB));
	Button_SetCheck(GetDlgItem(IDC_CB_CBZ),  m_reg.HasTH(CBX_CBZ));
	Button_SetCheck(GetDlgItem(IDC_CB_RAR),  m_reg.HasTH(CBX_RAR));
	Button_SetCheck(GetDlgItem(IDC_CB_CBR),  m_reg.HasTH(CBX_CBR));
	Button_SetCheck(GetDlgItem(IDC_CB_MOBI), m_reg.HasTH(CBX_MOBI));
	Button_SetCheck(GetDlgItem(IDC_CB_SHOWICON), m_reg.IsShowIconOpt());//CBX_SHOWICON
	Button_SetCheck(GetDlgItem(IDC_CB_SORT), m_reg.IsSortOpt());//CBX_SORT
}

//check ui state,compare to registry state->if!= refresh
void CMainDlg::OnApplyImpl()
{
	BOOL bRet, bRefresh=FALSE;

	//sort option
	bRet=(BST_CHECKED==Button_GetCheck(GetDlgItem(IDC_CB_SORT)));
	if (bRet!=m_reg.IsSortOpt())
	{
		bRefresh=TRUE;
		m_reg.SetSortOpt(bRet);
	}
	//show archive icon
	bRet = (BST_CHECKED == Button_GetCheck(GetDlgItem(IDC_CB_SHOWICON)));
	if (bRet != m_reg.IsShowIconOpt())
	{
		bRefresh = TRUE;
		m_reg.SetShowIconOpt(bRet);
	}
	//thumbnail handlers
	bRet=(BST_CHECKED==Button_GetCheck(GetDlgItem(IDC_CB_ZIP)));
	if (bRet!=m_reg.HasTH(CBX_ZIP))
	{
		bRefresh=TRUE;
		m_reg.SetHandlers(CBX_ZIP, bRet);
	}
	bRet = (BST_CHECKED == Button_GetCheck(GetDlgItem(IDC_CB_EPUB)));
	if (bRet != m_reg.HasTH(CBX_EPUB))
	{
		bRefresh = TRUE;
		m_reg.SetHandlers(CBX_EPUB, bRet);
	}
	bRet=(BST_CHECKED==Button_GetCheck(GetDlgItem(IDC_CB_CBZ)));
	if (bRet!=m_reg.HasTH(CBX_CBZ))
	{
		bRefresh=TRUE;
		m_reg.SetHandlers(CBX_CBZ, bRet);
	}
	bRet=(BST_CHECKED==Button_GetCheck(GetDlgItem(IDC_CB_RAR)));
	if (bRet!=m_reg.HasTH(CBX_RAR))
	{
		bRefresh=TRUE;
		m_reg.SetHandlers(CBX_RAR, bRet);
	}
	bRet=(BST_CHECKED==Button_GetCheck(GetDlgItem(IDC_CB_CBR)));
	if (bRet!=m_reg.HasTH(CBX_CBR))
	{
		bRefresh=TRUE;
		m_reg.SetHandlers(CBX_CBR, bRet);
	}
	bRet = (BST_CHECKED == Button_GetCheck(GetDlgItem(IDC_CB_MOBI)));
	if (bRet != m_reg.HasTH(CBX_MOBI))
	{
		bRefresh = TRUE;
		m_reg.SetHandlers(CBX_MOBI, bRet);
	}

	if (bRefresh)
	{
		ATLTRACE("refreshing FS\n");
		SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST | SHCNF_FLUSHNOWAIT | SHCNF_NOTIFYRECURSIVE, NULL, NULL);
	}
	
	InitUI();//reload
}

LRESULT CMainDlg::OnApply(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	OnApplyImpl();
return 0;
}

LRESULT CMainDlg::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	OnApplyImpl();
	CloseDialog(wID);
return 0;
}


LRESULT CMainDlg::OnSysCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if (wParam!=IDC_APPABOUT) return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
	CAboutDlg _a;
	_a.DoModal(m_hWnd);
return 0;
}

LRESULT CMainDlg::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// unregister message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->RemoveMessageFilter(this);
	pLoop->RemoveIdleHandler(this);
return 0;
}

LRESULT CMainDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CloseDialog(wID);
return 0;
}

void CMainDlg::CloseDialog(int nVal)
{
	DestroyWindow();
	::PostQuitMessage(nVal);
}

LRESULT CMainDlg::OnAppHelp(LPHELPINFO lphi)
{
	switch (lphi->iCtrlId)
	{
		case IDC_TH_GROUP:
		case IDC_CB_CBZ:
		case IDC_CB_EPUB:
		case IDC_CB_CBR:
		case IDC_CB_ZIP:
		case IDC_CB_RAR:
		case IDC_CB_MOBI:
			// '#' anchors must use id attribute
			HtmlHelp(m_hWnd, _T("CBXShellHelp.chm::manager.html#optth"), HH_DISPLAY_TOPIC, NULL);
		break;

		case IDC_SORT_ADVOPTGROUP:
			HtmlHelp(m_hWnd, _T("CBXShellHelp.chm::manager.html#advopt"), HH_DISPLAY_TOPIC, NULL);
		break;

		case IDC_CB_SORT:
		case IDC_SORT_DESC:
			ATLTRACE("HH sort opt\n");
			HtmlHelp(m_hWnd, _T("CBXShellHelp.chm::FAQ.html#custth"), HH_DISPLAY_TOPIC, NULL);
		break;

	default:
			ATLTRACE("HH default\n");
			HtmlHelp(m_hWnd, _T("CBXShellHelp.chm::manager.html"), HH_DISPLAY_TOPIC, NULL);//about?
		break;
	}
return 0;
}
