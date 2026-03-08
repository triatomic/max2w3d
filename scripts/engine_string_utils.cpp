#include "engine_string_utils.h"
#include <stdlib.h>
#include <string.h>
#include <intrin.h>

#ifdef DEBUG
#pragma optimize("gt", on) // optimize even in debug builds
#endif

SCRIPTS_API char* newstr(const char* str)
{
	if (!str)
	{
		return nullptr;
	}
	size_t len = strlen(str) + 1;
	char* s = new char[len];
	memcpy(s, str, len);
	return s;
}

SCRIPTS_API wchar_t* newwcs(const wchar_t* str)
{
	if (!str)
		return nullptr;
	size_t len = wcslen(str) + 1;
	wchar_t* s = new wchar_t[len];
	memcpy(s, str, len * 2);
	return s;
}

SCRIPTS_API char* strtrim(char* v)
{
	if (v)
	{
		char* r = v;
		while (*r > 0 && *r < 0x21)
			r++;
		strcpy(v, r);
		r = v + strlen(v);
		while (r > v && r[-1] > 0 && r[-1] < 0x21)
			r--;
		*r = 0;
	}
	return v;
}

SCRIPTS_API char* strrtrim(char* s)
{
	char* t, * tt;

	TT_ASSERT(s != nullptr);

	for (tt = t = s; *t != '\0'; ++t)
		if (!tt_isspace(*(unsigned char*)t))
			tt = t + 1;
	*tt = '\0';

	return s;
}


// -----------------------------------------------------------
// Functions for converting entire strings to lower/upper case
// -----------------------------------------------------------

// NOTE: This function can read past the end of containers but not past capacity, which will still trigger ASan.
// Since this function by itself doesn't know what container it is operating on, it can't manually unpoison and repoison memory.
__declspec(no_sanitize_address)
SCRIPTS_API char* tt_strlwr_copy(char* dest, const char* src, size_t dest_buffer_size, size_t src_buffer_size)
{
	static constexpr size_t MIN_SIZE_FOR_SIMD = 4;
	static constexpr size_t MIN_SIZE_FOR_SIMD_MASK = (1 << (MIN_SIZE_FOR_SIMD - 1)) - 1;

	size_t min_buffer_size = min(dest_buffer_size, src_buffer_size);

	int64_t count = min_buffer_size - 1;       // exclude null terminator from conversion count
	size_t block_count = min_buffer_size >> 4; // except if we can process it as part of a block

	while (block_count--)
	{
		__m128i data = _mm_loadu_si128((const __m128i*)(src));
		__m128i cmp = _mm_cmpeq_epi8(data, _mm_setzero_si128());

		int mask = _mm_movemask_epi8(cmp);

		if (mask == 0 || mask == (1 << 15)) // data block has no null terminator or it's at the very end
		{
			_mm_storeu_si128((__m128i*)(dest), tt_tolower_x16(data));

			// uppermost byte was null terminator, return immediately so we don't try processing more blocks
			if (mask == (1 << 15))
				return dest += 15;

			dest += 16;
			src += 16;
			count -= 16;
		}
		else if (mask & 1) // data block starts with null terminator, we can just null terminate and return
		{
			*dest = '\0';
			return dest;
		}
		// if there is a null terminator within the first N bytes, don't bother converting with SIMD
		else if (mask & MIN_SIZE_FOR_SIMD_MASK)
		{
			// we already have the data loaded into a register, so might as well use it instead of loading it again
			while (!(mask & 1))
			{
				*dest++ = (char)tt_tolower((unsigned char)_mm_cvtsi128_si32(data));
				mask >>= 1;
				data = _mm_bsrli_si128(data, 1); // NOTE: shift amount for this instruction is in bytes, not bits
			}
			*dest = '\0';
			return dest;
		}
		else // we have a null terminator in our data block, but not at the very start or end
		{
			// get the byte index of the null terminator in our block, that is the number of characters left to process
			unsigned long index;
			_BitScanForward(&index, mask);
			count = index;
			char* end = dest + count;

			__m128i converted = tt_tolower_x16(data);

			// store N bytes at a time, including the null terminator if possible
			if (count >= 7) {
				_mm_storeu_si64(dest, converted);
				dest += 8;
				count -= 8;
				converted = _mm_bsrli_si128(converted, 8); // NOTE: shift amount for this instruction is in bytes, not bits
			}
			if (count >= 3) {
				_mm_storeu_si32(dest, converted);
				dest += 4;
				count -= 4;
				converted = _mm_bsrli_si128(converted, 4);
			}
			if (count >= 1)
				_mm_storeu_si16(dest, converted);

			// NOTE: it's possible we wrote the null terminator already, but branching over that actually performs worse
			*end = '\0';
			return end;
		}
	}

	TT_ASSERT(count >= -1);

	// if the destination is smaller than the source, we may have copied one extra character we need to null
	if (count == -1) {
		*--dest = '\0';
		return dest;
	}

	// we get here if there are no blocks to process
	while (*src && count--) {
		*dest++ = (char)tt_tolower((unsigned char)*src++);
	}
	*dest = '\0';
	return dest;
}

