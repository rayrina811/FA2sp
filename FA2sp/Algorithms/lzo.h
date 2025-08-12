#pragma once

#include <string>
#include <CMapData.h>

class lzo
{
public:
	static std::string compress(const void* src, int slen);
	static std::string compressIsoMapPack5(const IsoMapPack5Entry* src, int slen);
	static std::string decompress(const void* src, int slen);
};