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
#include "unzip.h"
#include <string>
#include "mobi.h"

#define CBXMEM_MAXBUFFER_SIZE 33554432 //32mb
#define CBXTYPE int
#define CBXTYPE_NONE 0
#define CBXTYPE_ZIP  1
#define CBXTYPE_CBZ  2
#define CBXTYPE_RAR  3
#define CBXTYPE_CBR  4
#define CBXTYPE_EPUB 5
#define CBXTYPE_MOBI 6

#define CBX_APP_KEY _T("Software\\T800 Productions\\{9E6ECB90-5A61-42BD-B851-D3297D9C7F39}")

namespace __cbx {

class CUnzip
{
public:
		CUnzip() { hz=NULL; }
		virtual ~CUnzip(){::CloseZip(hz);}

public:
		bool Open(LPCTSTR zfile)
		{
			if (zfile==NULL) return false;
			HZIP temp_hz=::OpenZip(zfile, NULL);//try new
			if (temp_hz==NULL) return false;
			Close();//close old
			hz=temp_hz;
			if (ZR_OK!=::GetZipItem(hz,-1, &maindirEntry)) return false;
		return true;
		}

		bool GetItem(int zi)
		{
			zr=::GetZipItem(hz, zi, &ZipEntry);
		return (ZR_OK==zr);
		}

		bool UnzipItemToMembuffer(int index, void *z,unsigned int len)
		{
			zr=::UnzipItem(hz, index, z, len);
		return (ZR_OK==zr);
		}

		void Close()
		{
			CloseZip(hz);
			hz=NULL;//critical!
		}

		inline BOOL ItemIsDirectory() {return (BOOL)(CUnzip::GetItemAttributes() & 0x0010);}
		int GetItemCount() const {return maindirEntry.index;}
		long GetItemPackedSize() const {return ZipEntry.comp_size;}
		long GetItemUnpackedSize() const {return ZipEntry.unc_size;}
		DWORD GetItemAttributes() const {return ZipEntry.attr;}
		LPCTSTR GetItemName() {return ZipEntry.name;}
		
		HZIP getHZIP() { return hz; }	

private:
		ZIPENTRY ZipEntry, maindirEntry;
		HZIP hz;
		ZRESULT zr;
};

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
					*phBmpThumbnail = ThumbnailFromIStream(pIs, &thumbSize, false);
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

	static inline BOOL Draw(
		CImage ci,
		_In_ HDC hDestDC,
		_In_ int xDest,
		_In_ int yDest,
		_In_ int nDestWidth,
		_In_ int nDestHeight,
		_In_ int xSrc,
		_In_ int ySrc,
		_In_ int nSrcWidth,
		_In_ int nSrcHeight,
		_In_ Gdiplus::InterpolationMode interpolationMode) throw()
	{
		Gdiplus::Bitmap bm((HBITMAP)ci, NULL);
		if (bm.GetLastStatus() != Gdiplus::Ok)
		{
			return FALSE;
		}

		Gdiplus::Graphics dcDst(hDestDC);
		dcDst.SetInterpolationMode(interpolationMode);

		Gdiplus::Rect destRec(xDest, yDest, nDestWidth, nDestHeight);

		Gdiplus::Status status = dcDst.DrawImage(&bm, destRec, xSrc, ySrc, nSrcWidth, nSrcHeight, Gdiplus::Unit::UnitPixel);

		return status == Gdiplus::Ok;
	}


