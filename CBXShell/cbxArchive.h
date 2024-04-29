/*
* DarkThumbs - Windows Explorer thumbnails for ebooks, image archives
* Copyright © 2020-2022 by Kevin Routley. All rights reserved.
*
* Primary processing code.
*/

///////////////////////////////////////////////
// v4.6
//////////////////////////////////////////////
// CCBXShell functionality implementation

#ifndef _CBXARCHIVE_442B998D_B9C0_4AB0_BB2A_BC9C0AA10053_
#define _CBXARCHIVE_442B998D_B9C0_4AB0_BB2A_BC9C0AA10053_

#ifndef STRICT
#define STRICT
#endif

#include <shlObj.h>
#include <shlwapi.h>
#pragma comment(lib,"shlwapi.lib")

//#include "gdiplus.h" // uncomment if needed
//#pragma comment(lib,"gdiplus.lib")

//ATL headers
//#include <atlstr.h> // uncomment if needed
#include <atlimage.h>
#include <atlfile.h>

// WTL headers
#include <atlgdi.h>

#include "unrar.h"
#ifdef _WIN64
#pragma comment(lib,"unrar64.lib")
#else
#pragma comment(lib,"unrar.lib")
#endif
#include "cunzip.h"
#include <string>
#ifdef _WIN64
#include "mobi.h"
#endif

#ifdef _DEBUG
#include "debugapi.h"
#endif

#define CBXMEM_MAXBUFFER_SIZE 33554432 //32mb
#define CBXTYPE int
#define CBXTYPE_NONE 0
#define CBXTYPE_ZIP  1
#define CBXTYPE_CBZ  2
#define CBXTYPE_RAR  3
#define CBXTYPE_CBR  4
#define CBXTYPE_EPUB 5
#ifdef _WIN64
#define CBXTYPE_MOBI 6
#endif
#define CBXTYPE_FB   7

#define CBX_APP_KEY _T("Software\\DarkThumbs\\{9E6ECB90-5A61-42BD-B851-D3297D9C7F39}")

CString GetEpubTitle(CString);
HRESULT ExtractEpub(CString m_cbxFile, HBITMAP* phBmpThumbnail, SIZE m_thumbSize, BOOL m_showIcon);

inline BOOL StrEqual(LPCTSTR psz1, LPCTSTR psz2);
BOOL IsImage(LPCTSTR szFile);
HBITMAP ThumbnailFromIStream(IStream* pIs, const LPSIZE pThumbSize, bool showIcon);
HRESULT WICCreate32BitsPerPixelHBITMAP(IStream* pstm, HBITMAP* phbmp);
void addIcon(HBITMAP* phBmpThumbnail);

HRESULT ExtractFBCover(CString filepath, HBITMAP* phBmpThumbnail);

extern void __cdecl logit(LPCWSTR format, ...);

namespace __cbx {


// unrar wrapper
typedef const RARHeaderDataEx* LPCRARHeaderDataEx;
typedef const RAROpenArchiveDataEx* LPCRAROpenArchiveDataEx;

class CUnRar
{
public:
	CUnRar() { if (RAR_DLL_VERSION>RARGetDllVersion()) throw RAR_DLL_VERSION;	_init(); }
	virtual ~CUnRar(){Close();_init();}
	void Shutdown() { Close(); _init(); }

public:
	BOOL Open(LPCTSTR rarfile, BOOL bListingOnly=TRUE, char* cmtBuf=NULL, UINT cmtBufSize=0, char* password=NULL)
	{
		if (m_harc) return FALSE;//must close old first

		SecureZeroMemory(&m_arcinfo, sizeof(RAROpenArchiveDataEx));
	#ifndef UNICODE
		m_arcinfo.ArcName=(PTCHAR)rarfile;
	#else
		m_arcinfo.ArcNameW=(PTCHAR)rarfile;
	#endif
		m_arcinfo.OpenMode=bListingOnly ? RAR_OM_LIST : RAR_OM_EXTRACT;
		m_arcinfo.CmtBuf=cmtBuf;
		m_arcinfo.CmtBufSize=cmtBufSize;

		m_harc=RAROpenArchiveEx(&m_arcinfo);
		if (m_harc==NULL || m_arcinfo.OpenResult!=0) return FALSE;

		if (password) RARSetPassword(m_harc,password);
		RARSetCallback(m_harc, __rarCallbackProc, (LPARAM)this);
	return TRUE;
	}

