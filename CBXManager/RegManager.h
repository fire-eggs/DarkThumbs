#ifndef _REGMANAGER_79AE66E4_84E2_45A1_BF4F_43AA714BE55F_
#define _REGMANAGER_79AE66E4_84E2_45A1_BF4F_43AA714BE55F_
#pragma once

#include <windows.h>
#pragma comment(lib,"advapi32.lib")
#include <shlwapi.h>
#pragma comment(lib,"shlwapi.lib")


#define CBX_MUTEX_GLOBAL	L"Global\\{64DEE47D-9669-4430-9D5C-304867F87B51}"
#define CBX_MGRMUTEX		L"Local\\{50D9CBE6-C168-4901-8CC9-2A7C97E558F7}"


#define CBX_GUID_KEY _T("{9E6ECB90-5A61-42BD-B851-D3297D9C7F39}") //38+1 TCHAR
#define CBX_GUID_KEY_SLEN 39
#define CBX_APP_KEY _T("Software\\DarkThumbs\\{9E6ECB90-5A61-42BD-B851-D3297D9C7F39}")

// per-user settings (HKCU)
// thumbnail handler keys
#define CBX_ZIPTH_KEY   _T("SOFTWARE\\Classes\\.ZIP\\shellex\\{BB2E617C-0920-11d1-9A0B-00C04FC2D6C1}")
#define CBX_CBZTH_KEY   _T("SOFTWARE\\Classes\\.CBZ\\shellex\\{BB2E617C-0920-11d1-9A0B-00C04FC2D6C1}")
#define CBX_RARTH_KEY   _T("SOFTWARE\\Classes\\.RAR\\shellex\\{BB2E617C-0920-11d1-9A0B-00C04FC2D6C1}")
#define CBX_CBRTH_KEY   _T("SOFTWARE\\Classes\\.CBR\\shellex\\{BB2E617C-0920-11d1-9A0B-00C04FC2D6C1}")
#define CBX_EPUBTH_KEY  _T("SOFTWARE\\Classes\\.EPUB\\shellex\\{BB2E617C-0920-11d1-9A0B-00C04FC2D6C1}")
#define CBX_MOBITH1_KEY _T("SOFTWARE\\Classes\\.MOBI\\shellex\\{BB2E617C-0920-11d1-9A0B-00C04FC2D6C1}")
#define CBX_MOBITH2_KEY _T("SOFTWARE\\Classes\\.AZW\\shellex\\{BB2E617C-0920-11d1-9A0B-00C04FC2D6C1}")
#define CBX_MOBITH3_KEY _T("SOFTWARE\\Classes\\.AZW3\\shellex\\{BB2E617C-0920-11d1-9A0B-00C04FC2D6C1}")
#define CBX_FBTH_KEY    _T("SOFTWARE\\Classes\\.FB2\\shellex\\{BB2E617C-0920-11d1-9A0B-00C04FC2D6C1}")

// infotip handler keys
#define CBX_ZIPIH_KEY _T("SOFTWARE\\Classes\\.ZIP\\shellex\\{00021500-0000-0000-C000-000000000046}")
#define CBX_CBZIH_KEY _T("SOFTWARE\\Classes\\.CBZ\\shellex\\{00021500-0000-0000-C000-000000000046}")
#define CBX_RARIH_KEY _T("SOFTWARE\\Classes\\.RAR\\shellex\\{00021500-0000-0000-C000-000000000046}")
#define CBX_CBRIH_KEY _T("SOFTWARE\\Classes\\.CBR\\shellex\\{00021500-0000-0000-C000-000000000046}")
#define CBX_EPUBIH_KEY _T("SOFTWARE\\Classes\\.EPUB\\shellex\\{00021500-0000-0000-C000-000000000046}")

// cbx types
#define CBX_NONE 0
#define CBX_ZIP 1
#define CBX_CBZ 2
#define CBX_RAR 3
#define CBX_CBR 4
#define CBX_EPUB 5
#define CBX_MOBI 6
#define CBX_AZW 7
#define CBX_AZW3 8
#define CBX_FB2  9
//#define CBX_SORT 5

#define SORT_KEY L"NoSort"
#define SKIP_KEY L"SkipScanlation"
#define COVER_KEY L"PreferCover"
#define ICON_KEY L"ShowIcon"

class CRegManager
{
public:
	//CRegManager(void){}
	//virtual ~CRegManager(void){}
public:

	BOOL IsOption(const wchar_t* optionKey)
	{
		DWORD d;
		CRegKey rk;
		if (ERROR_SUCCESS == rk.Open(HKEY_CURRENT_USER, CBX_APP_KEY, KEY_READ))
		{
			if (ERROR_SUCCESS == rk.QueryDWORDValue(optionKey, d))
				return (d == 1);
		}
		return FALSE;
	}

	void SetOption(const wchar_t* optionKey, BOOL val)
	{
		CRegKey rk;
		if (ERROR_SUCCESS == rk.Create(HKEY_CURRENT_USER, CBX_APP_KEY, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE))
			rk.SetDWORDValue(optionKey, (DWORD)(val ? TRUE : FALSE));
	}

