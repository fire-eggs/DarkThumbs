#include "StdAfx.h"
#include <atlimage.h>
#include <wincodec.h>   // Windows Imaging Codecs
#include <thumbcache.h> // WTS_ALPHATYPE

#include "resource.h"   // zip icon

#define LOGDBG
#define LOGERR

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
	if (StrEqual(_e, _T(".svg"))) return TRUE;  // NOTE: hopefully works if a codec is installed?

	// The following require a WIC codec to be installed
	if (StrEqual(_e, _T(".webp"))) return TRUE;
	if (StrEqual(_e, _T(".jxr"))) return TRUE;
	if (StrEqual(_e, _T(".nrw"))) return TRUE;
	if (StrEqual(_e, _T(".nef"))) return TRUE;
	if (StrEqual(_e, _T(".dng"))) return TRUE;
	if (StrEqual(_e, _T(".cr2"))) return TRUE;

	// Issues # 83, # 82
	if (StrEqual(_e, _T(".heif"))) return TRUE;
	if (StrEqual(_e, _T(".heic"))) return TRUE;
	if (StrEqual(_e, _T(".avif"))) return TRUE;
	if (StrEqual(_e, _T(".jxl"))) return TRUE;

	return FALSE;
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

extern HINSTANCE _hInstance;
extern void __cdecl logit(LPCWSTR format, ...);

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
		{
			logit(_T("showIcon"));
			if (zipIcon == NULL)
			{
				logit(_T("zipIcon NULL"));
				zipIcon = LoadIcon(_hInstance, MAKEINTRESOURCE(IDI_ICON1));
			}
			BOOL res = DrawIcon(hdcNew, 0, 0, zipIcon);
			if (!res)
				logit(_T("DrawIcon fail"));
		}

		hdcNew.SelectBitmap(hbmpOld);

		return hbmpNew.Detach();
	}

	return ci.Detach();
}

// Decodes the base64-encoded string to a stream.
HRESULT GetStreamFromString(PCWSTR pszImageName, IStream** ppImageStream)
{
	HRESULT hr = E_FAIL;

	DWORD dwDecodedImageSize = 0;
	DWORD dwSkipChars = 0;
	DWORD dwActualFormat = 0;

	// Base64-decode the string
	BOOL fSuccess = CryptStringToBinaryW(pszImageName, NULL, CRYPT_STRING_BASE64,
		NULL, &dwDecodedImageSize, &dwSkipChars, &dwActualFormat);
	if (fSuccess)
	{
		BYTE* pbDecodedImage = (BYTE*)LocalAlloc(LPTR, dwDecodedImageSize);
		if (pbDecodedImage)
		{
			fSuccess = CryptStringToBinaryW(pszImageName, lstrlenW(pszImageName), CRYPT_STRING_BASE64,
				pbDecodedImage, &dwDecodedImageSize, &dwSkipChars, &dwActualFormat);
			if (fSuccess)
			{
				*ppImageStream = SHCreateMemStream(pbDecodedImage, dwDecodedImageSize);
				if (*ppImageStream != NULL)
				{
					hr = S_OK;
				}
			}
			LocalFree(pbDecodedImage);
		}
	}
	return hr;
}


extern void __cdecl logit(LPCWSTR format, ...);

// Decodes the base64-encoded string to a stream.
// This version takes ASCII input and a length limit [see epub.cpp, "Standard Ebook" handling]
HRESULT GetStreamFromStringA(PCSTR pszImageName, IStream** ppImageStream, DWORD lenLimit)
{
	HRESULT hr = E_FAIL;

	DWORD dwDecodedImageSize = lenLimit;
	DWORD dwSkipChars = 0;
	DWORD dwActualFormat = 0;

	// Base64-decode the string
	BOOL fSuccess = CryptStringToBinaryA(pszImageName, NULL, CRYPT_STRING_BASE64,
		NULL, &dwDecodedImageSize, &dwSkipChars, &dwActualFormat);
	if (fSuccess)
	{
		HGLOBAL hGCont = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, dwDecodedImageSize);
		if (!hGCont)
			return hr;
		LPVOID pbuf = GlobalLock(hGCont);

		if (pbuf)
		{
			fSuccess = CryptStringToBinaryA(pszImageName, lstrlenA(pszImageName), CRYPT_STRING_BASE64,
				(BYTE *)pbuf, &dwDecodedImageSize, &dwSkipChars, &dwActualFormat);
			if (fSuccess)
			{
				*ppImageStream = SHCreateMemStream((BYTE *)pbuf, dwDecodedImageSize);
				if (*ppImageStream != NULL)
				{
					hr = S_OK;
				}
			}
			GlobalUnlock(hGCont);
			GlobalFree(hGCont);
		}
	}
	return hr;
}