	BOOL Close() { if (m_harc) m_ret=RARCloseArchive(m_harc); return (m_ret!=0);}

	inline LPCRAROpenArchiveDataEx GetArchiveInfo() {return &m_arcinfo;}
	inline LPCRARHeaderDataEx GetItemInfo() {return &m_iteminfo;}
	inline void SetPassword(char* password) {RARSetPassword(m_harc, password);}
	inline int GetLastError() {return m_ret;}

	//inline UINT GetArchiveFlags() {return m_arcinfo.Flags;}
	//inline UINT GetItemFlags(){return m_iteminfo.Flags;}
	inline BOOL IsArchiveVolume() {return (BOOL)(m_arcinfo.Flags & 0x0001);}
	inline BOOL IsArchiveComment() {return (BOOL)(m_arcinfo.Flags & 0x0002);}
	inline BOOL IsArchiveLocked() {return (BOOL)(m_arcinfo.Flags & 0x0004);}
	inline BOOL IsArchiveSolid() {return (BOOL)(m_arcinfo.Flags & 0x0008);}
	inline BOOL IsArchivePartN() {return (BOOL)(m_arcinfo.Flags & 0x0010);}
	inline BOOL IsArchiveSigned() {return (BOOL)(m_arcinfo.Flags & 0x0020);}
	inline BOOL IsArchiveRecoveryRecord() {return (BOOL)(m_arcinfo.Flags & 0x0040);}
	inline BOOL IsArchiveEncryptedHeaders() {return (BOOL)(m_arcinfo.Flags & 0x0080);}
	inline BOOL IsArchiveFirstVolume() {return (BOOL)(m_arcinfo.Flags & 0x0100);}
	
	BOOL ReadItemInfo()
	{
		SecureZeroMemory(&m_iteminfo, sizeof(RARHeaderDataEx));
		m_ret=RARReadHeaderEx(m_harc, &m_iteminfo);
	return (m_ret==0);
	}

	inline BOOL IsItemDirectory() {return ((m_iteminfo.Flags & 0x00E0)==0x00E0);}

	inline LPCTSTR GetItemName()
	{
#ifdef UNICODE
	return (LPCTSTR)(GetItemInfo()->FileNameW);
#else
	return (LPCTSTR)CUnRar::GetItemInfo()->FileName;
#endif
	}

	//MAKEUINT64
	inline UINT64 GetItemPackedSize64() {return ((((UINT64)m_iteminfo.PackSizeHigh)<<32) | m_iteminfo.PackSize);}
	inline UINT64 GetItemUnpackedSize64() {return ((((UINT64)m_iteminfo.UnpSizeHigh)<<32) | m_iteminfo.UnpSize);}

	virtual BOOL ProcessItem()
	{
		m_ret=RARProcessFileW(m_harc,RAR_TEST,NULL,NULL);
		if (m_ret!=0) return FALSE;
	return TRUE;
	}

	virtual BOOL SkipItem()
	{
		m_ret=RARProcessFile(m_harc,RAR_SKIP,NULL,NULL);
	return (m_ret==0);
	}

	virtual BOOL SkipItems(UINT64 si)
	{
		UINT64 _i;//0 skips?
		for (_i=0; _i<si; _i++) {if (!ReadItemInfo() || !SkipItem()) return FALSE;}
	return TRUE;
	}

	virtual int CallbackProc(UINT msg, LPARAM UserData, LPARAM P1, LPARAM P2)
	{
		if (msg==UCM_PROCESSDATA) return ProcessItemData((LPBYTE)P1,(ULONG)P2);
	return -1;
	}

	virtual int ProcessItemData(LPBYTE pBuf, ULONG dwBufSize)
	{
		logit(_T("PID: %ld"), dwBufSize);
		if (m_pIs)
		{
			ULONG br=0;
			if (S_OK == m_pIs->Write(pBuf, dwBufSize, &br))
			{
				if (br == dwBufSize) return 1;
				else logit(_T("--write %ld"), br);
			}
		}
		if (m_pBuf)
		{
			// The "problem cbr" files have 3 callbacks, data has to be appended
			memcpy(m_pBuf, pBuf, dwBufSize);
			m_pBuf += dwBufSize;
			return 1;
		}
		logit(_T("PID: fail"));
	return -1;
	}

