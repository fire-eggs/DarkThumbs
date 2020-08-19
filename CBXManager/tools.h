#ifndef _TOOLS_C804F48E_5335_49D0_B277_FAB445845DF3_
#define _TOOLS_C804F48E_5335_49D0_B277_FAB445845DF3_

#include <windows.h>
#include <stdio.h>

//dialog drag support
template <class T> class CDialogDrag
{
public:
		BEGIN_MSG_MAP(CDialogDrag<T>)
			MESSAGE_HANDLER(WM_NCHITTEST, OnNCHitTest)
		END_MSG_MAP()

	LRESULT OnNCHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		T* pT=static_cast<T*>(this);
		m_lHitTest = ::DefWindowProcW(pT->m_hWnd, uMsg, wParam, lParam);
		if (m_lHitTest == HTCLIENT) return HTCAPTION;
	return m_lHitTest;
	}
private:
	LRESULT m_lHitTest;
};
#define CDragDialog CDialogDrag;

//************************************************************************
#include <shellapi.h>
#pragma comment(lib, "shell32.lib")
template <class T> class CDropFileTarget // implements dropfile target window
{
public:
	BEGIN_MSG_MAP(CDropFileTarget<T>)
		MESSAGE_HANDLER(WM_DROPFILES, OnDropFiles) 
	END_MSG_MAP()

	void RegisterDropTarget(BOOL bAccept = TRUE)
	{
		T* pT=static_cast<T*>(this);
		ATLASSERT(::IsWindow(pT->m_hWnd));
		::DragAcceptFiles(pT->m_hWnd, bAccept);
	}

	//override in inherited class
protected:
	virtual BOOL StartDropFile(UINT uNumFiles){ATLTRACE("CDropFileTarget::StartDropFile NumFiles: %d\n",uNumFiles);return TRUE;}
	virtual BOOL ProcessDropFile(LPCTSTR szDropFile, UINT nFile){ATLTRACE(_T("CDropFileTarget::ProcessDropFile: %s\n"),szDropFile);return TRUE;}
	virtual void FinishDropFile(){ATLTRACE("CDropFileTarget::FinishDropFile\n");}

private:
	virtual LRESULT OnDropFiles(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		UINT i;
		UINT uMaxLen=0;
		UINT uNumFiles = ::DragQueryFile((HDROP)wParam, 0xFFFFFFFF, NULL, 0);
		if (!StartDropFile(uNumFiles)) return 0;

		for (i=0; i<uNumFiles; i++)
			uMaxLen=max(uMaxLen, ::DragQueryFile((HDROP)wParam, i, NULL, 0));

		uMaxLen++;//DragQueryFile doesn't include terminating null
		TCHAR* szFilename=(TCHAR*)::CoTaskMemAlloc(sizeof(TCHAR)*uMaxLen);

		for (i=0; i<uNumFiles; i++)
		{
			::DragQueryFile((HDROP)wParam, i, szFilename, uMaxLen);
			if (!ProcessDropFile(szFilename, i)) break;
		}

		::CoTaskMemFree(szFilename);
		::DragFinish((HDROP)wParam);
		FinishDropFile();
	return 0;
	}
};


////*******************************unused*****************************
//class CSingleInstance
//{
//protected:
//  BOOL  m_instantiated;
//  HANDLE m_hMutex;
//public:
//  CSingleInstance(TCHAR *strMutexName)
//  {
//    m_hMutex = CreateMutex(NULL, FALSE, strMutexName); //do early
//    m_instantiated=(ERROR_ALREADY_EXISTS == GetLastError()); //save for use later...
//  }
//  virtual ~CSingleInstance() 
//  {
//    if (m_hMutex)  //Do not forget to close handles.
//    {
//       CloseHandle(m_hMutex); //Do as late as possible.
//       m_hMutex = NULL; //Good habit to be in.
//    }
//  }
//  BOOL IsAnotherInstanceRunning(){return m_instantiated;}
//};

//************************************************************************

