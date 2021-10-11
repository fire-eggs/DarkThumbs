// Tester.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <atlstr.h>
#include <string>
#include <strsafe.h>
#include <locale.h>

extern CString m_cbxFile; // Tester hack
extern BOOL m_bSort;      // Tester hack
extern SIZE m_thumbSize;  // Tester hack
extern int m_howFound;

extern HRESULT OnExtract(HBITMAP* phBmpThumbnail);
extern CString GetEpubTitle(LPCTSTR);
extern HRESULT ExtractMobiCover(CString filepath, HBITMAP* phBmpThumbnail);
extern HRESULT Cbr(HBITMAP* phBmpThumbnail);


void FindFilesRecursively(LPCTSTR lpFolder, LPCTSTR lpFilePattern)
{
    TCHAR szFullPattern[MAX_PATH];
    WIN32_FIND_DATA FindFileData;
    HANDLE hFindFile;
    // first we are going to process any subdirectories
    PathCombine(szFullPattern, lpFolder, _T("*"));
    hFindFile = FindFirstFile(szFullPattern, &FindFileData);
    if (hFindFile != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                if (FindFileData.cFileName[0] == '.') continue;

                // found a subdirectory; recurse into it
                PathCombine(szFullPattern, lpFolder, FindFileData.cFileName);
                FindFilesRecursively(szFullPattern, lpFilePattern);
            }
        } while (FindNextFile(hFindFile, &FindFileData));
        FindClose(hFindFile);
    }

    // Now we are going to look for the matching files
    PathCombine(szFullPattern, lpFolder, lpFilePattern);
    hFindFile = FindFirstFile(szFullPattern, &FindFileData);
    if (hFindFile != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                // found a file; do something with it
                PathCombine(szFullPattern, lpFolder, FindFileData.cFileName);

                m_cbxFile = szFullPattern;
                HBITMAP pbmp;
#ifdef MOBI
                HRESULT res = ExtractMobiCover(m_cbxFile, &pbmp);
#else
                HRESULT res = OnExtract(&pbmp);
#endif
                if (res == E_FAIL)
                    _tprintf(TEXT("%s : %s - %d\n"), szFullPattern, (res == E_FAIL) ? TEXT("Fail") : TEXT("Success"), m_howFound);

//                _tprintf_s(_T("%s\n"), szFullPattern);
            }
        } while (FindNextFile(hFindFile, &FindFileData));
        FindClose(hFindFile);
    }
}


int _tmain(int argc, TCHAR **argv)
{
    if (argc < 2)
        return 1;
    setlocale(LC_ALL, ""); // for localized titles

    m_thumbSize.cx = m_thumbSize.cy = 16;


#ifdef MOBI
    TCHAR szWild[MAX_PATH];
    StringCchCopy(szWild, MAX_PATH, argv[1]);

    FindFilesRecursively(szWild, _T("*.mobi"));
#endif


#ifdef RECURSIVE
    TCHAR szWild[MAX_PATH];
    StringCchCopy(szWild, MAX_PATH, argv[1]);

    FindFilesRecursively(szWild, _T("*.epub"));
#endif

#ifdef ARG_IS_PATH
    TCHAR szWild[MAX_PATH];
    StringCchCopy(szWild, MAX_PATH, argv[1]);
    StringCchCat(szWild, MAX_PATH, TEXT("\\*.epub"));

    WIN32_FIND_DATA ffd;
    HANDLE hFind = FindFirstFile(szWild, &ffd);
    if (INVALID_HANDLE_VALUE == hFind)
        return 1;

    do
    {
        m_cbxFile = argv[1];
        m_cbxFile += "\\";
        m_cbxFile += ffd.cFileName;

        m_bSort = TRUE;
        m_thumbSize.cx = m_thumbSize.cy = 16;
        HBITMAP pbmp;
        HRESULT res = OnExtract(&pbmp);
        _tprintf(TEXT("%s : %s - %d\n"), ffd.cFileName, (res == E_FAIL) ? TEXT("Fail") : TEXT("Success"), m_howFound);

    } while (FindNextFile(hFind, &ffd) != 0);

    FindClose(hFind);
#endif
#ifndef RECURSIVE
#ifndef MOBI
    m_cbxFile = CString(argv[1]);
    m_bSort = TRUE;
    m_thumbSize.cx = m_thumbSize.cy = 16;
    m_howFound = -1;

    HBITMAP pbmp;
    HRESULT res;

    CString b = m_cbxFile.Right(4);
    b.MakeLower();

    if (b == "mobi")
    {
        res = ExtractMobiCover(m_cbxFile, &pbmp);
    }
    else if (b == "epub")
    {
        res = OnExtract(&pbmp);
    }
    else if (b == ".cbr")
    {
        m_bSort = FALSE;
        res = Cbr(&pbmp);
    }
    if (res == E_FAIL)
        printf("Cover: Fail\n");
    else
        printf("Cover: Success - %d\n", m_howFound);

    //CString title = GetEpubTitle(m_cbxFile);
    //printf("Title: %ls\n", title);
#endif
#endif
}