	void SetIStream(IStream* pIs){_ASSERTE(pIs);m_pIs=pIs;}
	void SetBuffer(LPBYTE pbuf) { _ASSERTE(pbuf); m_pBuf = pbuf; }

private:
	HANDLE m_harc;
	RARHeaderDataEx m_iteminfo;
	RAROpenArchiveDataEx m_arcinfo;
	int m_ret;
	IStream* m_pIs;
	LPBYTE m_pBuf;
	void _init()
	{
		m_harc=NULL;
		m_pIs=NULL;
		m_ret=0;
		SecureZeroMemory(&m_arcinfo, sizeof(RAROpenArchiveDataEx));
		SecureZeroMemory(&m_iteminfo, sizeof(RARHeaderDataEx));
	}

	//raw callback function
public:
	static int PASCAL __rarCallbackProc(UINT msg,LPARAM UserData,LPARAM P1,LPARAM P2)
	{
		CUnRar* _pc=(CUnRar*)UserData;
	return _pc->CallbackProc(msg,UserData,P1,P2);
	}
};

class CCBXArchive
{
public:
	CCBXArchive()
	{
		m_bSort=TRUE;//default
		m_showIcon = FALSE;
		//GetRegSettings();
		m_thumbSize.cx=m_thumbSize.cy=0;
		m_cbxType=CBXTYPE_NONE;
		m_pIs=NULL;

		m_bSkip = FALSE;  // V1.7 : new setting, don't change existing behavior
		m_bCover = FALSE; // V1.7
	}

	virtual ~CCBXArchive()
	{
		if (m_pIs) {m_pIs->Release(); m_pIs=NULL;}
	}

#ifdef _WIN64
    //
	// Extrapolated from mobitool.c in the libmobi repo. See https://github.com/bfabiszewski/libmobi
	//
	int fetchMobiCover(const MOBIData* m, HBITMAP* phBmpThumbnail, SIZE thumbSize, BOOL showIcon) 
	{
		MOBIPdbRecord* record = NULL;
		MOBIExthHeader* exth = mobi_get_exthrecord_by_tag(m, EXTH_COVEROFFSET);
		if (exth) 
		{
			uint32_t offset = mobi_decode_exthvalue((const unsigned char*)(exth->data), exth->size);
			size_t first_resource = mobi_get_first_resource_record(m);
			size_t uid = first_resource + offset;
			record = mobi_get_record_by_seqnumber(m, uid);
		}
		if (record == NULL || record->size < 4) 
		{
			return E_FAIL;
		}

		HGLOBAL hG = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, (SIZE_T)record->size);
		//HRESULT hr = E_FAIL;
		if (hG)
		{
			LPVOID pBuf = ::GlobalLock(hG);
			if (pBuf)
				CopyMemory(pBuf, record->data, record->size);

			if (::GlobalUnlock(hG) == 0 && GetLastError() == NO_ERROR)
			{
				IStream* pIs = NULL;
				if (S_OK == CreateStreamOnHGlobal(hG, TRUE, (LPSTREAM*)&pIs))//autofree hG
				{
					// Issue #84: never show archive icon for MOBI
					//*phBmpThumbnail = ThumbnailFromIStream(pIs, &thumbSize, FALSE); // showIcon);
					//hr = WICCreate32BitsPerPixelHBITMAP(pIs, phBmpThumbnail);

					// Issue *86 use WIC for mobi
					IStream* pImageStream = SHCreateMemStream((const BYTE*)pBuf, record->size);
					HRESULT hr = WICCreate32BitsPerPixelHBITMAP(pImageStream, phBmpThumbnail);
					pImageStream->Release();

					pIs->Release();
					pIs = NULL;
				}
			}
			GlobalFree(hG);
		}
		return ((*phBmpThumbnail) ? S_OK : E_FAIL);
	}

