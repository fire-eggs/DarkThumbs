#ifndef __CUNZIP_H__
#define __CUNZIP_H__

#include "unzip.h"
#include <wtypes.h>

class CUnzip
{
public:
	CUnzip() { hz = NULL; }
	virtual ~CUnzip() { ::CloseZip(hz); }

public:
	bool Open(LPCTSTR zfile)
	{
		if (zfile == NULL) return false;
		HZIP temp_hz = ::OpenZip(zfile, NULL);//try new
		if (temp_hz == NULL) return false;
		Close();//close old
		hz = temp_hz;
		if (ZR_OK != ::GetZipItem(hz, -1, &maindirEntry)) return false;
		return true;
	}

	bool GetItem(int zi)
	{
		zr = ::GetZipItem(hz, zi, &ZipEntry);
		return (ZR_OK == zr);
	}

	bool UnzipItemToMembuffer(int index, void* z, unsigned int len)
	{
		zr = ::UnzipItem(hz, index, z, len);
		return (ZR_OK == zr);
	}

	void Close()
	{
		CloseZip(hz);
		hz = NULL;//critical!
	}

	inline BOOL ItemIsDirectory() { return (BOOL)(CUnzip::GetItemAttributes() & 0x0010); }
	int GetItemCount() const { return maindirEntry.index; }
	long GetItemPackedSize() const { return ZipEntry.comp_size; }
	long GetItemUnpackedSize() const { return ZipEntry.unc_size; }
	DWORD GetItemAttributes() const { return ZipEntry.attr; }
	LPCTSTR GetItemName() { return ZipEntry.name; }

	HZIP getHZIP() { return hz; }

private:
	ZIPENTRY ZipEntry, maindirEntry;
	HZIP hz;
	ZRESULT zr;
};

#endif // __CUNZIP_H__
