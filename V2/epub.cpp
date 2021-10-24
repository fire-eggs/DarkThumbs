#include <string>
#include <algorithm>
#include "bitarchiveinfo.hpp"
#include "bitexception.hpp"
#include "bitextractor.hpp"

using namespace bit7z;

#include "common.h"
#include "log.h"

// TODO implement with Bit7z or unzip?

// TODO to common
std::wstring get_wstring(const std::string& s)
{
    const char* cs = s.c_str();
    const size_t wn = std::mbsrtowcs(NULL, &cs, 0, NULL);

    if (wn == size_t(-1))
    {
        return L"";
    }

    std::vector<wchar_t> buf(wn + 1);
    const size_t wn_again = std::mbsrtowcs(buf.data(), &cs, wn + 1, NULL);

    if (wn_again == size_t(-1))
    {
        return L"";
    }

    assert(cs == NULL); // successful conversion

    return std::wstring(buf.data(), wn);
}

int findRootFile(const std::wstring& path, const vector<BitArchiveItem>& items, BitExtractor* bex)
{
    auto foo = L"META-INF\\container.xml";

    int contdex = 0;
    bool found = false;
    for (auto& item : items)
    {
        if (found = (item.path() == foo))
            break;
        contdex++;
    }

    if (!found)
        return -1; // TODO logging

    vector<byte_t> bytes;
    bex->extract(path, bytes, contdex);
    std::string xmlContent = (char*)bytes.data();

    size_t posStart = xmlContent.find("rootfile ");

    if (posStart == std::string::npos)
        return -1;

    posStart = xmlContent.find("full-path=\"", posStart);

    if (posStart == std::string::npos)
        return -1;

    posStart += 11;
    size_t posEnd = xmlContent.find("\"", posStart);

    std::string rootfile = xmlContent.substr(posStart, posEnd - posStart);
    std::replace(rootfile.begin(), rootfile.end(), '/', '\\');

    int rootdex = 0;
    found = false;
    for (auto& item : items)
    {
        if (found = (item.path() == get_wstring(rootfile)))
            break;
        rootdex++;
    }

    if (!found)
        return -1; // TODO logging

    return rootdex;
}

int findCover(const vector<BitArchiveItem>& items, uint64_t *size)
{
    int contdex = 0;
    for (auto& item : items)
    {
        auto name = item.name();
        std::transform(name.begin(), name.end(), name.begin(), ::towlower);

        if (IsImage(item.extension()) && name.find(L"cover") != std::string::npos)
        {
            *size = item.size();
            return contdex;
        }

        contdex++;
    }
    return -1;
}

int Epub(const std::wstring& path, uint64_t* size)
{
    int res = -1;
    try
    {
        Bit7zLibrary lib{};
        BitArchiveInfo bai{ lib, path, BitFormat::Zip }; // epub are ZIP
        BitExtractor bex(lib, BitFormat::Zip);

        auto arc_items = bai.items();

        // 1. retrieve the rootfile from container.xml
        int rootdex = findRootFile(path, arc_items, &bex);

        // 2. using metadata, determine the cover file [various methods]
        
        // 3. if fail, brute-force return an image with "cover" in its name
        int coverdex = findCover(arc_items, size);

        // 4. if fail, return -1
        return coverdex;
    }
    catch (const BitException& ex)
    {
        LOGERR(L"Epub: BitException: %s", ex.what());
    }
    return res;
}