// NOTE: This function can read and even write past the end of containers but not past capacity, which will still trigger ASan.
// Since this function by itself doesn't know what container it is operating on, it can't manually unpoison and repoison memory.
__declspec(no_sanitize_address)
SCRIPTS_API char* tt_strlwr_copy_unsafe(char* dest, const char* src, size_t dest_buffer_size, size_t src_buffer_size)
{
	size_t min_buffer_size = min(dest_buffer_size, src_buffer_size);

	int64_t count = min_buffer_size - 1;       // exclude null terminator from conversion count
	size_t block_count = min_buffer_size >> 4; // except if we can process it as part of a block
	count -= 16 * block_count;

	while (block_count--)
	{
		__m128i data = _mm_loadu_si128((const __m128i*)(src));
		__m128i cmp = _mm_cmpeq_epi8(data, _mm_setzero_si128());

		int mask = _mm_movemask_epi8(cmp);

		if (mask != 0) // we have a null terminator in this block
		{
			// optional: write 0s past the null terminator instead of whatever garbage was in source
			//cmp = _mm_or_si128(_mm_bslli_si128(cmp, 1), cmp);
			//cmp = _mm_or_si128(_mm_bslli_si128(cmp, 2), cmp);
			//cmp = _mm_or_si128(_mm_bslli_si128(cmp, 4), cmp);
			//cmp = _mm_or_si128(_mm_bslli_si128(cmp, 8), cmp);
			//data = _mm_andnot_si128(cmp, data);

			_mm_storeu_si128((__m128i*)(dest), tt_tolower_x16(data));

			unsigned long index; // index of null terminator
			_BitScanForward(&index, mask);
			return dest + index;
		}

		_mm_storeu_si128((__m128i*)(dest), tt_tolower_x16(data));

		dest += 16;
		src += 16;
	}
	
	TT_ASSERT(count >= -1);

	// if the destination is smaller than the source, we may have copied one extra character we need to null
	if (count == -1) {
		*--dest = '\0';
		return dest;
	}

	// we get here if there are no blocks to process
	while (*src && count--) {
		*dest++ = (char)tt_tolower((unsigned char)*src++);
	}
	*dest = '\0';
	return dest;
}