	///////////////
	// sort option. The registry key is "backward", as it records the "NoSort" state.
	BOOL IsSortOpt() { return !IsOption(SORT_KEY); }
	void SetSortOpt(BOOL bSort) { SetOption(SORT_KEY, !bSort); }

	//////////////
	// show archive Icon
	BOOL IsShowIconOpt() { return IsOption(ICON_KEY); }
	void SetShowIconOpt(BOOL bShowIcon) { SetOption(ICON_KEY, bShowIcon); }

	///////////////
	// V1.7 skip option
	BOOL IsSkipOpt() { return IsOption(SKIP_KEY); }
	void SetSkipOpt(BOOL val) { SetOption(SKIP_KEY, val); }

	///////////////
	// V1.7 cover option
	BOOL IsCoverOpt() { return IsOption(COVER_KEY); }
	void SetCoverOpt(BOOL val) { SetOption(COVER_KEY, val); }

	///////////////////////////////
	// check for thumbnail handlers
	BOOL HasTH(int cbxType)
	{
		ATLASSERT(cbxType>CBX_NONE);
		ULONG n=CBX_GUID_KEY_SLEN;
		TCHAR s[CBX_GUID_KEY_SLEN];

		CRegKey rk;
		if (ERROR_SUCCESS==rk.Open(HKEY_CURRENT_USER, GetTHKeyName(cbxType), KEY_READ))
		{
			if (ERROR_SUCCESS==rk.QueryStringValue(NULL, s, &n))
			{
				if (0==StrCmpI(s, CBX_GUID_KEY))
					return TRUE;
			}
		}
	return FALSE;
	}

	/////////////////////////////
	// check for infotip handlers
	BOOL HasIH(int cbxType)
	{
		ATLASSERT(cbxType>CBX_NONE);
		ULONG n=CBX_GUID_KEY_SLEN;
		TCHAR s[CBX_GUID_KEY_SLEN];

		CRegKey rk;
		if (ERROR_SUCCESS==rk.Open(HKEY_CURRENT_USER, GetIHKeyName(cbxType), KEY_READ))
		{
			if (ERROR_SUCCESS==rk.QueryStringValue(NULL, s, &n))
			{
				if (StrCmpI(s, CBX_GUID_KEY)==0)
					return TRUE;
			}
		}
	return FALSE;
	}

	/////////////////////////
	// set thumbnail / infotip handlers
	void SetHandlers(int cbxType, BOOL bSet)
	{
		ATLASSERT(cbxType>CBX_NONE);

		if (bSet)
		{
			//thumbnail
			CRegKey rkt, rki;
			if (ERROR_SUCCESS==rkt.Create(HKEY_CURRENT_USER, GetTHKeyName(cbxType), NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE))
				rkt.SetStringValue(NULL, CBX_GUID_KEY);
			//infotip
			if (ERROR_SUCCESS==rki.Create(HKEY_CURRENT_USER, GetIHKeyName(cbxType), NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE))
				rki.SetStringValue(NULL, CBX_GUID_KEY);
		}
		else
		{
			//thumbnail
			if (HasTH(cbxType)) RegDeleteKey(HKEY_CURRENT_USER, GetTHKeyName(cbxType));
			//infotip
			if (HasIH(cbxType)) RegDeleteKey(HKEY_CURRENT_USER, GetIHKeyName(cbxType));
		}
	}

	//get handler reg key names
	LPCTSTR GetTHKeyName(int cbxType)
	{
		ATLASSERT(cbxType>CBX_NONE);
		switch (cbxType)
		{
		case CBX_ZIP: return CBX_ZIPTH_KEY;
		case CBX_CBZ: return CBX_CBZTH_KEY;
		case CBX_EPUB: return CBX_EPUBTH_KEY;
		case CBX_RAR: return CBX_RARTH_KEY;
		case CBX_CBR: return CBX_CBRTH_KEY;
		case CBX_MOBI: return CBX_MOBITH1_KEY;
		case CBX_AZW: return CBX_MOBITH2_KEY;
		case CBX_AZW3: return CBX_MOBITH3_KEY;
		case CBX_FB2: return CBX_FBTH_KEY;
		default:break;
		}
	return NULL;
	}
	LPCTSTR GetIHKeyName(int cbxType)
	{
		ATLASSERT(cbxType>CBX_NONE);
		switch (cbxType)
		{
		case CBX_ZIP: return CBX_ZIPIH_KEY;
		case CBX_CBZ: return CBX_CBZIH_KEY;
		case CBX_EPUB: return CBX_EPUBIH_KEY;
		case CBX_RAR: return CBX_RARIH_KEY;
		case CBX_CBR: return CBX_CBRIH_KEY;
		default:break;
		}
	return NULL;
	}
};

#endif//_REGMANAGER_79AE66E4_84E2_45A1_BF4F_43AA714BE55F_