	//
	// Extrapolated from mobitool.c in the libmobi repo. See https://github.com/bfabiszewski/libmobi
	//
	HRESULT ExtractMobiCover(CString filepath, HBITMAP* phBmpThumbnail, BOOL showIcon)
	{
		MOBI_RET mobi_ret;
		int ret = S_OK;
		/* Initialize main MOBIData structure */
		MOBIData* m = mobi_init();
		if (m == NULL) {
			return E_FAIL;
		}
		/* By default loader will parse KF8 part of hybrid KF7/KF8 file */
	//    if (parse_kf7_opt) 
		{
			/* Force it to parse KF7 part */
			mobi_parse_kf7(m);
		}
		errno = 0;
		FILE* file;
		errno_t errnot = _wfopen_s(&file, filepath, L"rb");
		if (file == NULL) {
			int errsv = errnot;
			mobi_free(m);
			return E_FAIL;
		}
		/* MOBIData structure will be filled with loaded document data and metadata */
		mobi_ret = mobi_load_file(m, file);
		fclose(file);

		if (mobi_ret != MOBI_SUCCESS) {
			mobi_free(m);
			return E_FAIL;
		}

		ret = fetchMobiCover(m, phBmpThumbnail, m_thumbSize, showIcon);
		/* Free MOBIData structure */
		mobi_free(m);
		return ret;
	}
#endif

	HRESULT ExtractZip(HBITMAP* phBmpThumbnail, BOOL toSort)
	{
		//logit(_T("Z:sort:'%d'"), toSort);

		CUnzip _z;
		if (!_z.Open(m_cbxFile)) 
			return E_FAIL;
		int j = _z.GetItemCount();
		if (j == 0) 
			return E_FAIL;

		CString prevname;
		int foundIndex = -1;

		for (int i = 0; i < j; i++)
		{
			// Skip empty items, directories, etc.
			if (!_z.GetItem(i)) break;
			if (_z.ItemIsDirectory() || (_z.GetItemUnpackedSize() > CBXMEM_MAXBUFFER_SIZE)) continue;
			if ((_z.GetItemPackedSize() == 0) || (_z.GetItemUnpackedSize() == 0)) continue;

			CString iName = _z.GetItemName();

			// Skip non-images and other undesirables
			if (iName.Find(L"__MACOSX") != -1) continue;
			if (!IsImage(iName)) continue;

			// Skipping scanlation files takes precedence
			auto allLower = iName.MakeLower();
			if (m_bSkip)
			{
				// TODO could these come from a registry setting?
				// TODO manga like "Death Note" which might fail all images?
				if (allLower.Find(L"credit") != -1 ||
					allLower.Find(L"note") != -1 ||
					allLower.Find(L"recruit") != -1 ||
					allLower.Find(L"invite") != -1 ||
					allLower.Find(L"logo") != -1)
				{
					continue;
				}
			}

			// Not a skipped image
			if (!toSort)
			{
				foundIndex = i; // Not sorting: take the first non-skipped image file
				break;
			}
			if (m_bCover && allLower.Find(L"cover") != -1)
			{
				foundIndex = i; // Seeking cover file: take the first "cover"
				break;
			}

			// During sorting, force certain characters to sort later.
			allLower.Replace(L"[", L"z");

			//logit(_T("Z:'%ls'(%d) vs '%ls(%d)'"), allLower, i, prevname, foundIndex);

			// So we're sorting. Want to take the first file in alphabetic order
			if (prevname.IsEmpty())
			{
				prevname = allLower;// can't compare empty string
				foundIndex = i;  // initialize when sorting
			}
			else if (-1 == StrCmpLogicalW(allLower, prevname))
			{
				foundIndex = i;
				prevname = allLower;
			}


		}//for loop

		if (foundIndex < 0) 
			return E_FAIL;

		//go to thumb index
		if (!_z.GetItem(foundIndex)) 
			return E_FAIL;

		//create thumb			//GHND
		HGLOBAL hG = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, (SIZE_T)_z.GetItemUnpackedSize());
		if (hG)
		{
			bool b = false;
			LPVOID pBuf = ::GlobalLock(hG);

			long itemSize = _z.GetItemUnpackedSize();
			if (pBuf)
				b = _z.UnzipItemToMembuffer(foundIndex, pBuf, itemSize);

			if (::GlobalUnlock(hG) == 0 && GetLastError() == NO_ERROR && b)
			{
				IStream* pImageStream = SHCreateMemStream((const BYTE*)pBuf, itemSize);
				HRESULT hr = WICCreate32BitsPerPixelHBITMAP(pImageStream, phBmpThumbnail);
				pImageStream->Release();
			}
			GlobalFree(hG);
		}

		if (m_showIcon)
			addIcon(phBmpThumbnail);