	HBITMAP ThumbnailFromIStream(IStream* pIs, const LPSIZE pThumbSize, bool showIcon)
	{
		ATLASSERT(pIs);
		CImage ci;//uses gdi+ internally
		if (S_OK != ci.Load(pIs)) return NULL;

		//check size
		int tw = ci.GetWidth();
		int th = ci.GetHeight();
		float cx = (float)pThumbSize->cx;
		float cy = (float)pThumbSize->cy;
		float rx = cx / (float)tw;
		float ry = cy / (float)th;

		//if bigger size
		if ((rx < 1) || (ry < 1))
		{
			CDC hdcNew = ::CreateCompatibleDC(NULL);
			if (hdcNew.IsNull()) return NULL;

			hdcNew.SetStretchBltMode(HALFTONE);
			hdcNew.SetBrushOrg(0, 0, NULL);
			//variables retain values until assignment
			tw = (int)(min(rx, ry) * tw);//C424 warning workaround
			th = (int)(min(rx, ry) * th);

			CBitmap hbmpNew;
			hbmpNew.CreateCompatibleBitmap(ci.GetDC(), tw, th);
			ci.ReleaseDC();//don't forget!
			if (hbmpNew.IsNull()) return NULL;

			HBITMAP hbmpOld = hdcNew.SelectBitmap(hbmpNew);
			hdcNew.FillSolidRect(0, 0, tw, th, RGB(255, 255, 255));//white background

			Draw(ci, hdcNew, 0, 0, tw, th, 0, 0, ci.GetWidth(), ci.GetHeight(), Gdiplus::InterpolationMode::InterpolationModeHighQualityBicubic);//too late for error checks
			if (showIcon)
				DrawIcon(hdcNew, 0, 0, zipIcon);

			hdcNew.SelectBitmap(hbmpOld);

			return hbmpNew.Detach();
		}

		return ci.Detach();
	}


public:
	////////////////////////////////////////
	// IPersistFile::Load
	HRESULT OnLoad(LPCOLESTR wszFile)
	{
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
		//ATLTRACE("IExtractImage2::GetDateStamp\n");
		FILETIME ftCreationTime, ftLastAccessTime, ftLastWriteTime;
		CAtlFile _f;
		if (S_OK!=_f.Create(m_cbxFile,GENERIC_READ,FILE_SHARE_READ,OPEN_EXISTING,0)) return E_FAIL;
		//alternatively GetFileInformationByHandle?
		if (!GetFileTime(_f, &ftCreationTime, &ftLastAccessTime, &ftLastWriteTime)) return E_FAIL;
		*pDateStamp = ftLastWriteTime;
	return NOERROR;
	}

	void ReplaceStringInPlace(std::string& subject, const std::string& search,
		const std::string& replace) {
		size_t pos = 0;
		while ((pos = subject.find(search, pos)) != std::string::npos) {
			subject.replace(pos, search.length(), replace);
			pos += replace.length();
		}
	}

	// EPUP3 standard: open the container.xml file to fetch the path of the rootfile
	// Requires a successfully opened and not empty zipfile
	std::string GetEpubRootFile(CUnzip *_z)
	{
		std::string rootfile;

		// KBR 20210717 FindZipItem faster than iterating using the zip wrapper
		int dex;
		ZIPENTRY ze;
		FindZipItem(_z->getHZIP(), _T("META-INF/container.xml"), false, &dex, &ze);
		if (dex < 0)
			return rootfile;
		
		long itemSize = ze.unc_size;

	    HGLOBAL hGContainer = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, itemSize);
		if (!hGContainer)
			return rootfile;

		LPVOID pBuf = ::GlobalLock(hGContainer);
		
		bool b = false;
		if (pBuf)
			b = _z->UnzipItemToMembuffer(dex, pBuf, itemSize);

		if (::GlobalUnlock(hGContainer) != 0 || GetLastError() != NO_ERROR || !b)
			return rootfile;

		std::string xmlContent = (char*)pBuf;

		size_t posStart = xmlContent.find("rootfile ");

		if (posStart == std::string::npos)
			return rootfile;

		posStart = xmlContent.find("full-path=\"", posStart);

		if (posStart == std::string::npos)
			return rootfile;

		posStart += 11;
		size_t posEnd = xmlContent.find("\"", posStart);

		rootfile = xmlContent.substr(posStart, posEnd - posStart);
		