// NOTE: This function can read and even write past the end of containers but not past capacity, which will still trigger ASan.
// Since this function by itself doesn't know what container it is operating on, it can't manually unpoison and repoison memory.
__declspec(no_sanitize_address)
SCRIPTS_API char* tt_strlwr_copy_unsafe(char* dest, const char* src, size_t dest_buffer_size, size_t src_buffer_size, size_t src_length)
{
	size_t min_buffer_size = min(dest_buffer_size, src_buffer_size);

	int64_t count = min(src_length, dest_buffer_size - 1);                    // exclude null terminator from conversion count
	size_t simple_block_count = min((src_length + 1), dest_buffer_size) >> 4; // except if we can process it as part of a block
	bool can_do_last_block = (min_buffer_size - simple_block_count * 16) >= 16 && (((src_length + 1) & 15) != 0);
	count -= 16 * simple_block_count;

	while (simple_block_count--)
	{
		TT_TOLOWER_X16(dest, src);
		dest += 16;
		src += 16;
	}

	if (can_do_last_block)
	{
		__m128i data = _mm_loadu_si128((const __m128i*)(src));
		__m128i cmp = _mm_cmpeq_epi8(data, _mm_setzero_si128());

		int mask = _mm_movemask_epi8(cmp);

		unsigned long nullindex; // index of null terminator
		auto hasnull = _BitScanForward(&nullindex, mask);

		// if you pass a src_length shorter than the actual string, we need to manually insert a null terminator in the right spot
		if (!hasnull || nullindex > count)
		{
			// NOTE: I have no idea which version is better overall.
			// Lookup table wins in benchmarks because it's either already in cache or not needed.
			// This entire branch should not happen almost ever in any case.
#if 0
			constexpr static unsigned char ones_table[16][16]{
				{0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},
				{0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},
				{0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},
				{0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},
				{0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},
				{0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},
				{0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},
				{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},
				{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},
				{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},
				{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},
				{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF},
				{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF},
				{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF},
				{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF},
				{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF}
			};
			__m128i ones = _mm_loadu_si128((__m128i*)ones_table[count]);
#else
			__m128i ones = _mm_cvtsi32_si128(0xFF);
			if (count < 8) {
				__m128i count_xmm = _mm_cvtsi64_si128(count * 8);
				ones = _mm_sll_epi64(ones, count_xmm);
			}
			else {
				__m128i count_xmm = _mm_cvtsi64_si128((count - 8) * 8);
				ones = _mm_bslli_si128(ones, 8);
				ones = _mm_sll_epi64(ones, count_xmm);
			}
			// optional: write 0s past the null terminator instead of whatever garbage was in source
			//ones = _mm_or_si128(_mm_bslli_si128(ones, 1), ones);
			//ones = _mm_or_si128(_mm_bslli_si128(ones, 2), ones);
			//ones = _mm_or_si128(_mm_bslli_si128(ones, 4), ones);
			//ones = _mm_or_si128(_mm_bslli_si128(ones, 8), ones);
#endif

			data = _mm_andnot_si128(ones, data);
			nullindex = (unsigned long)count;
		}
		else {
			// optional: write 0s past the null terminator instead of whatever garbage was in source
			//cmp = _mm_or_si128(_mm_bslli_si128(cmp, 1), cmp);
			//cmp = _mm_or_si128(_mm_bslli_si128(cmp, 2), cmp);
			//cmp = _mm_or_si128(_mm_bslli_si128(cmp, 4), cmp);
			//cmp = _mm_or_si128(_mm_bslli_si128(cmp, 8), cmp);
			//data = _mm_andnot_si128(cmp, data);
		}

		_mm_storeu_si128((__m128i*)(dest), tt_tolower_x16(data));

		return dest + nullindex;
	}

	TT_ASSERT(count >= -1);

	// if the destination is smaller than the source, we may have copied one extra character we need to null
	if (count == -1) {
		*--dest = '\0';
		return dest;
	}

	// we get here if there are no blocks to process
	while (*src && count--) {
		*dest++ = (char)tt_tolower((unsigned char)*src++);
	}
	*dest = '\0';
	return dest;
}


