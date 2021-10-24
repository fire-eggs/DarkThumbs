#include <string>
#include <algorithm>
#include "bitarchiveinfo.hpp"
#include "bitexception.hpp"
#include "bitextractor.hpp"

using namespace bit7z;

#include "common.h"
#include "log.h"

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

        bool firstFile = true;
        for (auto& item : arc_items)
        {
            if (IsImage(item.extension())) // TODO check for 32M limit
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

