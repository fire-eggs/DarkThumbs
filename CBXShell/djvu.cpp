#include "ddjvuapi.h"
#include <atlimage.h>
#include <wincodec.h>   // Windows Imaging Codecs

extern void __cdecl logit(LPCWSTR format, ...);

extern HRESULT _cdecl ConvertBitmapSourceTo32BPPHBITMAP(IWICBitmapSource* pBitmapSource,
    IWICImagingFactory* pImagingFactory,
    HBITMAP* phbmp);

// TODO is a global an issue?
ddjvu_context_t* ctx;

void handle(int wait)
{
    const ddjvu_message_t* msg;
    if (!ctx)
        return;
    if (wait)
        msg = ddjvu_message_wait(ctx);
    while ((msg = ddjvu_message_peek(ctx)))
    {
        switch (msg->m_any.tag)
        {
        case DDJVU_ERROR:
            logit(_T("djvu error"));
            break;
        default:
            break;
        }
        ddjvu_message_pop(ctx);
    }
}

void ErrorDescription(HRESULT hr)
{
    if (FACILITY_WINDOWS == HRESULT_FACILITY(hr))
        hr = HRESULT_CODE(hr);
    TCHAR* szErrMsg;

    if (FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&szErrMsg, 0, NULL) != 0)
    {
        logit(_T("%s"), szErrMsg);
        LocalFree(szErrMsg);
    }
}

HRESULT test(int _nHeight, int _nWidth, BYTE *pImageBuffer, HBITMAP *phbmp)
{
    HRESULT res = E_FAIL;

    IWICImagingFactory* pImagingFactory;
    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pImagingFactory));
    if (SUCCEEDED(hr))
    {

        int strideIn = _nWidth * 3; // ((((_nWidth * 3) + 31) & ~31) >> 3)* _nHeight;

        IWICBitmap* pEmbBitmap;
        hr = pImagingFactory->CreateBitmapFromMemory(
            _nWidth,  // width
            _nHeight,   // height
            GUID_WICPixelFormat24bppRGB, // pixel format of the NEW bitmap
            strideIn,
            _nWidth * 3 * _nHeight, // height x width
            pImageBuffer, // name of the .c array
            &pEmbBitmap  // pointer to pointer to whatever an IWICBitmap is.
        );

        if (SUCCEEDED(hr))
        {
            res = ConvertBitmapSourceTo32BPPHBITMAP(pEmbBitmap, pImagingFactory,phbmp);
            pEmbBitmap->Release();
        }
        else
        {
            logit(_T("CBFM %ld"), hr);
            ErrorDescription(hr);
        }
        pImagingFactory->Release();
    }
    return res;
}


HRESULT renderPage(ddjvu_page_t* page, HBITMAP* phBmpThumbnail)
{
    *phBmpThumbnail = NULL;
    HRESULT res = E_FAIL;

    ddjvu_rect_t prect;
    ddjvu_rect_t rrect;
    ddjvu_format_style_t style;
    ddjvu_render_mode_t mode;
    ddjvu_format_t* fmt;
    int iw = ddjvu_page_get_width(page);
    int ih = ddjvu_page_get_height(page);
    int dpi = ddjvu_page_get_resolution(page);
    ddjvu_page_type_t type = ddjvu_page_get_type(page);
    char* image = NULL;
    int rowsize = 0;

    mode = DDJVU_RENDER_COLOR;
    style = DDJVU_FORMAT_RGB24;

    prect.w = iw;
    prect.h = ih;
    rrect = prect;

    if (!(fmt = ddjvu_format_create(style, 0, 0)))
    {
        logit(_T("Cannot determine pixel style for page 1"));
        goto finish;
    }
    ddjvu_format_set_row_order(fmt, 1);
    rowsize = rrect.w * 3;
    image = (char *)LocalAlloc(LPTR, rowsize * rrect.h);
    if (!image)
    {
        logit(_T("Cannot allocate image buffer for page 1"));
        goto finish;
    }
    if (!ddjvu_page_render(page, mode, &prect, &rrect, fmt, rowsize, image))
    {
        logit(_T("ddjvu_page_render fail"));
        goto finish;
    }

    res = test(ih, iw, (BYTE *)image, phBmpThumbnail);

finish:
    ddjvu_format_release(fmt);
    if (image)
        LocalFree(image);
    return res;
}

HRESULT ExtractDjvuCover(CString filepath, HBITMAP* phBmpThumbnail)
{
    ddjvu_document_t* doc;
    ctx = NULL;
    HRESULT res = E_FAIL;

    CT2A inputfilename(filepath); // TODO unicode filename

    if (!(ctx = ddjvu_context_create("CBXShell")))
    {
        logit(_T("Cannot create djvu context."));
        goto finish;
    }
    if (!(doc = ddjvu_document_create_by_filename(ctx, inputfilename.m_psz, TRUE)))
    {
        logit(_T("Cannot open djvu document '%ls'."), filepath);
        goto finish;
    }
    while (!ddjvu_document_decoding_done(doc))
        handle(TRUE);
    if (ddjvu_document_decoding_error(doc))
    {
        logit(_T("Cannot decode djvu document '%ls'."), filepath);
        goto finish;
    }

    // dopage(int pageno)
    ddjvu_page_t* page;
    if (!(page = ddjvu_page_create_by_pageno(doc, 0)))
    {
        logit(_T("Cannot access page 1 in djvu document '%ls'."), filepath);
        goto finish;
    }
    while (!ddjvu_page_decoding_done(page))
        handle(TRUE);
    if (ddjvu_page_decoding_error(page))
    {
        handle(FALSE);
        logit(_T("Cannot decode page 1 in djvu document '%ls'."), filepath);
        ddjvu_page_release(page);
        goto finish;
    }

    res = renderPage(page, phBmpThumbnail);
    ddjvu_page_release(page);

    //closefile(pageno);
finish:
    if (doc)
        ddjvu_document_release(doc);
    if (ctx)
        ddjvu_context_release(ctx);
    return res;
}