// NOTE: This function can read past the end of containers but not past capacity, which will still trigger ASan.
// Since this function by itself doesn't know what container it is operating on, it can't manually unpoison and repoison memory.
__declspec(no_sanitize_address)
SCRIPTS_API char* tt_strupr_copy(char* dest, const char* src, size_t dest_buffer_size, size_t src_buffer_size)
{
	static constexpr size_t MIN_SIZE_FOR_SIMD = 4;
	static constexpr size_t MIN_SIZE_FOR_SIMD_MASK = (1 << (MIN_SIZE_FOR_SIMD - 1)) - 1;

	size_t min_buffer_size = min(dest_buffer_size, src_buffer_size);

	int64_t count = min_buffer_size - 1;       // exclude null terminator from conversion count
	size_t block_count = min_buffer_size >> 4; // except if we can process it as part of a block

	while (block_count--)
	{
		__m128i data = _mm_loadu_si128((const __m128i*)(src));
		__m128i cmp = _mm_cmpeq_epi8(data, _mm_setzero_si128());

		int mask = _mm_movemask_epi8(cmp);

		if (mask == 0 || mask == (1 << 15)) // data block has no null terminator or it's at the very end
		{
			_mm_storeu_si128((__m128i*)(dest), tt_toupper_x16(data));

			// uppermost byte was null terminator, return immediately so we don't try processing more blocks
			if (mask == (1 << 15))
				return dest += 15;

			dest += 16;
			src += 16;
			count -= 16;
		}
		else if (mask & 1) // data block starts with null terminator, we can just null terminate and return
		{
			*dest = '\0';
			return dest;
		}
		// if there is a null terminator within the first N bytes, don't bother converting with SIMD
		else if (mask & MIN_SIZE_FOR_SIMD_MASK)
		{
			// we already have the data loaded into a register, so might as well use it instead of loading it again
			while (!(mask & 1))
			{
				*dest++ = (char)tt_toupper((unsigned char)_mm_cvtsi128_si32(data));
				mask >>= 1;
				data = _mm_bsrli_si128(data, 1); // NOTE: shift amount for this instruction is in bytes, not bits
			}
			*dest = '\0';
			return dest;
		}
		else // we have a null terminator in our data block, but not at the very start or end
		{
			// get the byte index of the null terminator in our block, that is the number of characters left to process
			unsigned long index;
			_BitScanForward(&index, mask);
			count = index;
			char* end = dest + count;

			__m128i converted = tt_toupper_x16(data);

			// store N bytes at a time, including the null terminator if possible
			if (count >= 7) {
				_mm_storeu_si64(dest, converted);
				dest += 8;
				count -= 8;
				converted = _mm_bsrli_si128(converted, 8); // NOTE: shift amount for this instruction is in bytes, not bits
			}
			if (count >= 3) {
				_mm_storeu_si32(dest, converted);
				dest += 4;
				count -= 4;
				converted = _mm_bsrli_si128(converted, 4);
			}
			if (count >= 1)
				_mm_storeu_si16(dest, converted);

			// NOTE: it's possible we wrote the null terminator already, but branching over that actually performs worse
			*end = '\0';
			return end;
		}
	}

	TT_ASSERT(count >= -1);

	// if the destination is smaller than the source, we may have copied one extra character we need to null
	if (count == -1) {
		*--dest = '\0';
		return dest;
	}

	// we get here if there are no blocks to process
	while (*src && count--) {
		*dest++ = (char)tt_toupper((unsigned char)*src++);
	}
	*dest = '\0';
	return dest;
}

// NOTE: This function can read and even write past the end of containers but not past capacity, which will still trigger ASan.
// Since this function by itself doesn't know what container it is operating on, it can't manually unpoison and repoison memory.
__declspec(no_sanitize_address)
SCRIPTS_API char* tt_strupr_copy_unsafe(char* dest, const char* src, size_t dest_buffer_size, size_t src_buffer_size)
{
	size_t min_buffer_size = min(dest_buffer_size, src_buffer_size);

	int64_t count = min_buffer_size - 1;       // exclude null terminator from conversion count
	size_t block_count = min_buffer_size >> 4; // except if we can process it as part of a block
	count -= 16 * block_count;

	while (block_count--)
	{
		__m128i data = _mm_loadu_si128((const __m128i*)(src));
		__m128i cmp = _mm_cmpeq_epi8(data, _mm_setzero_si128());

		int mask = _mm_movemask_epi8(cmp);

		if (mask != 0) // we have a null terminator in this block
		{
			// optional: write 0s past the null terminator instead of whatever garbage was in source
			//cmp = _mm_or_si128(_mm_bslli_si128(cmp, 1), cmp);
			//cmp = _mm_or_si128(_mm_bslli_si128(cmp, 2), cmp);
			//cmp = _mm_or_si128(_mm_bslli_si128(cmp, 4), cmp);
			//cmp = _mm_or_si128(_mm_bslli_si128(cmp, 8), cmp);
			//data = _mm_andnot_si128(cmp, data);

			_mm_storeu_si128((__m128i*)(dest), tt_toupper_x16(data));

			unsigned long index; // index of null terminator
			_BitScanForward(&index, mask);
			return dest + index;
		}

		_mm_storeu_si128((__m128i*)(dest), tt_toupper_x16(data));

		dest += 16;
		src += 16;
	}

	TT_ASSERT(count >= -1);

	// if the destination is smaller than the source, we may have copied one extra character we need to null
	if (count == -1) {
		*--dest = '\0';
		return dest;
	}

	// we get here if there are no blocks to process
	while (*src && count--) {
		*dest++ = (char)tt_toupper((unsigned char)*src++);
	}
	*dest = '\0';
	return dest;
}

