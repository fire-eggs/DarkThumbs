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

// TODO to common
int searchByPath(const vector<BitArchiveItem>& items, std::string& target)
{
    int filedex = 0;
    bool found = false;
    std::wstring wtarget = get_wstring(target);
    std::replace(wtarget.begin(), wtarget.end(), '/', '\\');  // NOTE: bit7z has rationalized all slashes to backslash

    for (auto& item : items)
    {
        if (found = (item.path() == wtarget))
            break;
        filedex++;
    }

    if (!found)
        return -1; // TODO logging

    return filedex;
}

// As per EPUB standard: open the container.xml file to fetch the path of the rootfile.
// returns: the index of the rootfile, -1 if not found.
//
int findRootFile(const std::wstring& path, const vector<BitArchiveItem>& items, BitExtractor* bex, std::string& rootpath)
{
    auto foo = L"META-INF\\container.xml"; // NOTE: bit7z has rationalized all slashes to backslash

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
        return -1; // TODO logging

    posStart = xmlContent.find("full-path=\"", posStart);

    if (posStart == std::string::npos)
        return -1; // TODO logging

    posStart += 11;
    size_t posEnd = xmlContent.find("\"", posStart);

    rootpath = xmlContent.substr(posStart, posEnd - posStart);

    int res = searchByPath(items, rootpath);

    posStart = rootpath.rfind('/');
    if (posStart != std::string::npos) {
        rootpath = rootpath.substr(0, posStart + 1);
    }

    return res;

/*
    std::replace(rootfile.begin(), rootfile.end(), '/', '\\');  // NOTE: bit7z has rationalized all slashes to backslash

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
*/
}

// A brute-force search for the first image file with "cover" in its name.
// items : the item list from the archive
// size  : the returned size of the found image. undefined if not found.
// returns: the index of the found image file, or -1 if none
//
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

int searchMeta(const std::string& xml, const vector<BitArchiveItem>& items)
{
    // search for '<meta name="cover" content="cover"/>'
    // then search for '<item href="cover.jpeg" id="cover" media-type="image/jpeg"/>'. The id val match earlier search
    return -1;
}

int searchManifest(const std::string& xmlContent, const vector<BitArchiveItem>& items, int rootdex, std::string& rootpath)
{
    // search for a <manifest> cover item with id="cover-image"


    size_t posManifest = xmlContent.find("<manifest>");
    if (posManifest == std::string::npos) return -1;

    size_t posStart = xmlContent.find("id=\"cover-image\"", posManifest);
    if (posStart == std::string::npos) return -1;

    // found it, move backward to find owning "<item"
    size_t posItem = xmlContent.rfind("<item ", posStart);
    if (posItem == std::string::npos) return -1;

    // find the href
    size_t posHref = xmlContent.find("href=\"", posItem);  // TODO this might not match in *this* item
    posHref += 6;
    size_t posEnd = xmlContent.find("\"", posHref);

    // TODO rootpath + xmlC.substr()
    std::string subpath = rootpath + xmlContent.substr(posHref, posEnd - posHref);
    return searchByPath(items, subpath);
}

int searchXHtml()
{
    return -1;
}

int findMetaCover(const std::wstring& path, std::string& rootpath, int rootdex, const vector<BitArchiveItem>& items, uint64_t* size, BitExtractor *bex)
{
    int imgDex = -1;

    // extract the rootfile
    vector<byte_t> bytes;
    bex->extract(path, bytes, rootdex);
    std::string xmlContent = (char*)bytes.data();

    // search for '<meta name="cover" content="cover"/>'
    // then search for '<item href="cover.jpeg" id="cover" media-type="image/jpeg"/>'. The id val match earlier search
    imgDex = searchMeta(xmlContent, items);

    // search for a <manifest> cover item with id="cover-image"
    if (imgDex == -1)
        imgDex = searchManifest(xmlContent, items, rootdex, rootpath);

    // Search for a <manifest> cover item and an image path via HTML file
    if (imgDex == -1)
        imgDex = searchXHtml();

    if (imgDex == -1)
        return -1;

    const BitArchiveItem* item = &items[imgDex];
    if (!IsImage(item->extension()))
        return -1; // TODO logging

    *size = item->size();
    return imgDex;
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
        std::string rootpath;
        int rootdex = findRootFile(path, arc_items, &bex, rootpath);

        // 2. using metadata, determine the cover file [various methods]
        int coverdex = -1;
        if (rootdex != -1)
            coverdex = findMetaCover(path, rootpath, rootdex, arc_items, size, &bex);
        
        // 3. if fail, brute-force return an image with "cover" in its name
        if (coverdex == -1)
            coverdex = findCover(arc_items, size);

        // 4. if fail, return -1
        return coverdex;
    }
    catch (const BitException& ex)
    {
        LOGERR(L"Epub: BitException: %s", ex.what());
    }
    return res;
}
