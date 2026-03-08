#pragma once
#include <pmmintrin.h>

#ifdef DEBUG
#pragma optimize("gt", on) // optimize even in debug builds
#endif

// -----
// concepts to reduce clutter and make flexible interfaces that can take any string type

template<typename T>
concept HasCapacity = requires(T str) {
	{ str.capacity() } -> std::same_as<size_t>;
};

template<typename T>
concept IsStringyType = requires(const T str) {
	{ str.data() } -> std::same_as<const char*>;
	{ str.length() } -> std::same_as<size_t>;
};

template<typename T>
concept IsWideStringyType = requires(const T str) {
	{ str.data() } -> std::same_as<const wchar_t*>;
	{ str.length() } -> std::same_as<size_t>;
};

template<typename T>
concept IsFullStringType = IsStringyType<T> && HasCapacity<T>;

template<typename T>
concept IsStringViewType = IsStringyType<T> && !HasCapacity<T>;

template<typename T>
concept IsFullWideStringType = IsWideStringyType<T> && HasCapacity<T>;

template<typename T>
concept IsWideStringViewType = IsWideStringyType<T> && !HasCapacity<T>;

// -----

// NOTE: please don't use this, use StringClass or std::string
SCRIPTS_API char* newstr(const char* str); //duplicate a character string
// NOTE: please don't use this, use WideStringClass or std::wstring
SCRIPTS_API wchar_t* newwcs(const wchar_t* str);  //duplicate a wide character string

SCRIPTS_API char* strtrim(char*); //trim a string
SCRIPTS_API char* strrtrim(char*); //trim trailing whitespace from a string

///////////////////////////////////////////////////////////////////////////////////////////////////
// Locale-independent functions for fast case insensitive comparisons and conversions, ASCII only!
// These functions are mostly used with file names, asset names and hardcoded strings.
// These will not directly handle extended ASCII, so e.g. Ä will *not* compare equal to ä,
// but they should still process the string fine and not break it.
// This is consistent with the standard behavior of _stricmp and friends if you don't use setlocale
// to use a more specific locale, which is the case for us, so these are direct replacements.
///////////////////////////////////////////////////////////////////////////////////////////////////

constexpr TT_INLINE int tt_isspace(int c) {
	return (c == ' ') || (((unsigned)c - '\t') < 5); // \t, \n, \v, \f, \r
}

constexpr TT_INLINE int tt_islower(int c) {
	return (((unsigned)c - 'a') < 26);
}

constexpr TT_INLINE int tt_isupper(int c) {
	return (((unsigned)c - 'A') < 26);
}

// -------------------------------------------------------
// Functions for converting characters to lower/upper case
// -------------------------------------------------------

// NOTE: int type actually improves code gen since all operators implicitly cast to int
constexpr TT_INLINE int tt_tolower(int c) {
	return (((unsigned)c - 'A') < 26) ? (c ^ 0b100000) : c;
}

// helper to prevent breakage when passing in extended ASCII chars
constexpr TT_INLINE int tt_tolower(char c) {
	return tt_tolower((unsigned char)c);
}

// NOTE: int type actually improves code gen since all operators implicitly cast to int
constexpr TT_INLINE int tt_toupper(int c) {
	return (((unsigned)c - 'a') < 26) ? (c ^ 0b100000) : c;
}

// helper to prevent breakage when passing in extended ASCII chars
constexpr TT_INLINE int tt_toupper(char c) {
	return tt_toupper((unsigned char)c);
}

// useful for very long strings or ideally ones that are exactly sizes divisible by 16 (e.g. .w3d names, bones, can include null terminator)
TT_INLINE __m128i tt_tolower_x16(__m128i packed_chars)
{
	// we only have > and < in SIMD, not >= and <=, so add/subtract 1 from the boundaries
	__m128i AAAA = _mm_set1_epi8('A' - 1);
	__m128i ZZZZ = _mm_set1_epi8('Z' + 1);
	__m128i BITS = _mm_set1_epi8(0b100000);
	// there is no unsigned comparison instruction, so we have to do either two comparisons or add some additional arithmetic
	__m128i cmp = _mm_and_si128(_mm_cmpgt_epi8(packed_chars, AAAA), _mm_cmplt_epi8(packed_chars, ZZZZ));
	__m128i lower = _mm_xor_si128(BITS, packed_chars);
	__m128i res = _mm_or_si128(_mm_and_si128(cmp, lower), _mm_andnot_si128(cmp, packed_chars));
	return res;
}
#define TT_TOLOWER_X16(dest, src) _mm_storeu_si128((__m128i*)(dest), tt_tolower_x16(_mm_loadu_si128((const __m128i*)(src))))

