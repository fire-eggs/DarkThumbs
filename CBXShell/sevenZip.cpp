#include "StdAfx.h"
#include <string>
#include <algorithm>
#include <atlstr.h>

extern void __cdecl logit(LPCWSTR format, ...);

#include "bitexception.hpp"
#include "bitextractor.hpp"
#include "bitarchivereader.hpp"

using namespace bit7z;

const wchar_t* imgs[] = { L"bmp", L"ico", L"gif", L"jpg", L"jpeg", L"jpe", L"jfif", L"png", 
                          L"tif", L"tiff", L"webp", L"jxr", L"nrw", L"nef", L"dng", L"cr2", 
                          L"heif", L"heic", L"avif", L"jxl" };

BOOL IsImage(std::wstring ext) // TODO is this pass-by-value?
{
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    for (auto foo : imgs)
        if (ext == foo)
            return TRUE;
    return FALSE;
}

int Generic(const std::wstring& path, BOOL sort, BOOL skip, BOOL cover, uint64_t* size, const BitInFormat* fmt)
{
    int res = -1;
    try
    {
        Bit7zLibrary lib{_T("7z.dll")};
        BitArchiveReader bai{ lib, path };

        if (bai.isSolid())
            logit(_T("7z SOLID"));

        if (bai.isMultiVolume() || bai.isPasswordDefined() || bai.hasEncryptedItems())
        {
            logit(_T("7z: unsupported [multi / password / encrypt]"));
            return res;
        }

        auto arc_items = bai.items();

        // TODO sort option

        bool firstFile = true;
        CString prevname;
        for (auto& item : arc_items)
        {
            if (!IsImage(item.extension()))
                continue;

            CStringW iName = CStringW(item.name().c_str());
            auto allLower = iName.MakeLower();

            // TODO issue with manga like "death note"
            if (skip) // Skipping common scanlation files
            {
                // TODO refactor to subroutine
                if (allLower.Find(L"credit") != -1 ||
                    allLower.Find(L"note") != -1 ||
                    allLower.Find(L"recruit") != -1 ||
                    allLower.Find(L"invite") != -1 ||
                    allLower.Find(L"logo") != -1)
                {
                    continue;
                }
            }

            if (!sort)
            {
                // Not sorting: take the first non-skipped image file
                res = item.index();
                *size = item.size();
                break;
            }

            if (cover && allLower.Find(L"cover") != -1)
            {
                // Seeking cover file: take the first "cover"
                res = item.index();
                *size = item.size();
                break;
            }

            // During sorting, force certain characters to sort later.
            allLower.Replace(L"[", L"z");

            // So we're sorting. Want to take the first file in alphabetic order
            if (prevname.IsEmpty())
            {
                prevname = allLower; // can't compare empty string
                res = item.index();
                *size = item.size();
            }
            else if (-1 == StrCmpLogicalW(allLower, prevname))
            {
                res = item.index();
                *size = item.size();
                prevname = allLower;
            }
        }
    }
    catch (const BitException& ex)
    {
        logit(_T("7z: bitexception [%ld] %ls"), ex.hresultCode(), ex.what());
        res = -1;
    }
    return res; // Not actually a result, is the item index
}

extern HRESULT doBmp(const std::wstring& path, int index, const BitInFormat* fmt, uint64_t size, HBITMAP* phBmpThumbnail); // TODO header

int Foo(CStringW origpath, BOOL sort, BOOL skip, BOOL cover, const BitInFormat* fmt, HBITMAP* phBmpThumbnail)
{
    const size_t newsizew = (origpath.GetLength() + 1) * 2;
    wchar_t* n2stringw = new wchar_t[newsizew];
    wcscpy_s(n2stringw, newsizew, origpath);

    uint64_t outsize = 0;
    int index = Generic(n2stringw, sort, skip, cover, &outsize, fmt);
    if (index == -1)
    {
        delete[]n2stringw;
        return E_FAIL;
    }
    HRESULT res = doBmp(n2stringw, index, fmt, outsize, phBmpThumbnail);
    delete[]n2stringw;
    return res;
}
