
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

CString m_cbxFile; // Tester hack
BOOL m_bSort;      // Tester hack
SIZE m_thumbSize;  // Tester hack
int m_howFound;

	class CUnzip
	{
	public:
		CUnzip() { hz = NULL; }
		virtual ~CUnzip() { ::CloseZip(hz); }

	public:
		bool Open(LPCTSTR zfile)
		{
			if (zfile == NULL) return false;
			HZIP temp_hz = ::OpenZip(zfile, NULL);//try new
			if (temp_hz == NULL) return false;
			Close();//close old
			hz = temp_hz;
			if (ZR_OK != ::GetZipItem(hz, -1, &maindirEntry)) return false;
			return true;
		}

		bool GetItem(int zi)
		{
			zr = ::GetZipItem(hz, zi, &ZipEntry);
			return (ZR_OK == zr);
		}

		bool UnzipItemToMembuffer(int index, void* z, unsigned int len)
		{
			zr = ::UnzipItem(hz, index, z, len);
			return (ZR_OK == zr);
		}

		void Close()
		{
			CloseZip(hz);
			hz = NULL;//critical!
		}

		inline BOOL ItemIsDirectory() { return (BOOL)(CUnzip::GetItemAttributes() & 0x0010); }
		int GetItemCount() const { return maindirEntry.index; }
		long GetItemPackedSize() const { return ZipEntry.comp_size; }
		long GetItemUnpackedSize() const { return ZipEntry.unc_size; }
		DWORD GetItemAttributes() const { return ZipEntry.attr; }
		LPCTSTR GetItemName() { return ZipEntry.name; }

		HZIP getHZIP() { return hz; }

	private:
		ZIPENTRY ZipEntry, maindirEntry;
		HZIP hz;
		ZRESULT zr;
	};

	inline BOOL StrEqual(LPCTSTR psz1, LPCTSTR psz2) { return (::StrCmpI(psz1, psz2) == 0); }

	BOOL IsImage(LPCTSTR szFile)
	{
		LPWSTR _e = PathFindExtension(szFile);
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

	void ReplaceStringInPlace(std::string& subject, const std::string& search, const std::string& replace)
	{
		size_t pos = 0;
		while ((pos = subject.find(search, pos)) != std::string::npos)
		{
			subject.replace(pos, search.length(), replace);
			pos += replace.length();
		}
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

	HBITMAP ThumbnailFromIStream(IStream* pIs, const LPSIZE pThumbSize, bool showIcon=false)
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
	// KBR turn off
	//		if (m_showIcon)
	//			DrawIcon(hdcNew, 0, 0, zipIcon);

			hdcNew.SelectBitmap(hbmpOld);

			return hbmpNew.Detach();
		}

		return ci.Detach();
	}

	std::string GetEpubRootFile(CUnzip *_z)
	{
		std::string rootfile;

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

	HRESULT OnExtract(HBITMAP* phBmpThumbnail)
	{
		std::string xmlContent, rootpath, coverfile;
		m_howFound = 0;

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
						m_howFound = 4;
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
							m_howFound = 2;
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
					m_howFound = 3;
					goto test_coverfile;
				}

			}
		}
			
test_coverfile:
		if (coverfile.empty()) {

			m_howFound = 0;

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
						m_howFound = 1;
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
							*phBmpThumbnail = ThumbnailFromIStream(pIs, &m_thumbSize);
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

		int dex = -1;
		ZIPENTRY ze;
		memset(&ze, 0, sizeof(ZIPENTRY));
		ZRESULT zr = FindZipItem(zipHandle, A2T(rootfile.c_str()), false, &dex, &ze);
		if (zr != ZR_OK || dex < 0)
			return title;

		_z.GetItem(dex);
		unsigned int itemSize = _z.GetItemUnpackedSize();
		HGLOBAL hGContainer = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, itemSize);
		if (hGContainer)
		{
			bool b = false;
			LPVOID pBuf = ::GlobalLock(hGContainer);
			if (pBuf)
				b = _z.UnzipItemToMembuffer(dex, pBuf, itemSize);

			if (::GlobalUnlock(hGContainer) == 0 && GetLastError() == NO_ERROR && b)
			{
				// UTF-8 input to wchar

				CAtlString cstr;
				LPWSTR ptr = cstr.GetBuffer(itemSize + 1);

				int newLen = MultiByteToWideChar( CP_UTF8, 0,
					(LPCCH)pBuf, itemSize, ptr, itemSize + 1
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

	on_exit:
		if (hGContainer)
			GlobalFree(hGContainer);
		return title;
	}


	int dump_cover(const MOBIData* m, HBITMAP* phBmpThumbnail, SIZE thumbSize) {

		MOBIPdbRecord* record = NULL;
		MOBIExthHeader* exth = mobi_get_exthrecord_by_tag(m, EXTH_COVEROFFSET);
		if (exth) {
			uint32_t offset = mobi_decode_exthvalue((const unsigned char*)(exth->data), exth->size);
			size_t first_resource = mobi_get_first_resource_record(m);
			size_t uid = first_resource + offset;
			record = mobi_get_record_by_seqnumber(m, uid);
		}
		if (record == NULL || record->size < 4) {
			//printf("Cover not found\n");
			return E_FAIL;
		}

		HGLOBAL hG = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, (SIZE_T)record->size);
		if (hG)
		{
			bool b = false;
			LPVOID pBuf = ::GlobalLock(hG);
			if (pBuf)
				CopyMemory(pBuf, record->data, record->size);

			if (::GlobalUnlock(hG) == 0 && GetLastError() == NO_ERROR)
			{
				//if (b)
				{
					IStream* pIs = NULL;
					if (S_OK == CreateStreamOnHGlobal(hG, TRUE, (LPSTREAM*)&pIs))//autofree hG
					{
						*phBmpThumbnail = ThumbnailFromIStream(pIs, &thumbSize, false);
						pIs->Release();
						pIs = NULL;
					}
				}
			}
			GlobalFree(hG);
		}
		return ((*phBmpThumbnail) ? S_OK : E_FAIL);
	}

	HRESULT __cdecl loadfilename(LPCTSTR fullpath, HBITMAP* phBmpThumbnail, SIZE thumbSize) {
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
		FILE* file = _wfopen(fullpath, L"rb"); // TODO validate
		if (file == NULL) {
			int errsv = errno;
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

		ret = dump_cover(m, phBmpThumbnail, thumbSize);
		/* Free MOBIData structure */
		mobi_free(m);
		return ret;
	}

	HRESULT ExtractMobiCover(CString filepath, HBITMAP* phBmpThumbnail)
	{
		return loadfilename(filepath, phBmpThumbnail, m_thumbSize);
	}

