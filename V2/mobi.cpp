#include <string>
#include <algorithm>

#include "common.h"
#include "log.h"

#include "mobi.h"

//
// Extrapolated from mobitool.c in the libmobi repo. See https://github.com/bfabiszewski/libmobi
//
int fetchMobiCover(const MOBIData* m, HBITMAP* phBmpThumbnail, SIZE thumbSize)
{
	MOBIPdbRecord* record = NULL;
	MOBIExthHeader* exth = mobi_get_exthrecord_by_tag(m, EXTH_COVEROFFSET);
	if (exth)
	{
		uint32_t offset = mobi_decode_exthvalue((const unsigned char*)(exth->data), exth->size);
		size_t first_resource = mobi_get_first_resource_record(m);
		size_t uid = first_resource + offset;
		record = mobi_get_record_by_seqnumber(m, uid);
	}
	if (record == NULL || record->size < 4)
	{
		return E_FAIL;
	}

	HGLOBAL hG = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, (SIZE_T)record->size);
	if (hG)
	{
		LPVOID pBuf = ::GlobalLock(hG);
		if (pBuf)
			CopyMemory(pBuf, record->data, record->size);

		if (::GlobalUnlock(hG) == 0 && GetLastError() == NO_ERROR)
		{
			IStream* pIs = NULL;
			if (S_OK == CreateStreamOnHGlobal(hG, TRUE, (LPSTREAM*)&pIs))//autofree hG
			{
				*phBmpThumbnail = ThumbnailFromIStream(pIs, &thumbSize, false);
				pIs->Release();
				pIs = NULL;
			}
		}
		GlobalFree(hG);
	}
	return ((*phBmpThumbnail) ? S_OK : E_FAIL);
}

//
// Extrapolated from mobitool.c in the libmobi repo. See https://github.com/bfabiszewski/libmobi
//
HRESULT ExtractMobiCover(const std::wstring& filepath, HBITMAP* phBmpThumbnail)
{
	MOBI_RET mobi_ret;
	int ret = S_OK;
	/* Initialize main MOBIData structure */
	MOBIData* m = mobi_init();
	if (m == NULL) {
		return E_FAIL;
	}
	/* By default loader will parse KF8 part of hybrid KF7/KF8 file */
//    if (parse_kf7_opt) 
	{
		/* Force it to parse KF7 part */
		mobi_parse_kf7(m);
	}
	errno = 0;
	FILE* file;
	errno_t errnot = _wfopen_s(&file, filepath.c_str(), L"rb");
	if (file == NULL) {
		int errsv = errnot; // TODO logging
		mobi_free(m);
		return E_FAIL;
	}
	/* MOBIData structure will be filled with loaded document data and metadata */
	mobi_ret = mobi_load_file(m, file);
	fclose(file);

	if (mobi_ret != MOBI_SUCCESS) {
		mobi_free(m);
		return E_FAIL;
	}

	SIZE thumbsize{ 256, 256 };
	ret = fetchMobiCover(m, phBmpThumbnail, thumbsize);
	/* Free MOBIData structure */
	mobi_free(m);
	return ret;
}