		return ((*phBmpThumbnail) ? S_OK : E_FAIL);
	}

	HRESULT ExtractRar(HBITMAP* phBmpThumbnail)
	{
		//logit(_T("R:sort:'%d'"), m_bSort);

		CUnRar _r;

		if (!_r.Open(m_cbxFile, FALSE))
			return E_FAIL;

		// Skip volumes or encrypted
		if (_r.IsArchiveVolume() || _r.IsArchiveEncryptedHeaders()) 
			return E_FAIL;

		CString prevname; // sorting
		int foundIndex = -1;
		int i = -1;

		while (_r.ReadItemInfo())
		{
			i++;
			//skip directory/empty/ file bigger than 32mb
			if (_r.IsItemDirectory() || (_r.GetItemPackedSize64() == 0) ||
				(_r.GetItemUnpackedSize64() == 0) || (_r.GetItemUnpackedSize64() > CBXMEM_MAXBUFFER_SIZE))
			{
				_r.SkipItem();
				continue;
			}

			CString iName = _r.GetItemName();

			// Skip non-image
			if (!IsImage(iName))
			{
				_r.SkipItem();
				continue;
			}

			// Skipping scanlation files takes precedence
			auto allLower = iName.MakeLower();
			if (m_bSkip)
			{
				// TODO refactor to subroutine
				if (allLower.Find(L"credit") != -1 ||
					allLower.Find(L"note") != -1 ||
					allLower.Find(L"recruit") != -1 ||
					allLower.Find(L"invite") != -1 ||
					allLower.Find(L"logo") != -1)
				{
					_r.SkipItem();
					continue;
				}
			}

			// For the next two tests, if they pass, the CUnrar instance is
			// "set" to the item of interest. When sorting, it may be required
			// to "rewind" the CUnrar instance to the item desired.
			if (!m_bSort)
			{
				goto AtThumb;
			}
			if (m_bCover && allLower.Find(L"cover") != -1)
			{
				goto AtThumb; // Seeking cover file: take the first "cover"
			}

			//logit(_T("R:'%ls'(%d) vs '%ls(%d)'"), allLower, i, prevname, foundIndex);

			// We're sorting. Want to take the first file in natural order.
			if (prevname.IsEmpty())
			{
				prevname = allLower;// can't compare empty string
				foundIndex = i;  // initialize when sorting
			}
			else if (-1 == StrCmpLogicalW(allLower, prevname))
			{
				foundIndex = i;
				prevname = allLower;
			}
			_r.SkipItem();

		}

		if (foundIndex < 0) 
		{
			logit(_T("R Fail 1")); return E_FAIL;
		}

		// Need to reposition the archive to the item of interest
		// TODO why can't RAR be reset w/o closing and re-opening?
		_r.Shutdown();
		if (!_r.Open(m_cbxFile, FALSE)) 
		{
			logit(_T("R Fail 2")); return E_FAIL;
		}
		if (!_r.SkipItems(foundIndex))
		{
			logit(_T("R Fail 3")); return E_FAIL;
		}
		if (!_r.ReadItemInfo())
		{
			logit(_T("R Fail 4")); return E_FAIL;
		}

AtThumb:
		// _r is currently 'positioned' at the item desired

		//create thumb
		IStream* pIs = NULL;
		UINT64 itemSize = _r.GetItemUnpackedSize64();
		logit(_T("R: itemsize %ld"), itemSize);
		HGLOBAL hG = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, (UINT)itemSize);
		//HRESULT hr;
		if (hG)
		{
			LPVOID pBuf = ::GlobalLock(hG);
			LPVOID origbuf = pBuf; // note that _r.ProcessItem() could modify the buffer pointer!

			if (S_OK == CreateStreamOnHGlobal(hG, TRUE, (LPSTREAM*)&pIs))
			{
				//_r.SetIStream(pIs);
				_r.SetIStream(NULL);
				_r.SetBuffer((LPBYTE)pBuf);
				if (_r.ProcessItem())
					//if (_r.ProcessItemData((LPBYTE) hG, _r.GetItemUnpackedSize64()))
				{
					IStream* pImageStream = SHCreateMemStream((const BYTE*)origbuf, (UINT)itemSize);
					HRESULT hr = WICCreate32BitsPerPixelHBITMAP(pImageStream, phBmpThumbnail);
					pImageStream->Release();
					//*phBmpThumbnail = ThumbnailFromIStream(pIs, &m_thumbSize, m_showIcon);
				}
			}
			::GlobalUnlock(hG);
		}
		GlobalFree(hG);
		pIs->Release();
		return ((*phBmpThumbnail) ? S_OK : E_FAIL);
	}