		return rootfile;
	}

	std::string metaCover(char *pBuf)
	{
		std::string xmlContent = pBuf;

		// Find meta tag for cover
		// I.e. searching for '<meta name="cover" content="cover"/>'

		std::string coverTag, coverId, itemTag;

		size_t posStart = xmlContent.find("name=\"cover\"");

		if (posStart == std::string::npos) {
			return coverId;
		}

		posStart = xmlContent.find_last_of("<", posStart);
		size_t posEnd = xmlContent.find(">", posStart);

		coverTag = xmlContent.substr(posStart, posEnd - posStart + 1);

		// Find cover item id

		posStart = coverTag.find("content=\"");

		if (posStart == std::string::npos) {
			return coverId;
		}

		posStart += 9;
		posEnd = coverTag.find("\"", posStart);

		coverId = coverTag.substr(posStart, posEnd - posStart);
		return coverId;
	}

	std::string coverImageItem(char* pBuf, std::string rootpath) {

		// find a <manifest> entry with id of "cover-image"
		// the href is the path, relative to rootpath

		std::string xmlContent = (char*)pBuf;
		size_t posManifest = xmlContent.find("<manifest>");
		if (posManifest == std::string::npos) return std::string();

		size_t posStart = xmlContent.find("id=\"cover-image\"", posManifest);
		if (posStart == std::string::npos) return std::string();

		// found it, move backward to find owning "<item"
		size_t posItem = xmlContent.rfind("<item ", posStart);
		if (posItem == std::string::npos) return std::string();

		// find the href
		size_t posHref = xmlContent.find("href=\"", posItem);  // TODO this might not match in *this* item
		posHref += 6;
		size_t posEnd  = xmlContent.find("\"", posHref);

		return rootpath + xmlContent.substr(posHref, posEnd - posHref);
	}

	std::string coverHTML(char* pBuf, std::string rootpath) {



		// 1. find a <manifest> entry with id either "cover" or "icover"
		// 2. get the href= file
		// 3. look in the file for an <img> tag
		// 4. the cover file is the rootpath + the path from the <img src=""> 

		return std::string();
	}

	HRESULT ExtractEpub(HBITMAP* phBmpThumbnail)
	{
		std::string xmlContent, rootpath, coverfile;

		CUnzip _z;
		if (!_z.Open(m_cbxFile)) return E_FAIL;
		int j = _z.GetItemCount();
		if (j == 0) return E_FAIL;

		std::string rootfile = GetEpubRootFile(&_z);

		size_t posStart, posEnd;

		USES_CONVERSION;

		if (rootfile.length() <= 0)
			goto test_coverfile;

		posStart = rootfile.rfind('/');
		if (posStart != std::string::npos) {
			rootpath = rootfile.substr(0, posStart + 1);
		}

		int dex = -1;
		ZIPENTRY ze;
		memset(&ze, 0, sizeof(ZIPENTRY));
		FindZipItem(_z.getHZIP(), A2T(rootfile.c_str()), false, &dex, &ze);
		if (dex < 0)
			goto test_coverfile;

		_z.GetItem(dex);
		int i = dex;

		HGLOBAL hGContainer = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, (SIZE_T)_z.GetItemUnpackedSize());
		if (hGContainer)
		{
			bool b = false;
			LPVOID pBuf = ::GlobalLock(hGContainer);
			if (pBuf)
				b = _z.UnzipItemToMembuffer(i, pBuf, _z.GetItemUnpackedSize());

			if (::GlobalUnlock(hGContainer) == 0 && GetLastError() == NO_ERROR && b)
			{
				// Find meta tag for cover
				// I.e. searching for '<meta name="cover" content="cover"/>'
				std::string coverId = metaCover((char*)pBuf);

				if (coverId.empty()) {

					// Some books [esp. calibre books] don't have a "cover" meta tag.
					// First test for a <manifest> cover item with id="cover-image"
					coverfile = coverImageItem((char*)pBuf, rootpath);
					if (!coverfile.empty()) {
						goto test_coverfile;
					}

					// Search for a <manifest> cover item and an image path via HTML file
					coverfile = coverHTML((char*)pBuf, rootpath);
					goto test_coverfile;
				}

				// Find item tag in original opf file contents
				// I.e. searching for '<item href="cover.jpeg" id="cover" media-type="image/jpeg"/>'. The id
				// value matching the "content" value from before.

				xmlContent = (char*)pBuf;
				posStart = xmlContent.find("id=\"" + coverId + "\"");
				if (posStart != std::string::npos)
				{
					posStart = xmlContent.find_last_of("<", posStart);
					posEnd = xmlContent.find(">", posStart);

					std::string itemTag = xmlContent.substr(posStart, posEnd - posStart + 1);

					// Find cover path in item tag

					posStart = itemTag.find("href=\"");

					if (posStart != std::string::npos)
					{
						posStart += 6;
						posEnd = itemTag.find("\"", posStart);

						if (posEnd != std::string::npos)
						{
							coverfile = rootpath + itemTag.substr(posStart, posEnd - posStart);
							ReplaceStringInPlace(coverfile, "%20", " ");
						}
					}
				}

				// if there is no matching item id, check to see if 
				// coverId may specify the actual image as found in some books
				// I.e. '<meta name="cover" content="Images/cover.jpg" />'
				// This is a "fall-through" from above because some books use an image path as the id,
				// e.g. '<meta content="cover.jpg" name="cover"/>'
				if (coverfile.empty() && IsImage(A2T(coverId.c_str())))
				{
					coverfile = rootpath + coverId;
					ReplaceStringInPlace(coverfile, "%20", " ");
					goto test_coverfile;
				}

			}
		}
			
