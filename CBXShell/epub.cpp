/*
* DarkThumbs - Windows Explorer thumbnails for ebooks, image archives
* Copyright © 2020-2022 by Kevin Routley. All rights reserved.
*
* epub file support
*/

#ifndef STRICT
#define STRICT
#endif

#include "StdAfx.h"

#include <string>
#include "cunzip.h"
#include <atlstr.h>

#define CBXMEM_MAXBUFFER_SIZE 33554432 //32mb

HBITMAP ThumbnailFromIStream(IStream* pIs, const LPSIZE pThumbSize, bool showIcon);
BOOL IsImage(LPCTSTR szFile);
HRESULT WICCreate32BitsPerPixelHBITMAP(IStream* pstm, HBITMAP* phbmp);
HRESULT WICCreate32BitsPerPixelHBITMAP_log(IStream* pstm, HBITMAP* phbmp);
HRESULT GetStreamFromStringA(PCSTR pszImageName, IStream** ppImageStream, DWORD lenLimit);

extern void __cdecl logit(LPCWSTR format, ...);


std::string urlDecode(std::string& SRC)
{
	std::string ret;
	for (int i = 0; i < SRC.length(); i++)
	{
		if (int(SRC[i]) == 37) // 37 is '%'
		{
			int ii;
			sscanf(SRC.substr(i + 1, 2).c_str(), "%x", &ii);
			char ch = static_cast<char>(ii);
			ret += ch;
			i = i + 2;
		}
		else
		{
			ret += SRC[i];
		}
	}
	return (ret);
}

std::string metaCover(char* pBuf)
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

// EPUP3 standard: open the container.xml file to fetch the path of the rootfile
// Requires a successfully opened and not empty zipfile
std::string GetEpubRootFile(CUnzip* _z)
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
		goto exitGRF;

	{
		std::string xmlContent = (char*)pBuf;

		size_t posStart = xmlContent.find("rootfile ");

		if (posStart == std::string::npos)
			goto exitGRF;

		posStart = xmlContent.find("full-path=\"", posStart);

		if (posStart == std::string::npos)
			goto exitGRF;

		posStart += 11;
		size_t posEnd = xmlContent.find("\"", posStart);

		rootfile = xmlContent.substr(posStart, posEnd - posStart);
	}

exitGRF:
	GlobalFree(hGContainer);
	return rootfile;
}

CString GetEpubTitle(CString m_cbxFile)
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

					title = cstr.Mid(tstart2 + 1, tend - tstart2 - 1);
				}
			}
		}
	}

on_exit:
	if (hGContainer)
		GlobalFree(hGContainer);
	return title;
}

std::string coverImageItem(char* pBuf, std::string rootpath) {

	// find a <manifest> entry with id of "cover-image" [for Epub V3.0]
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
	size_t posEnd = xmlContent.find("\"", posHref);

	return rootpath + xmlContent.substr(posHref, posEnd - posHref);
}

std::string coverHTML(char* pBuf, std::string rootpath) {



	// 1. find a <manifest> entry with id either "cover" or "icover"
	// 2. get the href= file
	// 3. look in the file for an <img> tag
	// 4. the cover file is the rootpath + the path from the <img src=""> 

	return std::string();
}



// Attempt to handle a coverfile from Standard Ebooks
HRESULT TryStandardEbooks(char *pBuf, long itemSize, HBITMAP *phbmp)
{
	// The XML file has an embedded, base64-encoded jpeg file. It looks
	// like: "xlink:href="data:image/jpeg;base64,.../>" where ... 
	// represents the base64-encoded characters.

	// So first find the bracketing characters from within the buffer
	// (which contains the XML in entirety). 
	const char* find = "data:image/jpeg;base64,";
	auto res = StrStrIA(pBuf, find);
	if (res == NULL)
		return NULL;
	const char* find2 = "\"/>";
	auto res2 = StrStrIA(res, find2);
	if (res2 == NULL)
		return NULL;

	// the beginning of the data proper is from the search results plus
	// the length of the start string. the length of the base64 characters
	// comes from the start of the end-bracket-string.
	auto base64 = res + 23;
	DWORD lenLimit = (DWORD)(res2 - base64);

	// We'll now get a binary stream from the decoded base64 characters.
	IStream* pImageStream0 = NULL;
	HRESULT hr = GetStreamFromStringA(base64, &pImageStream0, lenLimit);
	if (SUCCEEDED(hr))
	{
		// and decode the binary stream as an image
		hr = WICCreate32BitsPerPixelHBITMAP(pImageStream0, phbmp);
		pImageStream0->Release();
	}

	// TODO the second part is to take the SVG file, render it into a bitmap,
	// and merge the two bitmaps.

	return hr;
}

