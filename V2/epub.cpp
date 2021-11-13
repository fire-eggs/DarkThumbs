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
    else
        rootpath = ""; // Botany, Programming Pearls: OPF file is at the root, there is no path


    return res;
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

// Search the text of the OPF file [XML] for a <meta> tag with an attribute <name> of "cover".
// returns the text of the matching <content> attribute, empty string if none.
//
std::string metaCover(const std::string& xmlContent)
{
    // Find meta tag for cover
    // I.e. searching for '<meta name="cover" content="cover"/>'

    std::string coverTag, coverId, itemTag;

    size_t posStart = xmlContent.find("name=\"cover\"");

    if (posStart == std::string::npos) {
        return coverId;
    }

    posStart = xmlContent.find_last_of("<", posStart);
    size_t posEnd = xmlContent.find(">", posStart);

    coverTag = xmlContent.substr(posStart, posEnd - posStart + 1);

    // Find cover item id

    posStart = coverTag.find("content=\"");

    if (posStart == std::string::npos) {
        return coverId;
    }

    posStart += 9;
    posEnd = coverTag.find("\"", posStart);

    coverId = coverTag.substr(posStart, posEnd - posStart);
    return coverId;
}

// Search the text of the OPF file [XML] for a <item> tag with an <id> attribute with a given value.
// returns the text of the matching <href> attribute, empty string if none.
//
std::string manifestId(const std::string& xmlContent, const std::string& coverId)
{
    // Find item tag in original opf file contents
    // I.e. searching for '<item href="cover.jpeg" id="cover" media-type="image/jpeg"/>'. The id
    // value matching the "content" value from before.

    size_t posStart, posEnd;

    posStart = xmlContent.find("id=\"" + coverId + "\"");
    if (posStart != std::string::npos)
    {
        posStart = xmlContent.find_last_of("<", posStart);
        posEnd = xmlContent.find(">", posStart);

        std::string itemTag = xmlContent.substr(posStart, posEnd - posStart + 1);

        // Find cover path in item tag

        posStart = itemTag.find("href=\"");

        if (posStart != std::string::npos)
        {
            posStart += 6;
            posEnd = itemTag.find("\"", posStart);

            if (posEnd != std::string::npos)
            {
                return itemTag.substr(posStart, posEnd - posStart);
            }
        }
    }
    return "";
}

// Search the text of the OPF file [XML] for a set of <meta> and <item> tags which specify the cover file.
// These are of the form:
// <meta name="cover" content="id-val"/>
// ...
// <item href="images\cover.jpeg" id="id-val" media-type="image/jpeg"/>
// Note the 'id-val' from the <meta> matches the 'id-val' in the <item>
//
// Returns the index to the matching item within the archive, or -1 if none
//
int searchMeta(const std::string& xml, const vector<BitArchiveItem>& items, std::string& rootpath)
{
    // search for '<meta name="cover" content="cover"/>'
    // then search for '<item href="cover.jpeg" id="cover" media-type="image/jpeg"/>'. The id val match earlier search
    // then find the item id within the archive

    std::string coverId = metaCover(xml);
    if (coverId.empty())
        return -1;

    std::string secondId = manifestId(xml, coverId);
    if (secondId.empty())
        return -1;

    std::string subpath = rootpath + secondId;
    subpath = urlDecode(subpath); // Required for encoded chars, see gallun.epub
    return searchByPath(items, subpath);
}

// Search the text of the OPF file [XML] for a <manifest> tag which specifies the cover file.
// These will typically be of the form:
// ...
// <manifest>
// ...
// <item id = "cover-image" properties = "cover-image" href = "images/a-christmas-carol.jpg" media - type = "image/jpeg" / >
//
// Returns the index to the matching item within the archive, or -1 if none
//
int searchManifest(const std::string& xmlContent, const vector<BitArchiveItem>& items, std::string& rootpath)
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
    size_t posHref = xmlContent.find("href=\"", posItem);
    posHref += 6;
    size_t posEnd = xmlContent.find("\"", posHref);

    // specified path is relative to the rootpath
    std::string subpath = rootpath + xmlContent.substr(posHref, posEnd - posHref); // TODO possibly requires urlDecode?
    return searchByPath(items, subpath);
}