test_coverfile:
		if (coverfile.empty()) {

			// No cover file specified, do a brute-force search for the first file with "cover" in the name.
			for (int i = 0; i < j; i++) {
				if (!_z.GetItem(i)) break;
				if (_z.ItemIsDirectory() || (_z.GetItemUnpackedSize() > CBXMEM_MAXBUFFER_SIZE)) continue;
				if ((_z.GetItemPackedSize() == 0) || (_z.GetItemUnpackedSize() == 0)) continue;

				LPCTSTR nm = _z.GetItemName();
				if (IsImage(nm))
				{
					if (_tcsstr(nm, _T("cover")) != NULL ||
						_tcsstr(nm, _T("COVER")) != NULL ||
						_tcsstr(nm, _T("Cover")) != NULL)
					{
						coverfile = T2A(nm);
						break;
					}
				}
			}
		}

		if (!coverfile.empty()) {

			int thumbindex;
			ZIPENTRY ze;
			ZRESULT res = FindZipItem(_z.getHZIP(), A2T(coverfile.c_str()), true, &thumbindex, &ze);

			if (thumbindex < 0) return E_FAIL;
			//go to thumb index
			if (!_z.GetItem(thumbindex)) return E_FAIL;

			//create thumb			//GHND
			HGLOBAL hG = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, (SIZE_T)_z.GetItemUnpackedSize());
			if (hG)
			{
				bool b = false;
				LPVOID pBuf = ::GlobalLock(hG);
				if (pBuf)
					b = _z.UnzipItemToMembuffer(thumbindex, pBuf, _z.GetItemUnpackedSize());

				if (::GlobalUnlock(hG) == 0 && GetLastError() == NO_ERROR)
				{
					if (b)
					{
						IStream* pIs = NULL;
						if (S_OK == CreateStreamOnHGlobal(hG, TRUE, (LPSTREAM*)&pIs))//autofree hG
						{
							*phBmpThumbnail = ThumbnailFromIStream(pIs, &m_thumbSize, m_showIcon);
							pIs->Release();
							pIs = NULL;
						}
					}
				}
				GlobalFree(hG);
			}
			return ((*phBmpThumbnail) ? S_OK : E_FAIL);
		}
		return E_FAIL;
	}

	////////////////////////////////////
	//IExtractImage::Extract(HBITMAP* phBmpThumbnail)
	HRESULT OnExtract(HBITMAP* phBmpThumbnail)
	{
		*phBmpThumbnail=NULL;
		//ATLTRACE("IExtractImage::Extract\n");
try {
		switch (m_cbxType)
		{
		case CBXTYPE_MOBI:
			return ExtractMobiCover(m_cbxFile, phBmpThumbnail);

		case CBXTYPE_EPUB:
		{
			if (ExtractEpub(phBmpThumbnail) != E_FAIL)
				return S_OK;

			// something wrong with the epub, try falling back on first image in zip
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

					if (IsImage(_z.GetItemName()))
					{
						if (thumbindex<0) thumbindex=i;// assign thumbindex if already sorted

						if (!m_bSort) break;//if NoSort
						
						if (prevname.IsEmpty()) prevname=_z.GetItemName();//can't compare empty string
						//take only first alphabetical name
						if (-1==StrCmpLogicalW(_z.GetItemName(), prevname))
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
								*phBmpThumbnail= ThumbnailFromIStream(pIs, &m_thumbSize, m_showIcon);
								pIs->Release();
								pIs=NULL;
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
						if (_r.ProcessItem()) *phBmpThumbnail= ThumbnailFromIStream(pIs, &m_thumbSize, m_showIcon);
					}
				}
				GlobalFree(hG);
				pIs->Release();
			return ((*phBmpThumbnail) ? S_OK : E_FAIL );
			}//dtors!
		break;
		default:return E_FAIL;
		}
}
catch (...){ ATLTRACE("exception in IExtractImage::Extract\n"); return S_FALSE;}
	return S_OK;
	}

	CString GetEpubTitle(LPCTSTR ePubFile)
	{
		CString title;
		CUnzip _z;
		if (!_z.Open(m_cbxFile)) return title;
		int j = _z.GetItemCount();
		if (j == 0) return title;

		HZIP zipHandle = _z.getHZIP();

		std::string rootfile = GetEpubRootFile(&_z);
		if (rootfile.empty()) return title;

		USES_CONVERSION;

		HGLOBAL hGContainer = NULL;

		int dex = -1;
		ZIPENTRY ze;
		memset(&ze, 0, sizeof(ZIPENTRY));
		ZRESULT zr = FindZipItem(zipHandle, A2T(rootfile.c_str()), false, &dex, &ze);
		if (zr == ZR_OK && dex >= 0)
		{
			_z.GetItem(dex);
			int i = dex;
			size_t itemSize = _z.GetItemUnpackedSize();
			hGContainer = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, itemSize);
			if (hGContainer)
			{
				bool b = false;
				LPVOID pBuf = ::GlobalLock(hGContainer);
				if (pBuf)
					b = _z.UnzipItemToMembuffer(i, pBuf, (unsigned int)itemSize);

				if (::GlobalUnlock(hGContainer) == 0 && GetLastError() == NO_ERROR)
				{
					if (b)
					{
						// UTF-8 input to wchar

						CAtlString cstr;
						LPWSTR ptr = cstr.GetBuffer((int)(itemSize + 1));

						int newLen = MultiByteToWideChar(
							CP_UTF8, 0,
							(LPCCH)pBuf, (int)itemSize, ptr, ((int)itemSize + 1)
						);
						if (!newLen)
						{
							cstr.ReleaseBuffer(0);
							goto on_exit;
						}
						cstr.ReleaseBuffer();

						// Find <dc:title> tag

						int tstart = cstr.Find(_T("<dc:title"));
						if (tstart == -1) // nope
							goto on_exit;

						int tstart2 = cstr.Find(_T('>'), tstart); // KBR may be of the form '<dc:title id="title">'
						int tend = cstr.Find(_T("</dc:title>"), tstart2);

						title = cstr.Mid(tstart2+1, tend-tstart2-1);
					}
				}
			}
		}

	on_exit:
		if (hGContainer)
			GlobalFree(hGContainer);
		return title;
	}

	//////////////////////////////
	//IQueryInfo::GetInfoTip(DWORD dwFlags, LPWSTR *ppwszTip)
	HRESULT OnGetInfoTip(LPWSTR *ppwszTip)
	{
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
catch (...){ ATLTRACE("exception in IQueryInfo::GetInfoTip\n"); return S_FALSE;}

	return S_FALSE;
	}