// useful for very long strings or ideally ones that are exactly sizes divisible by 16 (e.g. .w3d names, bones, can include null terminator)
TT_INLINE __m128i tt_toupper_x16(__m128i packed_chars)
{
	// we only have > and < in SIMD, not >= and <=, so add/subtract 1 from the boundaries
	__m128i AAAA = _mm_set1_epi8('a' - 1);
	__m128i ZZZZ = _mm_set1_epi8('z' + 1);
	__m128i BITS = _mm_set1_epi8(0b100000);
	// there is no unsigned comparison instruction, so we have to do either two comparisons or add some additional arithmetic
	__m128i cmp = _mm_and_si128(_mm_cmpgt_epi8(packed_chars, AAAA), _mm_cmplt_epi8(packed_chars, ZZZZ));
	__m128i lower = _mm_xor_si128(BITS, packed_chars);
	__m128i res = _mm_or_si128(_mm_and_si128(cmp, lower), _mm_andnot_si128(cmp, packed_chars));
	return res;
}
#define TT_TOUPPER_X16(dest, src) _mm_storeu_si128((__m128i*)(dest), tt_toupper_x16(_mm_loadu_si128((const __m128i*)(src))))


// -----------------------------------------------------------
// Functions for converting entire strings to lower/upper case
// -----------------------------------------------------------


// This is so that tt_strlwr/upr prefers the array template overload instead of the non-template function taking a plain pointer.
struct CharPtrWrapper {
	char* ptr;
	constexpr CharPtrWrapper(char* p) : ptr(p) {}
};

// Returns the pointer you provided as parameter (same as _strlwr).
constexpr char* tt_strlwr(CharPtrWrapper str) {
	char* it = str.ptr;
	while (*it) {
		*it = (char)tt_tolower((unsigned char)*it);
		it++;
	}
	return str.ptr;
}
// Returns the pointer you provided as parameter (same as _strupr).
constexpr char* tt_strupr(CharPtrWrapper str) {
	char* it = str.ptr;
	while (*it) {
		*it = (char)tt_toupper((unsigned char)*it);
		it++;
	}
	return str.ptr;
}

// Returns the pointer you provided as parameter (same as _strlwr).
constexpr char* tt_strlwr(char* begin, const char* end) {
	while (begin != end) {
		*begin = (char)tt_tolower((unsigned char)*begin);
		begin++;
	}
	return begin;
}
// Returns the pointer you provided as parameter (same as _strupr).
constexpr char* tt_strupr(char* begin, const char* end) {
	while (begin != end) {
		*begin = (char)tt_toupper((unsigned char)*begin);
		begin++;
	}
	return begin;
}

// Returns the pointer to the null terminator in the destination.
constexpr char* tt_strlwr_copy(char* dest, const char* src) {
	while (*src)
		*dest++ = (char)tt_tolower((unsigned char)*src++);
	*dest = '\0';
	return dest;
}
// Returns the pointer to the null terminator in the destination.
constexpr char* tt_strupr_copy(char* dest, const char* src) {
	while (*src)
		*dest++ = (char)tt_toupper((unsigned char)*src++);
	*dest = '\0';
	return dest;
}

// Returns the pointer to the null terminator in the destination.
// NOTE: This may copy N+1 characters and then overwrite the last one with null, there are no checks for null terminators in the source.
// This is only a problem if you pass in a count that is larger than string length (not including null) or have a non-null-terminated string.
inline char* tt_strlwr_copy_n(char* dest, const char* src, size_t count)
{
	count++; // include the null terminator in the block copy because length 15 strings are common in W3D
	size_t block_count = count >> 4;
	while (block_count--) {
		TT_TOLOWER_X16(dest, src);
		dest += 16;
		src += 16;
	}
	count = count & 15;
	if (count) count--;
	else dest--; // we copied an extra byte, go back to null terminate

	while (count--)
		*dest++ = (char)tt_tolower((unsigned char)*src++);

	*dest = '\0';
	return dest;
}
// Returns the pointer to the null terminator in the destination.
// NOTE: This may copy N+1 characters and then overwrite the last one with null, there are no checks for null terminators in the source.
// This is only a problem if you pass in a count that is larger than string length (not including null) or have a non-null-terminated string.
TT_INLINE char* tt_strlwr_n(char* str, size_t count)
{
	return tt_strlwr_copy_n(str, str, count);
}