// NOTE: This function can read and even write past the end of containers but not past capacity, which will still trigger ASan.
// Since this function by itself doesn't know what container it is operating on, it can't manually unpoison and repoison memory.
__declspec(no_sanitize_address)
SCRIPTS_API char* tt_strupr_copy_unsafe(char* dest, const char* src, size_t dest_buffer_size, size_t src_buffer_size, size_t src_length)
{
	size_t min_buffer_size = min(dest_buffer_size, src_buffer_size);

	int64_t count = min(src_length, dest_buffer_size - 1);                    // exclude null terminator from conversion count
	size_t simple_block_count = min((src_length + 1), dest_buffer_size) >> 4; // except if we can process it as part of a block
	bool can_do_last_block = (min_buffer_size - simple_block_count * 16) >= 16 && (((src_length + 1) & 15) != 0);
	count -= 16 * simple_block_count;

	while (simple_block_count--)
	{
		TT_TOUPPER_X16(dest, src);
		dest += 16;
		src += 16;
	}

	if (can_do_last_block)
	{
		__m128i data = _mm_loadu_si128((const __m128i*)(src));
		__m128i cmp = _mm_cmpeq_epi8(data, _mm_setzero_si128());

		int mask = _mm_movemask_epi8(cmp);

		unsigned long nullindex; // index of null terminator
		auto hasnull = _BitScanForward(&nullindex, mask);

		// if you pass a src_length shorter than the actual string, we need to manually insert a null terminator in the right spot
		if (!hasnull || nullindex > count)
		{
			// NOTE: I have no idea which version is better overall.
			// Lookup table wins in benchmarks because it's either already in cache or not needed.
			// This entire branch should not happen almost ever in any case.
		#if 0
			constexpr static unsigned char ones_table[16][16]{
				{0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},
				{0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},
				{0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},
				{0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},
				{0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},
				{0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},
				{0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},
				{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},
				{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},
				{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},
				{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},
				{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF},
				{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF},
				{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF},
				{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF},
				{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF}
			};
			__m128i ones = _mm_loadu_si128((__m128i*)ones_table[count]);
		#else
			__m128i ones = _mm_cvtsi32_si128(0xFF);
			if (count < 8) {
				__m128i count_xmm = _mm_cvtsi64_si128(count * 8);
				ones = _mm_sll_epi64(ones, count_xmm);
			}
			else {
				__m128i count_xmm = _mm_cvtsi64_si128((count - 8) * 8);
				ones = _mm_bslli_si128(ones, 8);
				ones = _mm_sll_epi64(ones, count_xmm);
			}
			// optional: write 0s past the null terminator instead of whatever garbage was in source
			//ones = _mm_or_si128(_mm_bslli_si128(ones, 1), ones);
			//ones = _mm_or_si128(_mm_bslli_si128(ones, 2), ones);
			//ones = _mm_or_si128(_mm_bslli_si128(ones, 4), ones);
			//ones = _mm_or_si128(_mm_bslli_si128(ones, 8), ones);
		#endif

			data = _mm_andnot_si128(ones, data);
			nullindex = (unsigned long)count;
		}
		else {
			// optional: write 0s past the null terminator instead of whatever garbage was in source
			//cmp = _mm_or_si128(_mm_bslli_si128(cmp, 1), cmp);
			//cmp = _mm_or_si128(_mm_bslli_si128(cmp, 2), cmp);
			//cmp = _mm_or_si128(_mm_bslli_si128(cmp, 4), cmp);
			//cmp = _mm_or_si128(_mm_bslli_si128(cmp, 8), cmp);
			//data = _mm_andnot_si128(cmp, data);
		}

		_mm_storeu_si128((__m128i*)(dest), tt_toupper_x16(data));

		return dest + nullindex;
	}

	TT_ASSERT(count >= -1);

	// if the destination is smaller than the source, we may have copied one extra character we need to null
	if (count == -1) {
		*--dest = '\0';
		return dest;
	}

	// we get here if there are no blocks to process
	while (*src && count--) {
		*dest++ = (char)tt_toupper((unsigned char)*src++);
	}
	*dest = '\0';
	return dest;
}


