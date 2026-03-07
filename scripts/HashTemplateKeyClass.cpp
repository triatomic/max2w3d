#include "HashTemplateKeyClass.h"

// optimize even in debug mode
#ifdef DEBUG
#pragma optimize("gt", on)
#endif

#define XXH_INLINE_ALL
#include "xxhash.h"

Hash128 LargeByteHashFunc128(const char* buf, size_t length)
{
	alignas(16) XXH128_hash_t a = XXH3_128bits((void*)buf, length);
	return *(Hash128*)&a;
}

uint64 LargeByteHashFunc64(const char* buf, size_t length)
{
	return XXH3_64bits((void*)buf, length);
}

uint32 LargeByteHashFunc32(const char* buf, size_t length)
{
	return XXH32((void*)buf, length, FNV_OFFSET_BASIS_32);
}

size_t LargeByteHashFunc(const char* buf, size_t length)
{
	return (size_t) XXH3_64bits((void*)buf, length);
}

#ifdef DEBUG
#pragma optimize("", on)
#endif