// Returns the pointer to the null terminator in the destination.
// NOTE: This may copy N+1 characters and then overwrite the last one with null, there are no checks for null terminators in the source.
// This is only a problem if you pass in a count that is larger than string length (not including null) or have a non-null-terminated string.
inline char* tt_strupr_copy_n(char* dest, const char* src, size_t count)
{
	count++; // include the null terminator in the block copy because length 15 strings are common in W3D
	size_t block_count = count >> 4;
	while (block_count--) {
		TT_TOUPPER_X16(dest, src);
		dest += 16;
		src += 16;
	}
	count = count & 15;
	if (count) count--;
	else dest--; // we copied an extra byte, go back to null terminate

	while (count--)
		*dest++ = (char)tt_toupper((unsigned char)*src++);

	*dest = '\0';
	return dest;
}

// Returns the pointer to the null terminator in the destination.
// NOTE: This may copy N+1 characters and then overwrite the last one with null, there are no checks for null terminators in the source.
// This is only a problem if you pass in a count that is larger than string length (not including null) or have a non-null-terminated string.
TT_INLINE char* tt_strupr_n(char* str, size_t count)
{
	return tt_strupr_copy_n(str, str, count);
}


// Returns the pointer to the null terminator in the destination.
// This function will read up to 15 bytes past the null terminator in the source (if buffer size allows), but not write past where the null terminator goes in the destination.
// It will always null terminate. Do not pass buffer size 0. Buffer size includes null terminator, so e.g. std::string::capacity() + 1, but N for char[N].
// Strongly recommended to use the convenience wrappers below to prevent mistakes.
__declspec(no_sanitize_address) SCRIPTS_API char* tt_strlwr_copy(char* dest, const char* src, size_t dest_buffer_size, size_t src_buffer_size);

// Returns the pointer to the null terminator in the destination.
// This function will read up to 15 bytes past the null terminator in the source (if buffer size allows), but not write past where the null terminator goes in the destination.
// It will always null terminate. Do not pass buffer size 0. Buffer size includes null terminator, so e.g. std::string::capacity() + 1, but N for char[N].
// Strongly recommended to use the convenience wrappers below to prevent mistakes.
__declspec(no_sanitize_address) SCRIPTS_API char* tt_strupr_copy(char* dest, const char* src, size_t dest_buffer_size, size_t src_buffer_size);


// Returns the pointer to the null terminator in the destination.
// This function will read *and write* up to 15 bytes past where the null terminator is/goes (not past either buffer_size).
// It will always null terminate. Do not pass buffer size 0. Buffer size includes null terminator, so e.g. std::string::capacity() + 1, but N for char[N].
// Strongly recommended to use the convenience wrappers below to prevent mistakes.
__declspec(no_sanitize_address) SCRIPTS_API char* tt_strlwr_copy_unsafe(char* dest, const char* src, size_t dest_buffer_size, size_t src_buffer_size);

// Returns the pointer to the null terminator in the destination.
// This function will read *and write* up to 15 bytes past where the null terminator is/goes (not past either buffer_size).
// It will always null terminate. Do not pass buffer size 0. Buffer size includes null terminator, so e.g. std::string::capacity() + 1, but N for char[N].
// Strongly recommended to use the convenience wrappers below to prevent mistakes.
__declspec(no_sanitize_address) SCRIPTS_API char* tt_strupr_copy_unsafe(char* dest, const char* src, size_t dest_buffer_size, size_t src_buffer_size);


// Returns the pointer to the null terminator in the destination.
// This function will read *and write* up to 15 bytes past where the null terminator is/goes (not past either buffer_size).
// It will always null terminate. Do not pass buffer size 0. Buffer size includes null terminator, so e.g. std::string::capacity() + 1, but N for char[N].
// Strongly recommended to use the convenience wrappers below to prevent mistakes.
// Faster than the function above for long strings (>31 chars) if you already know the length, because it can copy up to src_length rapidly with no checks.
__declspec(no_sanitize_address) SCRIPTS_API char* tt_strlwr_copy_unsafe(char* dest, const char* src, size_t dest_buffer_size, size_t src_buffer_size, size_t src_length);