//window snap to screen edges
#ifdef __ATLBASE_H__
template <class T> class CSnapWindow
#else
class CSnapWindow
#endif
{
public:
       int snap_Margin, snap_ModifierKey;

#ifdef __ATLBASE_H__
	BEGIN_MSG_MAP(CSnapWindow<T>)
		MESSAGE_HANDLER(WM_MOVING, OnSnapMoving)
		MESSAGE_HANDLER(WM_ENTERSIZEMOVE, OnSnapEnterSizeMove)
	END_MSG_MAP()
#endif

#ifdef __ATLBASE_H__                                                                 
	virtual LRESULT OnSnapEnterSizeMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
#else
	virtual LRESULT OnSnapEnterSizeMove(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
#endif
					{
						snap_cur_pos.x=0;
						snap_cur_pos.y=0;
						snap_rcWindow.bottom=0;
						snap_rcWindow.left=0;
						snap_rcWindow.right=0;
						snap_rcWindow.top=0;

					#ifdef __ATLBASE_H__ 
						T* pT = static_cast<T*>(this);
						GetWindowRect(pT->m_hWnd, &snap_rcWindow );
					#else
						GetWindowRect(hWnd, &snap_rcWindow );
					#endif

						GetCursorPos( &snap_cur_pos );
						snap_x = snap_cur_pos.x - snap_rcWindow.left;
						snap_y = snap_cur_pos.y - snap_rcWindow.top;
					return 0;
					}

#ifdef __ATLBASE_H__ 
	virtual LRESULT OnSnapMoving(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
#else
	virtual LRESULT OnSnapMoving(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
#endif
					{
						//no snap if shift key pressed
						if (GetAsyncKeyState(snap_ModifierKey) < 0) return FALSE;
						snap_prc = (LPRECT)lParam;
						snap_cur_pos.x=0;
						snap_cur_pos.y=0;
						snap_rcWindow.bottom=0;
						snap_rcWindow.left=0;
						snap_rcWindow.right=0;
						snap_rcWindow.top=0;

						GetCursorPos( &snap_cur_pos );
						OffsetRect( snap_prc,
							snap_cur_pos.x - (snap_prc->left + snap_x) ,
							snap_cur_pos.y - (snap_prc->top  + snap_y) );
						//it may change during app lifetime
						SystemParametersInfo( SPI_GETWORKAREA, 0, &snap_wa, 0 );

						if (isSnapClose( snap_prc->left, snap_wa.left ))
							OffsetRect( snap_prc, snap_wa.left - snap_prc->left, 0);
						else 
							if (isSnapClose( snap_wa.right, snap_prc->right ))
								OffsetRect( snap_prc, snap_wa.right - snap_prc->right, 0);

						if (isSnapClose( snap_prc->top, snap_wa.top ))
							OffsetRect( snap_prc, 0, snap_wa.top - snap_prc->top );
						else 
							if (isSnapClose( snap_wa.bottom, snap_prc->bottom ))
								OffsetRect( snap_prc, 0, snap_wa.bottom - snap_prc->bottom );
					return TRUE;
					}

	virtual BOOL isSnapClose( int a, int b ) { return (::abs( a - b ) < snap_Margin);}

	CSnapWindow()
		{
			snap_ModifierKey=VK_SHIFT;
			NONCLIENTMETRICS ncm;
			SecureZeroMemory(&ncm, sizeof(NONCLIENTMETRICS));
			ncm.cbSize = sizeof ncm;
			SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);
			snap_Margin=ncm.iCaptionHeight;
		}
private:
	POINT snap_cur_pos;
	RECT snap_rcWindow, snap_wa, *snap_prc;
	int snap_x, snap_y;
};
//************************************************************************

//hi-res timer class
class CTimer
{
public:
	virtual ~CTimer(){}
	CTimer()
	{
		m_msSystemUpTime=0;
		m_ticksPerSecond.QuadPart=-1;
		m_startTick.QuadPart=0;
		m_count.QuadPart=0;
		::QueryPerformanceFrequency(&m_ticksPerSecond);
		::QueryPerformanceCounter(&m_startTick);
		m_msSystemUpTime = 1000*m_startTick.QuadPart/m_ticksPerSecond.QuadPart;
	}

	__int64 msElapsed()//elapsed time in milliseconds
	{
		::QueryPerformanceCounter(&m_count);
		__int64 m_elapsed=1000*(m_count.QuadPart - m_startTick.QuadPart)/m_ticksPerSecond.QuadPart;
	return m_elapsed;
	}

	BOOL resetTimer(){ return ::QueryPerformanceCounter(&m_startTick); }

private:
	__int64 m_msSystemUpTime;
	LARGE_INTEGER m_ticksPerSecond;
	LARGE_INTEGER m_startTick;
	LARGE_INTEGER m_count;
};
//************************************************************************

//#ifdef USES_HTMLHELP
// handles F1 or dialog context help
//#include "resource.h"
//#include "Htmlhelp.h"
//#pragma comment(lib,"Htmlhelp.lib")
template <class T> class CDialogHelp
{
public:
	BEGIN_MSG_MAP(CDialogHelp<T>)
		MESSAGE_HANDLER(WM_HELP, OnAppHelpImpl)
	END_MSG_MAP()

	LRESULT OnAppHelpImpl(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
	{
		T* pT=static_cast<T*>(this);
	return pT->OnAppHelp((HELPINFO *)lParam);
	}

	virtual LRESULT OnAppHelp(LPHELPINFO lphi)
	{
		ATLTRACE("CDialogHelp::OnAppHelp\n");
	return 0;
	}
};
//#endif


#endif//_TOOLS_C804F48E_5335_49D0_B277_FAB445845DF3_
