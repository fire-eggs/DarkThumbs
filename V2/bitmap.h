#pragma once

#include <wincodec.h>   // Windows Imaging Codecs

#pragma comment(lib, "windowscodecs.lib")

void SaveBitmap(char*, HBITMAP);
HRESULT ConvertBitmapSourceTo32BPPHBITMAP(IWICBitmapSource* pBitmapSource,
    IWICImagingFactory* pImagingFactory,
    HBITMAP* phbmp);
HRESULT WICCreate32BitsPerPixelHBITMAP(IStream* pstm, UINT /* cx */, HBITMAP* phbmp, WTS_ALPHATYPE* pdwAlpha);

HRESULT doBmp(wchar_t* path, int index, const BitInFormat* fmt, uint64_t size, HBITMAP *phbmp);