// Returns the pointer to the null terminator in the destination.
// This function will read *and write* up to 15 bytes past where the null terminator is/goes (not past either buffer_size).
// It will always null terminate. Do not pass buffer size 0. Buffer size includes null terminator, so e.g. std::string::capacity() + 1, but N for char[N].
// Strongly recommended to use the convenience wrappers below to prevent mistakes.
// Faster than the function above for long strings (>31 chars) if you already know the length, because it can copy up to src_length rapidly with no checks.
__declspec(no_sanitize_address) SCRIPTS_API char* tt_strupr_copy_unsafe(char* dest, const char* src, size_t dest_buffer_size, size_t src_buffer_size, size_t src_length);


// Returns the pointer to the start of the string (same as _strlwr).
// For std::string or StringClass. StringClass also has .ToLower().
// NOTE: we assume you don't care about data past the null terminator in string classes, only in raw char arrays
template<IsFullStringType T>
TT_INLINE char* tt_strlwr(T& in)
{
	char* str = in.data();
	size_t cap = in.capacity();
	tt_strlwr_copy_unsafe(str, str, cap+1, cap+1, in.length());
	return str;
}
// Returns the pointer to the start of the string (same as _strupr).
// For std::string or StringClass. StringClass also has .ToUpper().
// NOTE: we assume you don't care about data past the null terminator in string classes, only in raw char arrays
template<IsFullStringType T>
TT_INLINE char* tt_strupr(T& in)
{
	char* str = in.data();
	size_t cap = in.capacity();
	tt_strupr_copy_unsafe(str, str, cap+1, cap+1, in.length());
	return str;
}

// Returns the pointer you provided as parameter (same as _strlwr).
template<size_t N>
TT_INLINE char* tt_strlwr(char (&str)[N])
{
	tt_strlwr_copy(str, str, N, N);
	return str;
}
// Returns the pointer you provided as parameter (same as _strupr).
template<size_t N>
TT_INLINE char* tt_strupr(char (&str)[N])
{
	tt_strupr_copy(str, str, N, N);
	return str;
}

// Returns the pointer to the null terminator in the destination.
template<size_t N, size_t M>
TT_INLINE char* tt_strlwr_copy(char (&dest)[N], const char (&src)[M]) {
	return tt_strlwr_copy(dest, src, N, M);
}
// Returns the pointer to the null terminator in the destination.
template<size_t N, size_t M>
TT_INLINE char* tt_strupr_copy(char (&dest)[N], const char (&src)[M]) {
	return tt_strupr_copy(dest, src, N, M);
}

// Returns the pointer to the null terminator in the destination.
template<size_t N, IsStringyType T>
TT_INLINE char* tt_strlwr_copy(char(&dest)[N], const T& src) {
	if constexpr (IsStringViewType<T>)
		return tt_strlwr_copy(dest, src.data(), N, src.length());
	else
		return tt_strlwr_copy(dest, src.data(), N, src.capacity() + 1);
}
// Returns the pointer to the null terminator in the destination.
template<size_t N, IsStringyType T>
TT_INLINE char* tt_strupr_copy(char(&dest)[N], const T& src) {
	if constexpr (IsStringViewType<T>)
		return tt_strupr_copy(dest, src.data(), N, src.length());
	else
		return tt_strupr_copy(dest, src.data(), N, src.capacity() + 1);
}


// Returns the pointer to the null terminator in the destination.
// NOTE: we assume you don't care about data past the null terminator in string classes, only in raw char arrays
template<IsFullStringType Dest, IsStringyType Src>
TT_INLINE char* tt_strlwr_copy(Dest& dest, const Src& src) {
	size_t srclen = src.length();
	dest.resize(srclen);
	if constexpr (IsStringViewType<Src>)
		return tt_strlwr_copy_unsafe(dest.data(), src.data(), dest.capacity() + 1, srclen, srclen);
	else
		return tt_strlwr_copy_unsafe(dest.data(), src.data(), dest.capacity() + 1, src.capacity() + 1, srclen);
}

