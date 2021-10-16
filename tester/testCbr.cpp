#define VC_EXTRALEAN


// stdafx.h
#ifndef UNICODE
#define UNICODE// UNICODE-only project
#endif

#define STRICT

#include <atlbase.h>
#include <atlapp.h>
#include <atlcom.h>

// end stdafx.h


#include <shlObj.h>
#include <shlwapi.h>
#pragma comment(lib,"shlwapi.lib")

#include <atlimage.h>
#include <atlfile.h>

// WTL headers
#include "atlgdi.h"

#include "unrar.h"
#ifdef _WIN64
#pragma comment(lib,"unrar64.lib")
#else
#pragma comment(lib,"unrar.lib")
#endif
#include "unzip.h"

#include <string>

#define CBXMEM_MAXBUFFER_SIZE 33554432 //32mb

extern CString m_cbxFile; // Tester hack
extern BOOL m_bSort;      // Tester hack
extern SIZE m_thumbSize;  // Tester hack
extern int m_howFound;

extern BOOL IsImage(LPCTSTR szFile);
extern HBITMAP ThumbnailFromIStream(IStream* pIs, const LPSIZE pThumbSize, bool showIcon = false);

typedef const RARHeaderDataEx* LPCRARHeaderDataEx;
typedef const RAROpenArchiveDataEx* LPCRAROpenArchiveDataEx;

class CUnRar
{
public:
	CUnRar() { if (RAR_DLL_VERSION > RARGetDllVersion()) throw RAR_DLL_VERSION;	_init(); }
	virtual ~CUnRar() { Close(); _init(); }

public:
	BOOL Open(LPCTSTR rarfile, BOOL bListingOnly = TRUE, char* cmtBuf = NULL, UINT cmtBufSize = 0, char* password = NULL)
	{
		if (m_harc) return FALSE;//must close old first

		SecureZeroMemory(&m_arcinfo, sizeof(RAROpenArchiveDataEx));
#ifndef UNICODE
		m_arcinfo.ArcName = (PTCHAR)rarfile;
#else
		m_arcinfo.ArcNameW = (PTCHAR)rarfile;
#endif
		m_arcinfo.OpenMode = bListingOnly ? RAR_OM_LIST : RAR_OM_EXTRACT;
		m_arcinfo.CmtBuf = cmtBuf;
		m_arcinfo.CmtBufSize = cmtBufSize;

		m_harc = RAROpenArchiveEx(&m_arcinfo);
		if (m_harc == NULL || m_arcinfo.OpenResult != 0) return FALSE;

		if (password) RARSetPassword(m_harc, password);
		RARSetCallback(m_harc, __rarCallbackProc, (LPARAM)this);
		return TRUE;
	}

	BOOL Close() { if (m_harc) m_ret = RARCloseArchive(m_harc); return (m_ret != 0); }

	inline LPCRAROpenArchiveDataEx GetArchiveInfo() { return &m_arcinfo; }
	inline LPCRARHeaderDataEx GetItemInfo() { return &m_iteminfo; }
	inline void SetPassword(char* password) { RARSetPassword(m_harc, password); }
	inline int GetLastError() { return m_ret; }

	//inline UINT GetArchiveFlags() {return m_arcinfo.Flags;}
	//inline UINT GetItemFlags(){return m_iteminfo.Flags;}
	inline BOOL IsArchiveVolume() { return (BOOL)(m_arcinfo.Flags & 0x0001); }
	inline BOOL IsArchiveComment() { return (BOOL)(m_arcinfo.Flags & 0x0002); }
	inline BOOL IsArchiveLocked() { return (BOOL)(m_arcinfo.Flags & 0x0004); }
	inline BOOL IsArchiveSolid() { return (BOOL)(m_arcinfo.Flags & 0x0008); }
	inline BOOL IsArchivePartN() { return (BOOL)(m_arcinfo.Flags & 0x0010); }
	inline BOOL IsArchiveSigned() { return (BOOL)(m_arcinfo.Flags & 0x0020); }
	inline BOOL IsArchiveRecoveryRecord() { return (BOOL)(m_arcinfo.Flags & 0x0040); }
	inline BOOL IsArchiveEncryptedHeaders() { return (BOOL)(m_arcinfo.Flags & 0x0080); }
	inline BOOL IsArchiveFirstVolume() { return (BOOL)(m_arcinfo.Flags & 0x0100); }

	BOOL ReadItemInfo()
	{
		SecureZeroMemory(&m_iteminfo, sizeof(RARHeaderDataEx));
		m_ret = RARReadHeaderEx(m_harc, &m_iteminfo);
		return (m_ret == 0);
	}

	inline BOOL IsItemDirectory() { return ((m_iteminfo.Flags & 0x00E0) == 0x00E0); }

	inline LPCTSTR GetItemName()
	{
#ifdef UNICODE
		return (LPCTSTR)(GetItemInfo()->FileNameW);
#else
		return (LPCTSTR)CUnRar::GetItemInfo()->FileName;
#endif
	}

	//MAKEUINT64
	inline UINT64 GetItemPackedSize64() { return ((((UINT64)m_iteminfo.PackSizeHigh) << 32) | m_iteminfo.PackSize); }
	inline UINT64 GetItemUnpackedSize64() { return ((((UINT64)m_iteminfo.UnpSizeHigh) << 32) | m_iteminfo.UnpSize); }

	virtual BOOL ProcessItem()
	{
		m_ret = RARProcessFileW(m_harc, RAR_TEST, NULL, NULL);
		if (m_ret != 0) return FALSE;
		return TRUE;
	}