private:
	CString m_cbxFile;//overcome MAX_PATH limit?
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

	inline BOOL StrEqual(LPCTSTR psz1,LPCTSTR psz2) {return (::StrCmpI(psz1, psz2)==0);}

	BOOL IsImage(LPCTSTR szFile)
	{
		LPWSTR _e=PathFindExtension(szFile);
		if (StrEqual(_e, _T(".bmp")))  return TRUE;
		if (StrEqual(_e, _T(".ico")))  return TRUE;
		if (StrEqual(_e, _T(".gif")))  return TRUE;
		if (StrEqual(_e, _T(".jpg")))  return TRUE;
		if (StrEqual(_e, _T(".jpe")))  return TRUE;
		if (StrEqual(_e, _T(".jfif"))) return TRUE;
		if (StrEqual(_e, _T(".jpeg"))) return TRUE;
		if (StrEqual(_e, _T(".png")))  return TRUE;
		if (StrEqual(_e, _T(".tif")))  return TRUE;
		if (StrEqual(_e, _T(".tiff"))) return TRUE;
		if (StrEqual(_e, _T(".webp"))) return TRUE;  // NOTE: works if a webp codec is installed
	return FALSE;
	}

	inline CBXTYPE GetCBXType(LPCTSTR szExt)
	{
		if (StrEqual(szExt, _T(".cbz"))) return CBXTYPE_CBZ;
		if (StrEqual(szExt, _T(".zip"))) return CBXTYPE_ZIP;
		if (StrEqual(szExt, _T(".cbr"))) return CBXTYPE_CBR;
		if (StrEqual(szExt, _T(".rar"))) return CBXTYPE_RAR;
		if (StrEqual(szExt, _T(".epub"))) return CBXTYPE_EPUB;
		if (StrEqual(szExt, _T(".phz"))) return CBXTYPE_CBZ;
		if (StrEqual(szExt, _T(".mobi"))) return CBXTYPE_MOBI;
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