// Returns the pointer to the null terminator in the destination.
// NOTE: we assume you don't care about data past the null terminator in string classes, only in raw char arrays
template<IsFullStringType Dest, IsStringyType Src>
TT_INLINE char* tt_strupr_copy(Dest& dest, const Src& src) {
	size_t srclen = src.length();
	dest.resize(srclen);
	if constexpr (IsStringViewType<Src>)
		return tt_strupr_copy_unsafe(dest.data(), src.data(), dest.capacity() + 1, srclen, srclen);
	else
		return tt_strupr_copy_unsafe(dest.data(), src.data(), dest.capacity() + 1, src.capacity() + 1, srclen);
}

// -------------------------------------------------
// Functions for case-insensitive string comparisons
// -------------------------------------------------

constexpr int tt_stricmp(const char* left, const char* right)
{
	int left_char = 0, right_char = 0;
	do {
		left_char = tt_tolower((unsigned char)*left++);
		right_char = tt_tolower((unsigned char)*right++);
	} while (left_char == right_char && left_char != 0);

	return left_char - right_char;
}

constexpr int tt_strnicmp(const char* left, const char* right, size_t count)
{
	if (count-- == 0)
		return 0;

	int left_char = 0, right_char = 0;
	do {
		left_char = tt_tolower((unsigned char)*left++);
		right_char = tt_tolower((unsigned char)*right++);
	} while (left_char == right_char && left_char != 0 && count--);

	return left_char - right_char;
}

constexpr bool tt_stricmp_equal(const char* left, const char* right)
{
	int left_char = 0, right_char = 0;
	do {
		left_char = tt_tolower((unsigned char)*left++);
		right_char = tt_tolower((unsigned char)*right++);
	} while (left_char == right_char && left_char != 0);

	return left_char == right_char;
}

template<bool HasNullTerminator>
inline bool tt_stricmp_equal(const char* left, const char* right, size_t left_len, size_t right_len)
{
	if (left_len != right_len)
		return false;
	if (left_len == 0)
		return true;

	size_t block_count = (left_len + HasNullTerminator ? 1ull : 0ull) >> 4; // null terminator is allowed to be part of block
	int64_t count = left_len - block_count * 16;

	while (block_count--)
	{
		__m128i ldata = _mm_loadu_si128((const __m128i*)(left));
		__m128i rdata = _mm_loadu_si128((const __m128i*)(right));

		ldata = tt_tolower_x16(ldata);
		rdata = tt_tolower_x16(rdata);

		__m128i equalmask = _mm_cmpeq_epi8(ldata, rdata);

		int equalmask_int = _mm_movemask_epi8(equalmask);
		if (equalmask_int != 0xFFFF)
			return false;

		left += 16;
		right += 16;
	}

	if (count <= 0) // count can be -1 if we processed the null terminator as part of a block
		return true;

	int left_char = 0, right_char = 0;
	do {
		left_char = tt_tolower((unsigned char)*left++);
		right_char = tt_tolower((unsigned char)*right++);
	} while (left_char == right_char && --count);

	return count == 0;
}

template<IsStringyType L, IsStringyType R>
TT_INLINE bool tt_stricmp_equal(const L& left, const R& right)
{
	// if one of the types is a string view, we cannot guarantee null terminators
	constexpr bool has_null = !(IsStringViewType<L> || IsStringViewType<R>);
	return tt_stricmp_equal<has_null>(left.data(), right.data(), left.length(), right.length());
}

// convenience wrapper for wide strings
TT_INLINE bool tt_wcsicmp_equal(const wchar_t* left, const wchar_t* right)
{
	return _wcsicmp(left, right) == 0;
}

// convenience wrapper for wide strings
template<IsWideStringyType L, IsWideStringyType R>
TT_INLINE bool tt_wcsicmp_equal(const L& left, const R& right)
{
	// if one of the types is a string view, we cannot guarantee null terminators
	return (left.length() == right.length()) && (_wcsnicmp(a.data(), b.data(), left.length()) == 0);
}

// --------------------------
// case sensitive comparisons
// --------------------------

constexpr int tt_strcmp(const char* left, const char* right) {

	char left_char = 0, right_char = 0;
	do {
		left_char = *left++;
		right_char = *right++;
	} while (left_char == right_char && left_char != 0);

	return left_char - right_char;
}

constexpr bool tt_strcmp_equal(const char* left, const char* right)
{
	char left_char = 0, right_char = 0;
	do {
		left_char = *left++;
		right_char = *right++;
	} while (left_char == right_char && left_char != 0);

	return left_char == right_char;
}