	virtual BOOL SkipItem()
	{
		m_ret = RARProcessFile(m_harc, RAR_SKIP, NULL, NULL);
		return (m_ret == 0);
	}

	virtual BOOL SkipItems(UINT64 si)
	{
		UINT64 _i;//0 skips?
		for (_i = 0; _i < si; _i++) { if (!ReadItemInfo() || !SkipItem()) return FALSE; }
		return TRUE;
	}

	virtual int CallbackProc(UINT msg, LPARAM UserData, LPARAM P1, LPARAM P2)
	{
		if (msg == UCM_PROCESSDATA) return ProcessItemData((LPBYTE)P1, (ULONG)P2);
		return -1;
	}

	virtual int ProcessItemData(LPBYTE pBuf, ULONG dwBufSize)
	{
		if (m_pIs)
		{
			ULONG br = 0;
			if (S_OK == m_pIs->Write(pBuf, dwBufSize, &br))
				if (br == dwBufSize) return 1;
		}
		return -1;
	}

	void SetIStream(IStream* pIs) { _ASSERTE(pIs); m_pIs = pIs; }

private:
	HANDLE m_harc;
	RARHeaderDataEx m_iteminfo;
	RAROpenArchiveDataEx m_arcinfo;
	int m_ret;
	IStream* m_pIs;
	void _init()
	{
		m_harc = NULL;
		m_pIs = NULL;
		m_ret = 0;
		SecureZeroMemory(&m_arcinfo, sizeof(RAROpenArchiveDataEx));
		SecureZeroMemory(&m_iteminfo, sizeof(RARHeaderDataEx));
	}

	//raw callback function
public:
	static int PASCAL __rarCallbackProc(UINT msg, LPARAM UserData, LPARAM P1, LPARAM P2)
	{
		CUnRar* _pc = (CUnRar*)UserData;
		return _pc->CallbackProc(msg, UserData, P1, P2);
	}
};

__int64 FindThumbnailSortRAR(LPCTSTR pszFile)
{
	CUnRar _r;
	if (!_r.Open(pszFile)) return -1;
	//skip solid (long processing time), volumes or encrypted file headers
	if (_r.IsArchiveSolid() || _r.IsArchiveVolume() || _r.IsArchiveEncryptedHeaders()) return -1;

	UINT64 _ps, _us;//my speed optimization?
	CString prevname;
	__int64 thumbindex = -1;
	__int64 i = -1;//start at none (-1)

	while (_r.ReadItemInfo())
	{
		i += 1;
		_ps = _r.GetItemPackedSize64();
		_us = _r.GetItemUnpackedSize64();

		//skip directory/emtpy file/bigger than 32mb
		if (_r.IsItemDirectory() || (_us > CBXMEM_MAXBUFFER_SIZE) || (_ps == 0) || (_us == 0))
		{
			_r.SkipItem(); continue;
		}

		//take only index of first alphabetical name
		if (IsImage(_r.GetItemName()))
		{
			//can't compare empty string
			if (prevname.IsEmpty()) prevname = _r.GetItemName();
			if (thumbindex < 0) thumbindex = i;// assign thumbindex if already sorted
			//sort by name
			if (-1 == StrCmpLogicalW(_r.GetItemName(), prevname))
			{
				thumbindex = i;
				prevname = _r.GetItemName();
			}
		}
		_r.SkipItem();//don't forget
	}

	return thumbindex;
}


HRESULT Cbr(HBITMAP* phBmpThumbnail)
{
	{
		CUnRar _r;
		__int64 thumbindex = -1;

		if (m_bSort)
		{
			thumbindex = FindThumbnailSortRAR(m_cbxFile);
			if (thumbindex < 0) return E_FAIL;
		}

		if (!_r.Open(m_cbxFile, FALSE)) return E_FAIL;

		if (m_bSort)
		{
			//archive flags already checked, go to thumbindex
			if (!_r.SkipItems(thumbindex)) return E_FAIL;
			if (!_r.ReadItemInfo()) return E_FAIL;
		}
		else
		{
			//skip solid (long processing time), volumes or encrypted file headers
			if (_r.IsArchiveSolid() || _r.IsArchiveVolume() || _r.IsArchiveEncryptedHeaders()) return E_FAIL;

			while (_r.ReadItemInfo())
			{
				//skip directory/empty/ file bigger than 32mb
				if (!(_r.IsItemDirectory() || 
					 (_r.GetItemPackedSize64() == 0) ||
					 (_r.GetItemUnpackedSize64() == 0) || 
					 (_r.GetItemUnpackedSize64() > CBXMEM_MAXBUFFER_SIZE)))
				{
					if (IsImage(_r.GetItemName())) 
					{ 
						thumbindex = TRUE; // not actually an index, merely a "go ahead" flag
						break; 
					}
				}

				_r.SkipItem();//don't forget
			}
		}//else

		if (thumbindex < 0) 
			return E_FAIL;

		//create thumb
		IStream* pIs = NULL;
		HGLOBAL hG = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, (SIZE_T)_r.GetItemUnpackedSize64());
		if (hG)
		{
			if (S_OK == CreateStreamOnHGlobal(hG, TRUE, (LPSTREAM*)&pIs))
			{
				_r.SetIStream(pIs);
				if (_r.ProcessItem())
					*phBmpThumbnail = ThumbnailFromIStream(pIs, &m_thumbSize, FALSE);
			}
		}
		GlobalFree(hG);
		pIs->Release();
		return ((*phBmpThumbnail) ? S_OK : E_FAIL);
	}
}