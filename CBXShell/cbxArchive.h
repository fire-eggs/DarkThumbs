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

#define CBX_APP_KEY _T("Software\\T800 Productions\\{9E6ECB90-5A61-42BD-B851-D3297D9C7F39}")

CString GetEpubTitle(CString);
HRESULT ExtractEpub(CString m_cbxFile, HBITMAP* phBmpThumbnail, SIZE m_thumbSize, BOOL m_showIcon);

inline BOOL StrEqual(LPCTSTR psz1, LPCTSTR psz2);
BOOL IsImage(LPCTSTR szFile);
HBITMAP ThumbnailFromIStream(IStream* pIs, const LPSIZE pThumbSize, bool showIcon);
HRESULT WICCreate32BitsPerPixelHBITMAP(IStream* pstm, HBITMAP* phbmp);

HRESULT ExtractFBCover(CString filepath, HBITMAP* phBmpThumbnail);

namespace __cbx {


// unrar wrapper
typedef const RARHeaderDataEx* LPCRARHeaderDataEx;
typedef const RAROpenArchiveDataEx* LPCRAROpenArchiveDataEx;

class CUnRar
{
public:
	CUnRar() { if (RAR_DLL_VERSION>RARGetDllVersion()) throw RAR_DLL_VERSION;	_init(); }
	virtual ~CUnRar(){Close();_init();}

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
		if (m_pIs)
		{
			ULONG br=0;
			if (S_OK==m_pIs->Write(pBuf, dwBufSize, &br))
				if (br==dwBufSize) return 1;
		}
	return -1;
	}

	void SetIStream(IStream* pIs){_ASSERTE(pIs);m_pIs=pIs;}

private:
	HANDLE m_harc;
	RARHeaderDataEx m_iteminfo;
	RAROpenArchiveDataEx m_arcinfo;
	int m_ret;
	IStream* m_pIs;
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
//#ifdef _DEBUG
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
//#endif


public:
	CCBXArchive()
	{
		m_bSort=TRUE;//default
		//GetRegSettings();
		m_thumbSize.cx=m_thumbSize.cy=0;
		m_cbxType=CBXTYPE_NONE;
		m_pIs=NULL;
	}

	virtual ~CCBXArchive()
	{
		if (m_pIs) {m_pIs->Release(); m_pIs=NULL;}
	}

#ifdef _WIN64
    //
	// Extrapolated from mobitool.c in the libmobi repo. See https://github.com/bfabiszewski/libmobi
	//
	int fetchMobiCover(const MOBIData* m, HBITMAP* phBmpThumbnail, SIZE thumbSize) 
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
		HRESULT hr = E_FAIL;
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
					//*phBmpThumbnail = ThumbnailFromIStream(pIs, &thumbSize, false);
					hr = WICCreate32BitsPerPixelHBITMAP(pIs, phBmpThumbnail);
					pIs->Release();
					pIs = NULL;
				}
			}
			GlobalFree(hG);
		}
		return hr; // ((*phBmpThumbnail) ? S_OK : E_FAIL);
	}

	//
	// Extrapolated from mobitool.c in the libmobi repo. See https://github.com/bfabiszewski/libmobi
	//
	HRESULT ExtractMobiCover(CString filepath, HBITMAP* phBmpThumbnail)
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

		ret = fetchMobiCover(m, phBmpThumbnail, m_thumbSize);
		/* Free MOBIData structure */
		mobi_free(m);
		return ret;
	}
#endif

