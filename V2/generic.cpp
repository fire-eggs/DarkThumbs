#include <string>
#include <algorithm>
#include <Shlwapi.h> // strcmplogicalW
#include "bitarchiveinfo.hpp"
#include "bitexception.hpp"
#include "bitextractor.hpp"

using namespace bit7z;

#include "common.h"
#include "log.h"

// Alphabetic sort using the original T800 logic
bool sortfunc(const BitArchiveItem& a, const BitArchiveItem& b) { return StrCmpLogicalW(a.name().c_str(), b.name().c_str()) < 0; }

// Fetch an image from any archive [i.e. NOT an epub, mobi, fb, etc].
// path : path to archive file
// sort : if true, takes the first image alphabetically. if false, takes the first image found.
// size : size of the archive item
// fmt  : the archive format to use, based on the extension
// returns: the index of the image inside the archive, or -1 on error, unsupported, or not found.
//
int Generic(const std::wstring& path, BOOL sort, uint64_t* size, const BitInFormat* fmt)
{
    int res = -1;
    try
    {
        Bit7zLibrary lib{};
        BitArchiveInfo bai{ lib, path, *fmt };

        if (bai.isSolid())
            LOGDBG(L"Generic: solid")

        if (bai.isMultiVolume() || bai.isPasswordDefined() || bai.hasEncryptedItems()) // TODO logging
        {
            LOGINFO(L"Generic: unsupported");
            return res;
        }

        int icount = 0;
        auto arc_items = bai.items();

        // TODO sort option
        if (sort)
            std::sort(arc_items.begin(), arc_items.end(), sortfunc);

        bool firstFile = true;
        for (auto& item : arc_items)
        {
            // TODO check for 32M limit?

            if (IsImage(item.extension()) &&
                item.path().find(L"__MACOSX") == std::string::npos  // Issue #36: ignore __MACOSX folder
                ) 
            {
                icount++;
                if (firstFile)
                {
                    LOGDBG(L"Generic: First: %s", item.name().c_str());
                    firstFile = false;
                    res = item.index();
                    *size = item.size();
                    return res;
                }
            }
        }

        LOGDBG(L"Generic: Image count: %d", icount);
    }
    catch (const BitException& ex)
    {
        CMyLog::GetInstance().Log(LOG_LEVEL_INFO, TEXT(__FILE__), __LINE__, L"Generic: BitException: %s", ex.what());
    }
    return res; // Not actually a result, is the item index
}

