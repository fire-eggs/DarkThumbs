#include <string>
#include <algorithm>
#include <Windows.h>

#include <wincodec.h>   // Windows Imaging Codecs
#include <shlwapi.h>
#include <thumbcache.h>
#include <stdexcept>

#include "log.h"
#include "bitextractor.hpp"

using namespace bit7z;

const wchar_t* imgs[] = { L"bmp", L"ico", L"gif", L"jpg", L"jpeg", L"jpe", L"jfif", L"png", L"tif", L"tiff", L"webp"};

BOOL IsImage(std::wstring ext) // TODO is this pass-by-value?
{
	std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);

	for (auto foo : imgs)
		if (ext == foo)
			return TRUE;
	return FALSE;
}

void  SaveBitmap(char* szFilename, HBITMAP hBitmap)
{
    FILE*      fp = NULL;
    LPVOID     pBuf = NULL;
    BITMAPINFO bmpInfo;
    BITMAPFILEHEADER  bmpFileHeader;

    HDC hdc = GetDC(NULL);
    ZeroMemory(&bmpInfo, sizeof(BITMAPINFO));
    bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    GetDIBits(hdc, hBitmap, 0, 0, NULL, &bmpInfo, DIB_RGB_COLORS);
    if (bmpInfo.bmiHeader.biSizeImage <= 0)
        bmpInfo.bmiHeader.biSizeImage = bmpInfo.bmiHeader.biWidth * abs(bmpInfo.bmiHeader.biHeight) * (bmpInfo.bmiHeader.biBitCount + 7) / 8;

    if ((pBuf = malloc(bmpInfo.bmiHeader.biSizeImage)) == NULL)
    {
        if (hdc)     ReleaseDC(NULL, hdc);
        return;
    }

    bmpInfo.bmiHeader.biCompression = BI_RGB;
    GetDIBits(hdc, hBitmap, 0, bmpInfo.bmiHeader.biHeight, pBuf, &bmpInfo, DIB_RGB_COLORS);

    if ((fp = fopen(szFilename, "wb")) == NULL)
    {
        if (hdc)     ReleaseDC(NULL, hdc);
        if (pBuf)    free(pBuf);
        return;
    }

    bmpFileHeader.bfReserved1 = 0;
    bmpFileHeader.bfReserved2 = 0;
    bmpFileHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + bmpInfo.bmiHeader.biSizeImage;
    bmpFileHeader.bfType = 'MB';
    bmpFileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    fwrite(&bmpFileHeader, sizeof(BITMAPFILEHEADER), 1, fp);
    fwrite(&bmpInfo.bmiHeader, sizeof(BITMAPINFOHEADER), 1, fp);
    fwrite(pBuf, bmpInfo.bmiHeader.biSizeImage, 1, fp);

    if (hdc)     ReleaseDC(NULL, hdc);
    if (pBuf)    free(pBuf);
    if (fp)      fclose(fp);
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

HRESULT WICCreate32BitsPerPixelHBITMAP(IStream* pstm, UINT /* cx */, HBITMAP* phbmp, WTS_ALPHATYPE* pdwAlpha)
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
//                IWICBitmapScaler* pIScaler;
//                hr = pImagingFactory->CreateBitmapScaler(&pIScaler);
//                if (SUCCEEDED(hr))
//                {
//                    hr = pIScaler->Initialize(pBitmapFrameDecode, 256, 256, WICBitmapInterpolationMode::WICBitmapInterpolationModeHighQualityCubic);
//                    if (SUCCEEDED(hr))
//                    {
//                        hr = ConvertBitmapSourceTo32BPPHBITMAP(pIScaler, pImagingFactory, phbmp);
                        hr = ConvertBitmapSourceTo32BPPHBITMAP(pBitmapFrameDecode, pImagingFactory, phbmp);
                        if (SUCCEEDED(hr))
                        {
                            *pdwAlpha = WTSAT_ARGB;
                        }
                        else LOGDBG(L"CBST32BH fail");
//                    }
//                    else LOGDBG(L"scaler fail");
//                    pIScaler->Release();
//                }
                pBitmapFrameDecode->Release();
            }
            else LOGDBG(L"getframe fail");
            pDecoder->Release();
        }
        else LOGDBG(L"decoder fail");
        pImagingFactory->Release();
    }
    return hr;
}

HRESULT doBmp(wchar_t* path, int index, const BitInFormat* fmt, uint64_t size, HBITMAP *phbmp)
{
    vector<byte_t> bytes;

    Bit7zLibrary lib{};
    BitExtractor bex(lib, *fmt);
    bex.extract(path, bytes, index);

    BYTE* pbDecodedImage = (BYTE*)LocalAlloc(LPTR, size);
    std::copy(bytes.begin(), bytes.end(), pbDecodedImage);
    IStream* pImageStream = SHCreateMemStream(pbDecodedImage, (UINT)size);
    if (pImageStream == NULL)
    {
        LocalFree(pbDecodedImage);
        return E_FAIL;
    }
    LocalFree(pbDecodedImage);

    UINT cx = 256;
    WTS_ALPHATYPE pdwAlpha = WTSAT_UNKNOWN;
    HRESULT hr = WICCreate32BitsPerPixelHBITMAP(pImageStream, cx, phbmp, &pdwAlpha);
    pImageStream->Release();
    return hr;
}

std::wstring getExt(const std::wstring& source)
{
    std::size_t found = source.find_last_of(L".");
    if (found != std::wstring::npos)
        return source.substr(found + 1);
    return L"";
}

std::wstring tail(std::wstring const& source, size_t const length)
{
    if (length >= source.size()) { return source; }
    return source.substr(source.size() - length);
}

static const wchar_t* simple[] = { L"cbr", L"cbz", L"zip", L"7z", L"cb7", L"rar" };
static const bit7z::BitInFormat* formats[] = { &BitFormat::Rar, &BitFormat::Zip, &BitFormat::Zip, &BitFormat::SevenZip, &BitFormat::SevenZip, &BitFormat::Rar };

const bit7z::BitInFormat* IsGeneric(const std::wstring& ext)
{
    int count = sizeof(simple) / sizeof(wchar_t*);
    for (int i = 0; i < count; i++)
        if (ext == simple[i])
            return formats[i];
    throw new std::logic_error("");
}