template<bool HasNullTerminator>
inline bool tt_strcmp_equal(const char* left, const char* right, size_t left_len, size_t right_len)
{
	if (left_len != right_len)
		return false;
	if (left_len == 0)
		return true;

	size_t block_count = (left_len + HasNullTerminator ? 1ull : 0ull) >> 4; // null terminator is allowed to be part of block
	int64_t count = left_len - block_count * 16;

	while (block_count--)
	{
		__m128i ldata = _mm_loadu_si128((const __m128i*)(left));
		__m128i rdata = _mm_loadu_si128((const __m128i*)(right));

		__m128i equalmask = _mm_cmpeq_epi8(ldata, rdata);

		int equalmask_int = _mm_movemask_epi8(equalmask);
		if (equalmask_int != 0xFFFF)
			return false;

		left += 16;
		right += 16;
	}

	if (count <= 0) // count can be -1 if we processed the null terminator as part of a block
		return true;

	char left_char = 0, right_char = 0;
	do {
		left_char = *left++;
		right_char = *right++;
	} while (left_char == right_char && --count);

	return count == 0;
}

template<IsStringyType L, IsStringyType R>
TT_INLINE bool tt_strcmp_equal(const L& left, const R& right)
{
	// if one of the types is a string view, we cannot guarantee null terminators
	constexpr bool has_null = !(IsStringViewType<L> || IsStringViewType<R>);
	return tt_strcmp_equal<has_null>(left.data(), right.data(), left.length(), right.length());
}


constexpr int tt_wcscmp(const wchar_t* left, const wchar_t* right) {

	wchar_t left_char = 0, right_char = 0;
	do {
		left_char = *left++;
		right_char = *right++;
	} while (left_char == right_char && left_char != 0);

	return left_char - right_char;
}

constexpr bool tt_wcscmp_equal(const wchar_t* left, const wchar_t* right)
{
	wchar_t left_char = 0, right_char = 0;
	do {
		left_char = *left++;
		right_char = *right++;
	} while (left_char == right_char && left_char != 0);

	return left_char == right_char;
}

template<bool HasNullTerminator>
inline bool tt_wcscmp_equal(const wchar_t* left, const wchar_t* right, size_t left_len, size_t right_len)
{
	if (left_len != right_len)
		return false;
	if (left_len == 0)
		return true;

	size_t block_count = (left_len + HasNullTerminator ? 1ull : 0ull) >> 3; // null terminator is allowed to be part of block
	int64_t count = left_len - block_count * 8;

	while (block_count--)
	{
		__m128i ldata = _mm_loadu_si128((const __m128i*)(left));
		__m128i rdata = _mm_loadu_si128((const __m128i*)(right));

		__m128i equalmask = _mm_cmpeq_epi8(ldata, rdata);

		int equalmask_int = _mm_movemask_epi8(equalmask);
		if (equalmask_int != 0xFFFF)
			return false;

		left += 8;
		right += 8;
	}

	if (count <= 0) // count can be -1 if we processed the null terminator as part of a block
		return true;

	wchar_t left_char = 0, right_char = 0;
	do {
		left_char = *left++;
		right_char = *right++;
	} while (left_char == right_char && --count);

	return count == 0;
}

template<IsWideStringyType L, IsWideStringyType R>
TT_INLINE bool tt_wcscmp_equal(const L& left, const R& right)
{
	// if one of the types is a string view, we cannot guarantee null terminators
	constexpr bool has_null = !(IsWideStringViewType<L> || IsWideStringViewType<R>);
	return tt_wcscmp_equal<has_null>(left.data(), right.data(), left.length(), right.length());
}

// --------------------------------------------
// Functions for case-insensitive string search
// --------------------------------------------

constexpr const char* tt_strichr(const char* str, int c) {
	c = tt_tolower((unsigned char)c);
	int s = 0;
	while (*str && (s = tt_tolower((unsigned char)*str)) != c)
		str++;
	if (s == c)
		return str;
	else
		return nullptr;
}

// NOTE: on very large strings, it can be better to convert to lower case (even with heap allocations!) and then call standard strstr instead
SCRIPTS_API const char* tt_stristr(const char* str, const char* substr);
SCRIPTS_API const wchar_t* wcsistr(const wchar_t* str, const wchar_t* substr); // uses locale, slow

#ifdef DEBUG
#pragma optimize("", on)
#endif