/*
Handle the case where the epub has a <manifest> item for the cover which points to 
a secondary xhtml / xml file. In *that* secondary file, the cover image is specified
as a filename within an <img> tag.

<manifest>
<item id="cover"    href="content/cover.xml"   media-type="application/xhtml+xml" />
...
cover.xml:
...
<img ... src="filename"... />
*/
int searchXHtml(const std::wstring& path, const std::string& xmlContent, const vector<BitArchiveItem>& items, std::string& rootpath, BitExtractor* bex)
{
    size_t posManifest = xmlContent.find("<manifest>");
    if (posManifest == std::string::npos) return -1;

    size_t posStart = xmlContent.find("id=\"cover\"", posManifest);
    if (posStart == std::string::npos) return -1;

    // found it, move backward to find owning "<item"
    size_t posItem = xmlContent.rfind("<item ", posStart);
    if (posItem == std::string::npos) return -1;

    // find the href
    size_t posHref = xmlContent.find("href=\"", posItem);
    posHref += 6;
    size_t posEnd = xmlContent.find("\"", posHref);

    // specified path is relative to the rootpath
    std::string subpath = rootpath + xmlContent.substr(posHref, posEnd - posHref); // TODO possibly requires urlDecode?

    std::string imgbase = subpath.substr(0, subpath.find_last_of("/")+1);

    int coverdex = searchByPath(items, subpath);
    if (coverdex == -1)
        return -1;

    // The path to the cover image is contained in an xHtml / xml file in an <img> tag.
    // <img ... src="filename" ...
    // This path is relative to the folder containing this second file.
    vector<byte_t> bytes;
    bex->extract(path, bytes, coverdex);
    std::string coverxml = (char*)bytes.data();

    // find the <img> tag begin
    size_t posImg = coverxml.find("<img");
    if (posImg == std::string::npos) return -1;

    // find the "src=", because it might not be immediately next to the "<img " text.
    size_t srcStart = coverxml.find("src=", posImg);
    if (srcStart == std::string::npos) return -1;

    // allowing for whitespace etc, find the quoted filename string
    size_t qStart = coverxml.find("\"", srcStart);
    if (qStart == std::string::npos) return -1;
    size_t qEnd = coverxml.find("\"", qStart + 1);
    if (qEnd == std::string::npos) return -1;

    std::string fname = coverxml.substr(qStart + 1, qEnd - qStart - 1);

    // "Hope takes flight" had a string of the form "../images/filename.jpg".
    if (fname[0] == '.' && fname[1] == '.' && fname[2] == '/')
        fname = fname.substr(3);
    
    // Try two variants of the image search path, as required to work for
    // "Hope takes flight" and "Woman an intimate geography".
    std::string subpath3 = rootpath + fname;
    int imagedex = searchByPath(items, subpath3);
    if (imagedex == -1)
    {
        // "Woman an intimate geography"
        std::string subpath2 = rootpath + imgbase + fname;
        imagedex = searchByPath(items, subpath2);
    }
    return imagedex;
}

// Search the text of the Epub OPF file [XML] for some specification of the cover image.
//
// Returns the index to the matching item within the archive, or -1 if none
//
int findMetaCover(const std::wstring& path, std::string& rootpath, int rootdex, const vector<BitArchiveItem>& items, uint64_t* size, BitExtractor *bex)
{
    int imgDex = -1;

    // extract the rootfile
    vector<byte_t> bytes;
    bex->extract(path, bytes, rootdex);
    std::string xmlContent = (char*)bytes.data();

    // search for '<meta name="cover" content="cover"/>'
    // then search for '<item href="cover.jpeg" id="cover" media-type="image/jpeg"/>'. The id val match earlier search
    imgDex = searchMeta(xmlContent, items, rootpath);

    // search for a <manifest> cover item with id="cover-image"
    if (imgDex == -1)
        imgDex = searchManifest(xmlContent, items, rootpath);

    // Search for a <manifest> cover item and an image path via HTML file
    if (imgDex == -1)
        imgDex = searchXHtml(path, xmlContent, items, rootpath, bex);

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