public:
	////////////////////////////////////////
	// IPersistFile::Load
	HRESULT OnLoad(LPCOLESTR wszFile)
	{
		logit(_T("OnLoad"));

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
		logit(_T("OnGetLocation"));

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
		*phBmpThumbnail=NULL;
		//ATLTRACE("IExtractImage::Extract\n");
		logit(_T("OnExtract '%ls'"), m_cbxFile);

try {
		switch (m_cbxType)
		{
#ifdef _WIN64
		case CBXTYPE_MOBI:
			return ExtractMobiCover(m_cbxFile, phBmpThumbnail);
#endif

		case CBXTYPE_EPUB:
		{
			if (ExtractEpub(m_cbxFile, phBmpThumbnail, m_thumbSize, m_showIcon) != E_FAIL)
				return S_OK;

			// something wrong with the epub, try falling back on first image in zip
			logit(_T("__Epub Extract: fallback to ZIP"));
		}
		case CBXTYPE_ZIP:
		case CBXTYPE_CBZ:
			{
				CUnzip _z;
				if (!_z.Open(m_cbxFile)) return E_FAIL;
				j=_z.GetItemCount();
				if (j==0) return E_FAIL;

				CString prevname;//helper vars
				int thumbindex=-1;

				for (i=0; i<j; i++)
				{
					if (!_z.GetItem(i)) break;
					if (_z.ItemIsDirectory() || (_z.GetItemUnpackedSize() > CBXMEM_MAXBUFFER_SIZE)) continue;
					if ((_z.GetItemPackedSize()==0) || (_z.GetItemUnpackedSize()==0)) continue;

					CString iName = _z.GetItemName();
					if (iName.Find(L"__MACOSX") == -1 && IsImage(iName)) // Issue #36: ignore Mac resource folder
					{
						if (thumbindex<0) thumbindex=i;// assign thumbindex if already sorted

						if (!m_bSort) break;//if NoSort
						
						if (prevname.IsEmpty()) 
							prevname=iName;//can't compare empty string
						//take only first alphabetical name
						if (-1==StrCmpLogicalW(iName, prevname))
						{
							thumbindex=i;
							prevname=_z.GetItemName();
						}
					}
				}//for loop

				if (thumbindex<0) return E_FAIL;
				//go to thumb index
				if (!_z.GetItem(thumbindex)) return E_FAIL;

				//create thumb			//GHND
				HGLOBAL hG = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, (SIZE_T)_z.GetItemUnpackedSize());
				if (hG)
				{
					bool b=false;
					LPVOID pBuf=::GlobalLock(hG);
					if (pBuf)
					     b=_z.UnzipItemToMembuffer(thumbindex, pBuf, _z.GetItemUnpackedSize());

					if (::GlobalUnlock(hG)==0 && GetLastError()==NO_ERROR)
					{
						if (b)
						{
							IStream* pIs=NULL;
							if (S_OK==CreateStreamOnHGlobal(hG, TRUE, (LPSTREAM*)&pIs))//autofree hG
							{
								//*phBmpThumbnail= ThumbnailFromIStream(pIs, &m_thumbSize, m_showIcon);
								HRESULT hr = WICCreate32BitsPerPixelHBITMAP(pIs, phBmpThumbnail);
								pIs->Release();
								pIs=NULL;
								return hr;
							}
						}
					}
				//GlobalFree(hG);//autofreed
				}

			return ((*phBmpThumbnail) ? S_OK : E_FAIL);
			}//dtors!
		break;

		case CBXTYPE_RAR:
		case CBXTYPE_CBR:
			{
				CUnRar _r;
				__int64 thumbindex=-1;

				if (m_bSort)
				{
					thumbindex=FindThumbnailSortRAR(m_cbxFile);
					if (thumbindex<0) return E_FAIL;
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
						if (!(_r.IsItemDirectory() || (_r.GetItemPackedSize64()==0) || 
							(_r.GetItemUnpackedSize64()==0) || (_r.GetItemUnpackedSize64() > CBXMEM_MAXBUFFER_SIZE)))
						{
							if (IsImage(_r.GetItemName())) {thumbindex=TRUE; break;}
						}

						_r.SkipItem();//don't forget
					}
				}//else

				if (thumbindex<0) return E_FAIL;

				//create thumb
				IStream* pIs = NULL;
				HGLOBAL hG = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, (SIZE_T)_r.GetItemUnpackedSize64());
				if (hG)
				{
					if (S_OK==CreateStreamOnHGlobal(hG, TRUE, (LPSTREAM*)&pIs))
					{
						_r.SetIStream(pIs);
						if (_r.ProcessItem())
						{
							//*phBmpThumbnail = ThumbnailFromIStream(pIs, &m_thumbSize, m_showIcon);
							HRESULT hr = WICCreate32BitsPerPixelHBITMAP(pIs, phBmpThumbnail);
						}
					}
				}
				GlobalFree(hG);
				pIs->Release();
			return ((*phBmpThumbnail) ? S_OK : E_FAIL );
			}//dtors!
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
	int i,j;//helpers
	CBXTYPE m_cbxType;
	IStream* m_pIs;
	BOOL m_bSort;
	BOOL m_showIcon;


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

	__int64 FindThumbnailSortRAR(LPCTSTR pszFile)
	{
		CUnRar _r;
		if (!_r.Open(pszFile)) return -1;
		//skip solid (long processing time), volumes or encrypted file headers
		if (_r.IsArchiveSolid() || _r.IsArchiveVolume() || _r.IsArchiveEncryptedHeaders()) return -1;

		UINT64 _ps,_us;//my speed optimization?
		CString prevname;
		__int64 thumbindex=-1;
		__int64 i=-1;//start at none (-1)

		while (_r.ReadItemInfo())
		{
			i+=1;
			_ps=_r.GetItemPackedSize64();
			_us=_r.GetItemUnpackedSize64();

			//skip directory/emtpy file/bigger than 32mb
			if (_r.IsItemDirectory() || (_us>CBXMEM_MAXBUFFER_SIZE) || (_ps==0) || (_us==0))
			 {_r.SkipItem();continue;}

			//take only index of first alphabetical name
			if (IsImage(_r.GetItemName()))
			{	
				//can't compare empty string
				if (prevname.IsEmpty()) prevname=_r.GetItemName();
				if (thumbindex<0) thumbindex=i;// assign thumbindex if already sorted
				//sort by name
				if (-1==StrCmpLogicalW(_r.GetItemName(), prevname))
				{
					thumbindex=i;
					prevname=_r.GetItemName();
				}
			}
		_r.SkipItem();//don't forget
		}

	return thumbindex;
	}

public:
	void LoadRegistrySettings()
	{
		DWORD _d;
		CRegKey _rk;
		if (ERROR_SUCCESS==_rk.Open(HKEY_CURRENT_USER, CBX_APP_KEY, KEY_READ))
		{
			if (ERROR_SUCCESS==_rk.QueryDWORDValue(_T("NoSort"), _d))
				m_bSort=(_d==FALSE);
			if (ERROR_SUCCESS == _rk.QueryDWORDValue(_T("ShowIcon"), _d))
				m_showIcon = (_d == TRUE);
		}
	}

#ifdef _DEBUG
public:
	void debug_SetSort(BOOL bS=TRUE) {m_bSort=bS;}
#endif

};//class _CCBXArchive



}//namespace __cbx

#endif//_CBXARCHIVE_442B998D_B9C0_4AB0_BB2A_BC9C0AA10053_