public:
	////////////////////////////////////////
	// IPersistFile::Load
	HRESULT OnLoad(LPCOLESTR wszFile)
	{
		//logit(_T("OnLoad"));

		//ATLTRACE("IPersistFile::Load\n");
#ifndef UNICODE
		USES_CONVERSION;
		m_cbxFile=OLE2T((WCHAR*)wszFile);
#else
		m_cbxFile=wszFile;
#endif
		m_cbxType=GetCBXType(PathFindExtension(m_cbxFile));
	return S_OK;
	}

	////////////////////////////////////
	// IExtractImage::GetLocation(LPWSTR pszPathBuffer,	DWORD cchMax, DWORD *pdwPriority, const SIZE *prgSize, DWORD dwRecClrDepth, DWORD *pdwFlags)
	HRESULT OnGetLocation(const SIZE *prgSize, DWORD *pdwFlags)
	{
		//logit(_T("OnGetLocation (%d,%d)"), prgSize->cx, prgSize->cy);

		//ATLTRACE("IExtractImage2::GetLocation\n");
		m_thumbSize.cx=prgSize->cx;
		m_thumbSize.cy=prgSize->cy;
		*pdwFlags |= (IEIFLAG_CACHE | IEIFLAG_REFRESH);//cache thumbnails
		//if (*pdwFlags & IEIFLAG_ASYNC) return E_PENDING;//Windows XP and earlier
#ifdef _DEBUG
		if (*pdwFlags & IEIFLAG_ASYNC) ATLTRACE("\nIExtractImage::GetLocation : IEIFLAG_ASYNC flag set\n");
#endif
	return NOERROR;
	}

	////////////////////////////////////
	// IExtractImage2::GetDateStamp(FILETIME *pDateStamp)
	HRESULT OnGetDateStamp(FILETIME *pDateStamp)
	{
		logit(_T("OnGetDateStamp"));
		//ATLTRACE("IExtractImage2::GetDateStamp\n");
		FILETIME ftCreationTime, ftLastAccessTime, ftLastWriteTime;
		CAtlFile _f;
		if (S_OK!=_f.Create(m_cbxFile,GENERIC_READ,FILE_SHARE_READ,OPEN_EXISTING,0)) return E_FAIL;
		//alternatively GetFileInformationByHandle?
		if (!GetFileTime(_f, &ftCreationTime, &ftLastAccessTime, &ftLastWriteTime)) return E_FAIL;
		*pDateStamp = ftLastWriteTime;
	return NOERROR;
	}

	////////////////////////////////////
	//IExtractImage::Extract(HBITMAP* phBmpThumbnail)
	HRESULT OnExtract(HBITMAP* phBmpThumbnail)
	{
		LoadRegistrySettings();
		*phBmpThumbnail=NULL;
		//ATLTRACE("IExtractImage::Extract\n");
		logit(_T("OnExtract '%ls' (%d)"), m_cbxFile, m_showIcon ? 1 : 0);

try {
		switch (m_cbxType)
		{
#ifdef _WIN64
		case CBXTYPE_MOBI:
			return ExtractMobiCover(m_cbxFile, phBmpThumbnail, m_showIcon);
#endif

		case CBXTYPE_EPUB:
		{
			if (ExtractEpub(m_cbxFile, phBmpThumbnail, m_thumbSize, m_showIcon) != E_FAIL)
			{
				if (m_showIcon)
					addIcon(phBmpThumbnail);
				return S_OK;
			}

			// something wrong with the epub, try falling back on first image in zip
			logit(_T("__Epub Extract: fallback to ZIP"));
			// NOTE: fallthrough down to Zip!
		}

		case CBXTYPE_ZIP:
		case CBXTYPE_CBZ:
			// NOTE: fallthrough from epub!
			return ExtractZip(phBmpThumbnail, m_bSort);
		break;

		case CBXTYPE_RAR:
		case CBXTYPE_CBR:
			if (ExtractRar(phBmpThumbnail) != E_FAIL)
			{
				// Issue #87: show icon for RAR/CBR
				if (m_showIcon)
					addIcon(phBmpThumbnail);
				return S_OK;
			}
			break;

		case CBXTYPE_FB:
			return ExtractFBCover(m_cbxFile, phBmpThumbnail);
			break;

		default:return E_FAIL;
		}
}
catch (...) {
	//ATLTRACE("exception in IExtractImage::Extract\n");
	logit(_T("exception in IExtractImage::Extract"));
	return E_FAIL;
}
	return S_OK;
	}

	//////////////////////////////
	//IQueryInfo::GetInfoTip(DWORD dwFlags, LPWSTR *ppwszTip)
	HRESULT OnGetInfoTip(LPWSTR *ppwszTip)
	{
		logit(_T("OnGetInfoTip '%ls'"), m_cbxFile);

		//ATLTRACE("IQueryInfo::GetInfoTip\n");
try
{
		CString tip;

		__int64 _fs;
		if (!GetFileSizeCrt(m_cbxFile, _fs)) return E_FAIL;

		TCHAR _tf[16];// SecureZeroMemory?
		int i, j;

		switch (m_cbxType)
		{
		case CBXTYPE_ZIP:
				if (_fs==0) tip=_T("ZIP Archive\nSize: 0 bytes");
				else
				{
					if (GetImageCountZIP(m_cbxFile, i,j))
						tip.Format(_T("ZIP Archive\n%d Images\n%d Files\nSize: %s"),
													i, j, StrFormatByteSize64(_fs, _tf, 16));
					else tip.Format(_T("ZIP Archive\nSize: %s"), StrFormatByteSize64(_fs, _tf, 16));
				}
		break;
		case CBXTYPE_CBZ:
				if (_fs==0) tip=_T("ZIP Image Archive\nSize: 0 bytes");
				else
				{
					if (GetImageCountZIP(m_cbxFile, i,j))
						tip.Format(_T("ZIP Image Archive\n%d Images\n%d Files\nSize: %s"),
													i, j, StrFormatByteSize64(_fs, _tf, 16));
					else tip.Format(_T("ZIP Image Archive\nSize: %s"), StrFormatByteSize64(_fs, _tf, 16));
				}
		break;
		case CBXTYPE_RAR:
				if (_fs==0) tip=_T("RAR Archive\nSize: 0 bytes");
				else
				{
					if (GetImageCountRAR(m_cbxFile, i,j))
						tip.Format(_T("RAR Archive\n%d Images\n%d Files\nSize: %s"),
													i, j, StrFormatByteSize64(_fs, _tf, 16));
					else tip.Format(_T("RAR Archive\nSize: %s"), StrFormatByteSize64(_fs, _tf, 16));
				}
		break;
		case CBXTYPE_CBR:
				if (_fs==0) tip=_T("RAR Image Archive\nSize: 0 bytes");
				else
				{
					if (GetImageCountRAR(m_cbxFile, i,j))
						tip.Format(_T("RAR Image Archive\n%d Images\n%d Files\nSize: %s"),
													i, j, StrFormatByteSize64(_fs, _tf, 16));
					else tip.Format(_T("RAR Image Archive\nSize: %s"), StrFormatByteSize64(_fs, _tf, 16));
				}
		break;
		case CBXTYPE_EPUB:
				if (_fs == 0) tip = _T("EPUB File\nSize: 0 bytes");
				else
				{
					CString title = GetEpubTitle(m_cbxFile);

					if (title.GetLength() > 0)
						tip.Format(_T("EPUB File\n%s\nSize: %s"),
							title, StrFormatByteSize64(_fs, _tf, 16));
					else tip.Format(_T("EPUB File\nSize: %s"), StrFormatByteSize64(_fs, _tf, 16));
				}
		break;

		default: ATLTRACE("IQueryInfo::GetInfoTip : CBXTYPE_NONE\n");return E_FAIL;
		}

		*ppwszTip = (WCHAR*) ::CoTaskMemAlloc( (tip.GetLength()+1)*sizeof(WCHAR) );//caller must call CoTaskMemFree
		if (*ppwszTip==NULL) return E_FAIL;
		if (0!=::wcscpy_s(*ppwszTip, tip.GetLength()+1, tip)) return E_FAIL;

	return S_OK;
}
catch (...)
{ 
	//ATLTRACE("exception in IQueryInfo::GetInfoTip\n"); 
	logit(_T("OnGetInfoTip Exception"));
	return E_FAIL; // S_FALSE;
}

	return S_FALSE;
	}


