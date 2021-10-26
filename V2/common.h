#pragma once

#include "bitextractor.hpp"

using namespace bit7z;

BOOL IsImage(std::wstring ext);

std::wstring getExt(const std::wstring& source);

std::wstring tail(std::wstring const& source, size_t const length);

bit7z::BitInFormat* IsGeneric(const std::wstring& ext);

HBITMAP ThumbnailFromIStream(IStream* pIs, const LPSIZE pThumbSize, bool showIcon);
