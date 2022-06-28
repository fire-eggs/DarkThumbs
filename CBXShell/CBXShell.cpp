// CBXshell.cpp : Implementation of DLL Exports.

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include "CBXshell.h"

#include "CBXShell_i.c"
#include "CBXShellClass.h"


CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_CBXShell, CCBXShell)
END_OBJECT_MAP()

HICON zipIcon = NULL;

void __cdecl logit(LPCWSTR format, ...)
{
	wchar_t buf[4096];
	wchar_t* p = buf;
	va_list args;
	int n;

	va_start(args, format);
	n = _vsnwprintf(p, sizeof(buf) - 3, format, args);
	va_end(args);

	p += (n < 0) ? sizeof buf - 3 : n;

	while (p > buf && isspace(p[-1]))
		*--p = '\0';

	*p++ = '\r';
	*p++ = '\n';
	*p = '\0';

	OutputDebugStringW(buf);
}

HINSTANCE _hInstance;

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point
extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	_hInstance = hInstance;
    if (dwReason == DLL_PROCESS_ATTACH)
    {
		zipIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
		logit(_T("DT:attach"));
        _Module.Init(ObjectMap, hInstance, &LIBID_CBXSHELLLib);
        DisableThreadLibraryCalls(hInstance);

    }
    else
	if (dwReason == DLL_PROCESS_DETACH)
	{
        logit(_T("DT:detach"));
        _Module.Term();
	}
return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE
STDAPI DllCanUnloadNow(void)
{
    //return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
	return S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Adds entries to the system registry
STDAPI DllRegisterServer(void)
{
    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// Removes entries from the system registry
STDAPI DllUnregisterServer(void)
{
    return _Module.UnregisterServer(TRUE);
}
