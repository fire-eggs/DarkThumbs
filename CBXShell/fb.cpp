/*
* DarkThumbs - Windows Explorer thumbnails for ebooks, image archives
* Copyright © 2020-2022 by Kevin Routley. All rights reserved.
* 
* fb2 file support
*/
#include "StdAfx.h"
#include <string>
#include <algorithm>
#include <iostream>
#include <fstream>  // std::ifstream
#include <sstream>
#include <codecvt>
#include <vector>
#include <thumbcache.h> // WTS_ALPHATYPE
#include <atlstr.h>

HRESULT GetStreamFromString(PCWSTR pszImageName, IStream** ppImageStream);
HRESULT WICCreate32BitsPerPixelHBITMAP(IStream* pstm, HBITMAP* phbmp);

std::wstring readFile2(CStringW fileName)
{
	wchar_t* blah = const_cast<wchar_t*>((LPCTSTR)fileName);

	std::wifstream ifs(blah, std::ios::in | std::ios::binary | std::ios::ate);

	std::wifstream::pos_type fileSize = ifs.tellg();
	ifs.seekg(0, std::ios::beg);

	std::vector<wchar_t> bytes(fileSize);
	ifs.read(bytes.data(), fileSize);

	return std::wstring(bytes.data(), fileSize);
}

// The XML consists of
// <binary id="image-id" ...>base64-data</binary>
// So we search for id="image-id", then the trailing > and then the </binary>.
// Take the base64 data and convert to an image.
HRESULT imageFromId(const std::wstring& fb, const std::wstring& imgid, HBITMAP* phbmp)
{
	// search for id="imgid"
	std::wstring target = L"id=\"" + imgid + L"\"";
	size_t imgP = fb.find(target);
	if (imgP == std::string::npos)
		return E_FAIL;

	// find the trailing >
	size_t endTag = fb.find(L">", imgP);
	// from there to </binary>
	size_t endBinary = fb.find(L"</binary>", endTag);

	// base64 to image
	std::wstring base64 = fb.substr(endTag + 1, endBinary - endTag - 1);

	IStream* pImageStream;
	HRESULT hr = GetStreamFromString(base64.c_str(), &pImageStream);
	if (SUCCEEDED(hr))
	{
		hr = WICCreate32BitsPerPixelHBITMAP(pImageStream, phbmp);
		//*phbmp = ThumbnailFromIStream(pImageStream, &m_thumbSize, showIcon);
		pImageStream->Release();
	}
	return hr; // ((*phBmp) ? S_OK : E_FAIL);
}

//HRESULT ExtractFBCover(const std::wstring& filepath, HBITMAP* phBmpThumbnail)
HRESULT ExtractFBCover(CString filepath, HBITMAP* phBmpThumbnail)
{
	try {
		//std::wstring file = readFile(filepath);
		std::wstring file = readFile2(filepath);

		if (file.find(L"<FictionBook") == std::string::npos)
			return E_FAIL; // not a valid fb file

		size_t cp_pos = file.find(L"<coverpage>");
		if (cp_pos == std::string::npos)
			return E_FAIL; // no cover page

		size_t im_pos = file.find(L"<image", cp_pos);
		if (im_pos == std::string::npos)
			return E_FAIL; // no <image> tag

		size_t idStart = file.find(L"\"", im_pos);
		size_t idEnd = file.find(L"\"", idStart + 1);
		std::wstring imgid = file.substr(idStart + 1, idEnd - idStart - 1);
		if (imgid[0] == L'#')
			imgid = imgid.substr(1);

		return imageFromId(file, imgid, phBmpThumbnail);

	}
	catch (...)
	{
		// any exception from reading the file: bail
		//LOGERR(L"FB Exception");
		return E_FAIL;
	}

	return E_FAIL;
}