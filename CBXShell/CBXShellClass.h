#ifndef _CBXSHELLCLASS_541926D5_D807_4CCB_9F35_8464657CC196_
#define _CBXSHELLCLASS_541926D5_D807_4CCB_9F35_8464657CC196_
#pragma once

#include "resource.h"       // main symbols

#include "cbxArchive.h"

/////////////////////////////////////////////////////////////////////////////
// CCBXShell

class ATL_NO_VTABLE CCBXShell :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CCBXShell,&CLSID_CBXShell>,
	public IDispatchImpl<ICBXShell, &IID_ICBXShell, &LIBID_CBXSHELLLib>, 
	public IPersistFile,
	public IExtractImage2,
	public IQueryInfo
{
public:
	HRESULT FinalConstruct(void);
	void FinalRelease(void);

	BEGIN_COM_MAP(CCBXShell)
		COM_INTERFACE_ENTRY(ICBXShell)
		COM_INTERFACE_ENTRY(IPersistFile)
		COM_INTERFACE_ENTRY(IQueryInfo)
		COM_INTERFACE_ENTRY(IExtractImage)
		COM_INTERFACE_ENTRY(IExtractImage2)
		COM_INTERFACE_ENTRY(IDispatch)
	END_COM_MAP()


	DECLARE_REGISTRY_RESOURCEID(IDR_CBXShell)
	DECLARE_PROTECT_FINAL_CONSTRUCT()

	// IPersistFile
	STDMETHOD(Load)(LPCOLESTR wszFile, DWORD dwMode) { return m_cbx.OnLoad(wszFile); }
	STDMETHOD(GetClassID)(LPCLSID clsid){return E_NOTIMPL;}
	STDMETHOD(IsDirty)(VOID){return E_NOTIMPL;}
	STDMETHOD(Save)(LPCOLESTR, BOOL){return E_NOTIMPL;}
	STDMETHOD(SaveCompleted)(LPCOLESTR){return E_NOTIMPL;}
	STDMETHOD(GetCurFile)(LPOLESTR FAR*){return E_NOTIMPL;}

	// IExtractImage/IExtractImage2
	STDMETHOD(GetLocation)(LPWSTR pszPathBuffer, DWORD cchMax,
							DWORD *pdwPriority, const SIZE *prgSize,
							DWORD dwRecClrDepth, DWORD *pdwFlags) { return m_cbx.OnGetLocation(prgSize, pdwFlags); }
	STDMETHOD(Extract)(HBITMAP* phBmpThumbnail) { return m_cbx.OnExtract(phBmpThumbnail); }
	// IExtractImage2
	STDMETHOD(GetDateStamp)(FILETIME *pDateStamp) { return m_cbx.OnGetDateStamp(pDateStamp);}

	// IQueryInfo
	STDMETHOD(GetInfoTip)(DWORD dwFlags, LPWSTR* ppwszTip) { return m_cbx.OnGetInfoTip(ppwszTip);}
	STDMETHOD(GetInfoFlags)(LPDWORD pdwFlags) { *pdwFlags = 0; return E_NOTIMPL;}

private:
	__cbx::CCBXArchive m_cbx;
};


#endif//_CBXSHELLCLASS_541926D5_D807_4CCB_9F35_8464657CC196_