private:
	CStringW m_cbxFile;//overcome MAX_PATH limit?
	SIZE m_thumbSize;
	CBXTYPE m_cbxType;
	IStream* m_pIs;
	BOOL m_bSort;
	BOOL m_showIcon;
	BOOL m_bSkip;  // V1.7 : skip common scanslation files
	BOOL m_bCover; // V1.7 : prefer a file with "cover" in the name


private:
	inline BOOL GetFileSizeCrt(LPCTSTR pszFile, __int64 &fsize)
	{
		struct _stat64 _s;
		_s.st_size=0;
		if (0!=::_tstat64(pszFile, &_s)) return FALSE;
		fsize=_s.st_size;
	return TRUE;
	}

	inline CBXTYPE GetCBXType(LPCTSTR szExt)
	{
		if (StrEqual(szExt, _T(".cbz"))) return CBXTYPE_CBZ;
		if (StrEqual(szExt, _T(".zip"))) return CBXTYPE_ZIP;
		if (StrEqual(szExt, _T(".cbr"))) return CBXTYPE_CBR;
		if (StrEqual(szExt, _T(".rar"))) return CBXTYPE_RAR;
		if (StrEqual(szExt, _T(".epub"))) return CBXTYPE_EPUB;
		if (StrEqual(szExt, _T(".phz"))) return CBXTYPE_CBZ;
#ifdef _WIN64
		if (StrEqual(szExt, _T(".mobi"))) return CBXTYPE_MOBI;
		if (StrEqual(szExt, _T(".azw"))) return CBXTYPE_MOBI;
		if (StrEqual(szExt, _T(".azw3"))) return CBXTYPE_MOBI;
#endif
		if (StrEqual(szExt, _T(".fb2"))) return CBXTYPE_FB;

		return CBXTYPE_NONE;
	}

	BOOL GetImageCountZIP(LPCTSTR cbzFile, int &imagecount, int &filecount)
	{
		imagecount=0;
		filecount=0;

		CUnzip _z;
		if (!_z.Open(cbzFile)) return FALSE;
		//empty zip is still valid
		if (_z.GetItemCount()==0) return TRUE;
	 
		int _i;
		for (_i=0; _i<_z.GetItemCount(); _i++)
		{
			if (!_z.GetItem(_i)) return FALSE;
			//skip dirs
			if (_z.ItemIsDirectory()) continue;
			if (IsImage(_z.GetItemName())) imagecount+=1;
			filecount+=1;
		}
	return TRUE;
	}

	BOOL GetImageCountRAR(LPCTSTR cbrFile, int &imagecount, int &filecount)
	{
		imagecount=0;
		filecount=0;

		CUnRar cr;
		if (!cr.Open(cbrFile)) return FALSE;

		//enum solid / skip volumes or encrypted file header archives
		if (cr.IsArchiveVolume() || cr.IsArchiveEncryptedHeaders()) return FALSE;

		while (cr.ReadItemInfo())
		{
			//skip dirs
			if (!cr.IsItemDirectory())
			{
				if (IsImage(cr.GetItemName())) imagecount+=1;
				filecount+=1;
			}
			if (!cr.SkipItem()) return FALSE;
		}
	return TRUE;
	}

public:
	void LoadRegistrySettings()
	{
		DWORD _d;
		CRegKey _rk;
		if (ERROR_SUCCESS==_rk.Open(HKEY_CURRENT_USER, CBX_APP_KEY, KEY_READ))
		{
			if (ERROR_SUCCESS==_rk.QueryDWORDValue(_T("NoSort"), _d))
				m_bSort=(_d == 0); // Sort is backward
			if (ERROR_SUCCESS == _rk.QueryDWORDValue(_T("ShowIcon"), _d))
				m_showIcon = (_d == 1);
			if (ERROR_SUCCESS == _rk.QueryDWORDValue(_T("SkipScanlation"), _d))
				m_bSkip = (_d == 1);
			if (ERROR_SUCCESS == _rk.QueryDWORDValue(_T("PreferCover"), _d))
				m_bCover = (_d == 1);
		}
		logit(_T("LRS: %d"), m_bSort);
	}

#ifdef _DEBUG
public:
	void debug_SetSort(BOOL bS=TRUE) {m_bSort=bS;}
#endif

};//class _CCBXArchive



}//namespace __cbx

#endif//_CBXARCHIVE_442B998D_B9C0_4AB0_BB2A_BC9C0AA10053_