HRESULT ConvertBitmapSourceTo32BPPHBITMAP(IWICBitmapSource* pBitmapSource,
	IWICImagingFactory* pImagingFactory,
	HBITMAP* phbmp)
{
	*phbmp = NULL;

	IWICBitmapSource* pBitmapSourceConverted = NULL;
	WICPixelFormatGUID guidPixelFormatSource;
	HRESULT hr = pBitmapSource->GetPixelFormat(&guidPixelFormatSource);
	if (SUCCEEDED(hr) && (guidPixelFormatSource != GUID_WICPixelFormat32bppBGRA))
	{
		IWICFormatConverter* pFormatConverter;
		hr = pImagingFactory->CreateFormatConverter(&pFormatConverter);
		if (SUCCEEDED(hr))
		{
			// Create the appropriate pixel format converter
			hr = pFormatConverter->Initialize(pBitmapSource, GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone, NULL, 0, WICBitmapPaletteTypeCustom);
			if (SUCCEEDED(hr))
			{
				hr = pFormatConverter->QueryInterface(&pBitmapSourceConverted);
			}
			pFormatConverter->Release();
		}
	}
	else
	{
		hr = pBitmapSource->QueryInterface(&pBitmapSourceConverted);  // No conversion necessary
	}

	if (SUCCEEDED(hr))
	{
		UINT nWidth, nHeight;
		hr = pBitmapSourceConverted->GetSize(&nWidth, &nHeight);
		if (SUCCEEDED(hr))
		{
			BITMAPINFO bmi = {};
			bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
			bmi.bmiHeader.biWidth = nWidth;
			bmi.bmiHeader.biHeight = -static_cast<LONG>(nHeight);
			bmi.bmiHeader.biPlanes = 1;
			bmi.bmiHeader.biBitCount = 32;
			bmi.bmiHeader.biCompression = BI_RGB;

			BYTE* pBits;
			HBITMAP hbmp = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, reinterpret_cast<void**>(&pBits), NULL, 0);
			hr = hbmp ? S_OK : E_OUTOFMEMORY;
			if (SUCCEEDED(hr))
			{
				WICRect rect = { 0, 0, static_cast<INT>(nWidth), static_cast<INT>(nHeight) };

				// Convert the pixels and store them in the HBITMAP.  Note: the name of the function is a little
				// misleading - we're not doing any extraneous copying here.  CopyPixels is actually converting the
				// image into the given buffer.
				hr = pBitmapSourceConverted->CopyPixels(&rect, nWidth * 4, nWidth * nHeight * 4, pBits);
				if (SUCCEEDED(hr))
				{
					*phbmp = hbmp;
				}
				else
				{
					DeleteObject(hbmp);
				}
			}
		}
		pBitmapSourceConverted->Release();
	}
	return hr;
}

// For debugging purposes
#if 0
HRESULT WICCreate32BitsPerPixelHBITMAP_log(IStream* pstm, HBITMAP* phbmp)
{
	logit(L"WC32BPPH 0");

	*phbmp = NULL;

	IWICImagingFactory* pImagingFactory;
	HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pImagingFactory));
	if (SUCCEEDED(hr))
	{
		logit(L"WC32BPPH 1");

		IWICBitmapDecoder* pDecoder;
		hr = pImagingFactory->CreateDecoderFromStream(pstm, &GUID_VendorMicrosoft, WICDecodeMetadataCacheOnDemand, &pDecoder);
		if (SUCCEEDED(hr))
		{
			logit(L"WC32BPPH 2");
			IWICBitmapFrameDecode* pBitmapFrameDecode;
			hr = pDecoder->GetFrame(0, &pBitmapFrameDecode);
			if (SUCCEEDED(hr))
			{
				logit(L"WC32BPPH 3");
				hr = ConvertBitmapSourceTo32BPPHBITMAP(pBitmapFrameDecode, pImagingFactory, phbmp);
				pBitmapFrameDecode->Release();
			}
			pDecoder->Release();
		}
		pImagingFactory->Release();
	}
	logit(L"WC32BPPH 4");
	return hr;
}
#endif

HRESULT WICCreate32BitsPerPixelHBITMAP(IStream* pstm, HBITMAP* phbmp)
{
	*phbmp = NULL;

	IWICImagingFactory* pImagingFactory;
	HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pImagingFactory));
	if (SUCCEEDED(hr))
	{
		IWICBitmapDecoder* pDecoder;
		hr = pImagingFactory->CreateDecoderFromStream(pstm, &GUID_VendorMicrosoft, WICDecodeMetadataCacheOnDemand, &pDecoder);
		if (SUCCEEDED(hr))
		{
			IWICBitmapFrameDecode* pBitmapFrameDecode;
			hr = pDecoder->GetFrame(0, &pBitmapFrameDecode);
			if (SUCCEEDED(hr))
			{
				hr = ConvertBitmapSourceTo32BPPHBITMAP(pBitmapFrameDecode, pImagingFactory, phbmp);
				pBitmapFrameDecode->Release();
			}
			pDecoder->Release();
		}
		pImagingFactory->Release();
	}
	return hr;
}

void addIcon(HBITMAP* phBmpThumbnail)
{
	if (*phBmpThumbnail)
	{
		HDC hdcMem = CreateCompatibleDC(NULL);

		BITMAP bm;
		int res2 = GetObject(*phBmpThumbnail, (int)sizeof(bm), &bm);
		//logit(_T("[%d]Dim(%ld x %ld)"), res2, bm.bmWidth, bm.bmHeight);

		// Issue #71: don't have the icon change size based on original image size
		int outW = bm.bmWidth / 3;
		int outH = bm.bmHeight / 5;
		outW = outW > outH ? outH : outW; // same height/width
		//outH = outH > outW ? outW : outH;

		HGDIOBJ hbmOld = SelectObject(hdcMem, *phBmpThumbnail);
		zipIcon = LoadIcon(_hInstance, MAKEINTRESOURCE(IDI_ICON1));
		BOOL res = DrawIconEx(hdcMem, 0, 10, zipIcon, outW, outW, 0, NULL, DI_NORMAL);
		SelectObject(hdcMem, hbmOld);
		DeleteDC(hdcMem);
	}
}
