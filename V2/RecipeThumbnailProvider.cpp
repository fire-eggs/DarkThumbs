
#include <shlwapi.h>
#include <thumbcache.h> // For IThumbnailProvider.
#include <Wincrypt.h>   // For CryptStringToBinary.
#include <msxml6.h>
#include <new>
#include <algorithm>    // std::transform
#include <locale>       // setlocale

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "Crypt32.lib")
#pragma comment(lib, "msxml6.lib")

#include "log.h"
#include "common.h"
#include "bitmap.h"


// TODO header
int Generic(const std::wstring& path, BOOL sort, uint64_t* size, const BitInFormat* fmt);
int Epub(const std::wstring& path, uint64_t* size);

// this thumbnail provider implements IInitializeWithStream to enable being hosted
// in an isolated process for robustness

class CRecipeThumbProvider : public IInitializeWithFile,
                             public IThumbnailProvider
{
public:
    CRecipeThumbProvider() : _cRef(1)
    {
    }

    virtual ~CRecipeThumbProvider()
    {
    }

    // IUnknown
    IFACEMETHODIMP QueryInterface(REFIID riid, void **ppv)
    {
        static const QITAB qit[] =
        {
            QITABENT(CRecipeThumbProvider, IInitializeWithFile),
            QITABENT(CRecipeThumbProvider, IThumbnailProvider),
            { 0 },
        };
        return QISearch(this, qit, riid, ppv);
    }

    IFACEMETHODIMP_(ULONG) AddRef()
    {
        return InterlockedIncrement(&_cRef);
    }

    IFACEMETHODIMP_(ULONG) Release()
    {
        ULONG cRef = InterlockedDecrement(&_cRef);
        if (!cRef)
        {
            delete this;
        }
        return cRef;
    }

    // IInitializeWithFile
    IFACEMETHODIMP Initialize(LPCWSTR pfilePath, DWORD grfMode);

    // IThumbnailProvider
    IFACEMETHODIMP GetThumbnail(UINT cx, HBITMAP *phbmp, WTS_ALPHATYPE *pdwAlpha);

private:
    long _cRef;
    //LPCWSTR _filepath;     // provided during initialization.
    WCHAR _filepath[512];
    BOOL m_bSort;
    SIZE m_thumbSize;
};

HRESULT CRecipeThumbProvider_CreateInstance(REFIID riid, void **ppv)
{
    CRecipeThumbProvider *pNew = new (std::nothrow) CRecipeThumbProvider();
    HRESULT hr = pNew ? S_OK : E_OUTOFMEMORY;
    if (SUCCEEDED(hr))
    {
        hr = pNew->QueryInterface(riid, ppv);
        pNew->Release();
    }
    return hr;
}

IFACEMETHODIMP CRecipeThumbProvider::Initialize(LPCWSTR pfilePath, DWORD grfMode)
{
    // A handler instance should be initialized only once in its lifetime. 
    //HRESULT hr = HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
    LOGINFO(L"Initialize with file: %s", pfilePath);
    wcscpy(_filepath, pfilePath);
    return S_OK;
}

// IThumbnailProvider
IFACEMETHODIMP CRecipeThumbProvider::GetThumbnail(UINT cx, HBITMAP *phbmp, WTS_ALPHATYPE *pdwAlpha)
{
    CMyLog log;

    LOGINFO(L"  GetThumbnail : %s", _filepath);
    LOGINFO(L"  Thumbsize: %d", cx);

    setlocale(LC_ALL, ""); // for localized titles

    m_bSort = TRUE; // TODO find from settings
    m_thumbSize.cx = m_thumbSize.cy = cx;

    std::wstring ext = getExt(_filepath);
    if (ext == L"")
        return E_FAIL;
    std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);

    uint64_t outsize; // The size of the image stream in bytes

    try
    {
        auto format = IsGeneric(ext);
        int index = Generic(_filepath, m_bSort, &outsize, format);
        if (index == -1)
        {
            LOGINFO(L"No image");
            return E_FAIL;
        }
        HRESULT res = doBmp((wchar_t*)_filepath, index, format, outsize, phbmp);
        *pdwAlpha = WTSAT_UNKNOWN;
        return res;
    }
    catch (...)//(const std::logic_error& ex)
    {
        // NOT generic, look for epub, mobi, fb

        if (ext == L"epub")
        {
            int index = Epub(_filepath, &outsize);

            // TODO if index === -1 call generic(zip)
            if (index == -1)
            {
                LOGINFO(L"No image");
                return E_FAIL;
            }
            HRESULT res = doBmp((wchar_t*)_filepath, index, &BitFormat::Zip, outsize, phbmp);
            *pdwAlpha = WTSAT_UNKNOWN;
            return res;
        }
    }

    return S_OK;
}