// --------------------------------------------
// Functions for case-insensitive string search
// --------------------------------------------

// specialized version for when the substring to search for is 2 characters long
static constexpr const char* stristr_2b(const unsigned char* str, const unsigned char* sub)
{
	// grab 16 bits at a time and compare at once
	uint16_t sub16 = (uint16_t)tt_tolower(sub[0]) << 8 | (uint16_t)tt_tolower(sub[1]);
	uint16_t str16 = (uint16_t)tt_tolower(str[0]) << 8 | (uint16_t)tt_tolower(str[1]);
	++str;
	while (*str && str16 != sub16)
		str16 = uint16_t((str16 << 8) | tt_tolower(*++str));

	return *str ? (const char*)str - 1 : nullptr;
}

// specialized version for when the substring to search for is 3 characters long
static constexpr const char* stristr_3b(const unsigned char* str, const unsigned char* sub)
{
	// grab 32 bits at a time and compare at once (already pre-shift up one byte so the lowest bits are always 0, otherwise we'd have to mask the upper bits)
	uint32_t sub32 = (uint32_t)tt_tolower(sub[0]) << 24 | (uint32_t)tt_tolower(sub[1]) << 16 | (uint32_t)tt_tolower(sub[2]) << 8;
	uint32_t str32 = (uint32_t)tt_tolower(str[0]) << 24 | (uint32_t)tt_tolower(str[1]) << 16 | (uint32_t)tt_tolower(str[2]) << 8;
	str += 2;
	while (*str && str32 != sub32)
		str32 = (str32 | (uint32_t)tt_tolower(*++str)) << 8;

	return *str ? (const char*)str - 2 : nullptr;
}

// specialized version for when the substring to search for is 4 characters long
static constexpr const char* stristr_4b(const unsigned char* str, const unsigned char* sub)
{
	// grab 32 bits at a time and compare at once
	uint32_t sub32 = (uint32_t)tt_tolower(sub[0]) << 24 | (uint32_t)tt_tolower(sub[1]) << 16 | (uint32_t)tt_tolower(sub[2]) << 8 | (uint32_t)tt_tolower(sub[3]);
	uint32_t str32 = (uint32_t)tt_tolower(str[0]) << 24 | (uint32_t)tt_tolower(str[1]) << 16 | (uint32_t)tt_tolower(str[2]) << 8 | (uint32_t)tt_tolower(str[3]);
	str += 3;
	while (*str && str32 != sub32)
		str32 = (str32 << 8) | (uint32_t)tt_tolower(*++str);

	return *str ? (const char*)str - 3 : nullptr;
}

// at 4 characters it starts being worth it to use SIMD instructions
static const char* stristr_4b_simd(const unsigned char* str, const unsigned char* sub)
{
	__m128i sub32 = _mm_cvtsi32_si128(_byteswap_ulong(*(unsigned long*)sub));
	__m128i str32 = _mm_cvtsi32_si128(_byteswap_ulong(*(unsigned long*)str));

	sub32 = tt_tolower_x16(sub32);
	str32 = tt_tolower_x16(str32);

	str += 3;
	while (*str && (_mm_movemask_epi8(_mm_cmpeq_epi8(str32, sub32)) != 0xFFFF))
		str32 = _mm_or_si128(_mm_slli_epi32(str32, 8), _mm_cvtsi32_si128(tt_tolower(*++str)));

	return *str ? (const char*)str - 3 : nullptr;
}