inline bool ends_with(std::string const& value, std::string const& ending)
{
	if (ending.size() > value.size()) return false;
	return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

HRESULT ExtractEpub(CString m_cbxFile, HBITMAP* phBmpThumbnail, SIZE m_thumbSize, BOOL showIcon)
{
	HGLOBAL hGContainer = NULL;

	std::string xmlContent, rootpath, coverfile;

	CUnzip _z;
	if (!_z.Open(m_cbxFile)) return E_FAIL;
	int j = _z.GetItemCount();
	if (j == 0) return E_FAIL;

	std::string rootfile = GetEpubRootFile(&_z);

	size_t posStart, posEnd;

	USES_CONVERSION;

	if (rootfile.length() <= 0)
		goto test_coverfile; // No root file path found

	posStart = rootfile.rfind('/');
	if (posStart != std::string::npos) {
		rootpath = rootfile.substr(0, posStart + 1);
	}

	// Get the root file itself
	int dex = -1;
	ZIPENTRY ze;
	memset(&ze, 0, sizeof(ZIPENTRY));
	FindZipItem(_z.getHZIP(), A2T(rootfile.c_str()), false, &dex, &ze);
	if (dex < 0)
		goto test_coverfile; // Root file not found

	_z.GetItem(dex);
	int i = dex;

	hGContainer = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, (SIZE_T)_z.GetItemUnpackedSize());
	if (hGContainer)
	{
		bool b = false;
		LPVOID pBuf = ::GlobalLock(hGContainer);
		if (pBuf)
			b = _z.UnzipItemToMembuffer(i, pBuf, _z.GetItemUnpackedSize());

		// If successful, pBuf now contains the contents of the root file.
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
						std::string coverfile0 = rootpath + itemTag.substr(posStart, posEnd - posStart);
						coverfile = urlDecode(coverfile0);
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
				std::string coverfile0 = rootpath + coverId;
				coverfile = urlDecode(coverfile0);
			}

		}
	}

test_coverfile:
	if (hGContainer) GlobalFree(hGContainer); // done with the rootfile

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

	if (!coverfile.empty()) {  // testing "again" because brute-force search might have found something

		int thumbindex;
		ZIPENTRY ze;
		ZRESULT res = FindZipItem(_z.getHZIP(), A2T(coverfile.c_str()), true, &thumbindex, &ze);

		if (thumbindex < 0) return E_FAIL;  // couldn't find coverfile by path
		//go to thumb index
		if (!_z.GetItem(thumbindex)) return E_FAIL; // can't unpack coverfile

		//create thumb			//GHND
		HGLOBAL hG = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, (SIZE_T)_z.GetItemUnpackedSize());
		if (hG)
		{
			bool b = false;
			LPVOID pBuf = ::GlobalLock(hG);
			long itemSize = _z.GetItemUnpackedSize();
			if (pBuf)
				b = _z.UnzipItemToMembuffer(thumbindex, pBuf, itemSize);

			// at this point pBuf has a copy of the coverfile bytes
			if (::GlobalUnlock(hG) == 0 && GetLastError() == NO_ERROR && b)
			{
				// Issue #34. Epub3 ebooks produced by "Standard Ebooks" (standardebooks.org) use a SVG file
				// as their cover image. These SVG files have two major parts: 1) a base64-encoded JPEG file,
				// and 2) rendering of text and author information on top of the JPEG.
				//
				// At this time, WIC doesn't support SVG files. Must try to handle ourselves.
				//
				if (ends_with(coverfile, ".svg"))
				{
					HRESULT hr = TryStandardEbooks((char *)pBuf, itemSize, phBmpThumbnail);
					return hr;
				}
				IStream* pImageStream = SHCreateMemStream((const BYTE*)pBuf, itemSize);
				HRESULT hr = WICCreate32BitsPerPixelHBITMAP(pImageStream, phBmpThumbnail);
				pImageStream->Release();
			}
			GlobalFree(hG);
		}
		return ((*phBmpThumbnail) ? S_OK : E_FAIL);
	}
	return E_FAIL;
}
