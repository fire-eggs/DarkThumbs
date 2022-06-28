// CBXShellClass.cpp : Implementation of CCBXshellApp and DLL registration.

#include "stdafx.h"
#include "CBXShell.h"
#include "CBXShellClass.h"


HRESULT CCBXShell::FinalConstruct(void)
{
	//ATLTRACE("CCBXShell::FinalConstruct\n");
	//m_cbx.LoadRegistrySettings();

return S_OK;
}

void CCBXShell::FinalRelease(void)
{
	//ATLTRACE("CCBXShell::FinalRelease\n");
	//logit(_T("DT:final release"));
}