SCRIPTS_API const char* tt_stristr(const char* haystack, const char* needle)
{
	if (*needle == '\0') return haystack; // strtstr is defined to return the first argument if the substring to find is empty
	if (!haystack || *haystack == '\0')
		return nullptr;

	size_t needle_len = strlen(needle);

	if (needle_len == 1)      return tt_strichr(haystack, *needle);
	if (haystack[1] == '\0')  return nullptr; // can't match substring of length >1 within a string of length 1
	if (needle_len == 2)      return stristr_2b((const unsigned char*)haystack, (const unsigned char*)needle);
	if (haystack[2] == '\0')  return nullptr; // can't match substring of length >2 within a string of length 2
	if (needle_len == 3)      return stristr_3b((const unsigned char*)haystack, (const unsigned char*)needle);
	if (haystack[3] == '\0')  return nullptr; // can't match substring of length >3 within a string of length 3
	if (needle_len == 4)      return stristr_4b_simd((const unsigned char*)haystack, (const unsigned char*)needle);

	// Runs strchr() on the first section of the haystack as it has a lower
	// algorithmic complexity for discarding the first non-matching characters.
	haystack = tt_strichr(haystack, *needle);
	if (!haystack) // First character of needle is not in the haystack.
		return nullptr;

	// First characters of haystack and needle are the same now. Both are
	// guaranteed to be at least one character long.
	// Now computes the sum of the first needle_len characters of haystack
	// minus the sum of characters values of needle.

	const char* i_haystack = haystack + 1;
	const char* i_needle = needle + 1;

	int64_t sums_diff = 0; // NOTE: overflowing this would require a specially prepared string that is 32 petabytes large
	bool identical = true;

	while (*i_haystack && *i_needle) {
		int hc = tt_tolower((unsigned char)*i_haystack++);
		int nc = tt_tolower((unsigned char)*i_needle++);
		sums_diff += hc;
		sums_diff -= nc;
		identical &= (hc == nc);
	}

	// i_haystack now references the (needle_len + 1)-th character.

	if (*i_needle) // haystack is smaller than needle.
		return nullptr;
	else if (identical)
		return haystack;

	//size_t needle_len = i_needle - needle;
	size_t needle_len_1 = needle_len - 1;
	const int needle_first = tt_tolower((unsigned char)*needle);

	// Loops for the remaining of the haystack, updating the sum iteratively.
	const char* sub_start = haystack;
	while (*i_haystack) {
		sums_diff -= tt_tolower((unsigned char)*sub_start++);
		sums_diff += tt_tolower((unsigned char)*i_haystack++);

		// Since the sum of the characters is already known to be equal at that
		// point, it is enough to check just needle_len-1 characters for
		// equality.
		if (
			sums_diff == 0
			&& needle_first == tt_tolower((unsigned char)*sub_start) // Avoids some calls to strnicmp.
			&& tt_strnicmp(sub_start, needle, needle_len_1) == 0
			)
			return sub_start;
	}

	return nullptr;
}

SCRIPTS_API const wchar_t* wcsistr(const wchar_t* str, const wchar_t* substr) {
	if (!*str)
		return nullptr;
	size_t substr_len = wcslen(substr);
	while (*str) {
		if (_wcsnicmp(str, substr, substr_len) == 0)
			return str;
		str++;
	}
	return nullptr;
}


///////////////////////////////////////////////////////////////////////////////
//                           COPYRIGHT NOTICES


// stristr_2b, stristr_3b, stristr_4b functions adapted from 
// https://git.musl-libc.org/cgit/musl/tree/src/string/strstr.c
// 
// Copyright © 2005 - 2020 Rich Felker, et al.
// 
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files(the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and /or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
// 
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


// tt_stristr adapted from
// https://github.com/RaphaelJ/fast_strstr/blob/master/fast_strstr.c
//
// This algorithm is licensed under the open-source BSD3 license
//
// Copyright (c) 2014, Raphael Javaux
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, 
// this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors
// may be used to endorse or promote products derived from this software without
// specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.