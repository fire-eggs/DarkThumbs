// MainDlg.cpp : implementation of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "about.h"
#include "MainDlg.h"

extern HINSTANCE _hInstance;

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

HWND CMainDlg::CreateToolTip(int toolID, LPWSTR pszText)
{
	if (!toolID || !pszText)
	{
		return FALSE;
	}
	// Get the window of the tool.
	HWND hwndTool = GetDlgItem(toolID);

	// Create the tooltip. g_hInst is the global instance handle.
	HWND hwndTip = CreateWindowEx(NULL, TOOLTIPS_CLASS, NULL,
		WS_POPUP | TTS_ALWAYSTIP | TTS_BALLOON,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		m_hWnd, NULL,
		_hInstance, NULL);

	if (!hwndTool || !hwndTip)
	{
		return (HWND)NULL;
	}

	// Associate the tooltip with the tool.
	TOOLINFO toolInfo = { 0 };
	toolInfo.cbSize = sizeof(toolInfo);
	toolInfo.hwnd = m_hWnd;
	toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
	toolInfo.uId = (UINT_PTR)hwndTool;
	toolInfo.lpszText = pszText;
	SendMessage(hwndTip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);

	return hwndTip;
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
	Button_SetCheck(GetDlgItem(IDC_CB_FB),   m_reg.HasTH(CBX_FB2));

	Button_SetCheck(GetDlgItem(IDC_CB_SHOWICON), m_reg.IsShowIconOpt());//CBX_SHOWICON
	Button_SetCheck(GetDlgItem(IDC_CB_SORT), m_reg.IsSortOpt());//CBX_SORT
	Button_SetCheck(GetDlgItem(IDC_CB_SKIP), m_reg.IsSkipOpt()); // V1.7 skip scanlation files
	Button_SetCheck(GetDlgItem(IDC_CB_COVER), m_reg.IsCoverOpt()); // V1.7 prefer cover file

	// TODO these should come from the resource file
	// V1.7 Tooltips 
	CreateToolTip(IDC_CB_SHOWICON, L"Display a ZIP icon top-left of thumbnail to let you know it's an archive.");
	CreateToolTip(IDC_CB_SKIP,  L"Ignore common 'scanlation' files: 'credit', 'recruit', etc.");
	CreateToolTip(IDC_CB_COVER, L"Prefer an image file with 'cover' in the name. Takes effect only if sorting is ON.");
	CreateToolTip(IDC_CB_SORT,  L"If OFF, uses the first image in the archive. If ON, uses the first alphabetical image.");
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
	// V1.7 skip scanlation files
	bRet = (BST_CHECKED == Button_GetCheck(GetDlgItem(IDC_CB_SKIP)));
	if (bRet != m_reg.IsSkipOpt())
	{
		bRefresh = TRUE;
		m_reg.SetSkipOpt(bRet);
	}
	// V1.7 prefer 'cover'
	bRet = (BST_CHECKED == Button_GetCheck(GetDlgItem(IDC_CB_COVER)));
	if (bRet != m_reg.IsCoverOpt())
	{
		bRefresh = TRUE;
		m_reg.SetCoverOpt(bRet);
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
	if (bRet != m_reg.HasTH(CBX_AZW))
	{
		bRefresh = TRUE;
		m_reg.SetHandlers(CBX_AZW, bRet);
	}
	if (bRet != m_reg.HasTH(CBX_AZW3))
	{
		bRefresh = TRUE;
		m_reg.SetHandlers(CBX_AZW3, bRet);
	}
	bRet = (BST_CHECKED == Button_GetCheck(GetDlgItem(IDC_CB_FB)));
	if (bRet != m_reg.HasTH(CBX_FB2))
	{
		bRefresh = TRUE;
		m_reg.SetHandlers(CBX_FB2, bRet);
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
