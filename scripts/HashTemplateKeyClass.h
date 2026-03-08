#ifndef TT_INCLUDE__HASHTEMPLATEKEYCLASS_H
#define TT_INCLUDE__HASHTEMPLATEKEYCLASS_H

#include "engine_string_view.h"

template<typename Key> class HashTemplateKeyClass
{
public:
	static uint Get_Hash_Value(const Key& key);
};

template<class T> TT_INLINE uint HashTemplateKeyClass<T>::Get_Hash_Value(const T& key)
{
	return key.GetHash();
}

TT_INLINE constexpr uint32 UInt32HashFunc32(uint32 i);
TT_INLINE constexpr uint64 UInt64HashFunc64(uint64 i);
TT_INLINE constexpr size_t SizeTypeHashFunc(size_t i);
TT_INLINE constexpr uintptr_t PointerHashFunc(void* ptr);

template<> TT_INLINE uint HashTemplateKeyClass<uint>::Get_Hash_Value(const uint& key)
{
	return UInt32HashFunc32(key);
}

TT_INLINE constexpr uint32 CombineHash32(uint32 h1, uint32 h2);
TT_INLINE constexpr uint64 CombineHash64(uint64 h1, uint64 h2);
TT_INLINE constexpr size_t CombineHash(size_t h1, size_t h2);

TT_INLINE constexpr uint32 ByteHashFunc32(const char* buf, size_t length);
TT_INLINE constexpr uint64 ByteHashFunc64(const char* buf, size_t length);
TT_INLINE constexpr size_t ByteHashFunc(const char* buf, size_t length);

TT_INLINE constexpr uint32 ByteHashFunc32_Begin();
TT_INLINE constexpr uint32 ByteHashFunc32_Add_Bytes(uint32 current_hash, const char* buf, size_t length);
TT_INLINE constexpr uint64 ByteHashFunc64_Begin();
TT_INLINE constexpr uint64 ByteHashFunc64_Add_Bytes(uint64 current_hash, const char* buf, size_t length);
TT_INLINE constexpr size_t ByteHashFunc_Begin();
TT_INLINE constexpr size_t ByteHashFunc_Add_Bytes(size_t current_hash, const char* buf, size_t length);

template<typename T>
TT_INLINE constexpr T ByteHashFunc_Begin()
{
	if constexpr (std::is_same_v<T, uint32>)
	{
		return ByteHashFunc32_Begin(); 
	}
	else if constexpr (std::is_same_v<T, uint64>)
	{
		return ByteHashFunc64_Begin(); 
	}
}

template<typename T>
TT_INLINE constexpr T ByteHashFunc_Add_Bytes(T current_hash, const char* buf, size_t length)
{
	if constexpr (std::is_same_v<T, uint32>)
	{
		return ByteHashFunc32_Add_Bytes(current_hash, buf, length);
	}
	else if constexpr (std::is_same_v<T, uint64>)
	{
		return ByteHashFunc64_Add_Bytes(current_hash, buf, length);
	}
}

template<typename T>
TT_INLINE constexpr T ByteHashFunc(const char* buf, size_t length)
{
	if constexpr (std::is_same_v<T, uint32>)
	{
		return ByteHashFunc32(buf, length);
	}
	else if constexpr (std::is_same_v<T, uint64>)
	{
		return ByteHashFunc64(buf, length);
	}
}

//struct Hash128 {
//	__m128i data;
//	TT_INLINE bool operator == (Hash128 other) const {
//		return _mm_movemask_epi8(_mm_cmpeq_epi8(data, other.data)) == 0xFFFF;
//	}
//};

typedef __m128i Hash128;

TT_INLINE bool __vectorcall operator== (Hash128 a, Hash128 b) {
	return _mm_movemask_epi8(_mm_cmpeq_epi8(a, b)) == 0xFFFF;
}

// Use these for large byte buffers, at least 128 bytes

SCRIPTS_API Hash128 LargeByteHashFunc128(const char* buf, size_t length);
SCRIPTS_API uint64 LargeByteHashFunc64(const char* buf, size_t length);
SCRIPTS_API uint32 LargeByteHashFunc32(const char* buf, size_t length);
SCRIPTS_API size_t LargeByteHashFunc(const char* buf, size_t length);


// NOTE/TODO(Mara): I will overhaul the entire way we handle string hashing and case insensitive strings in the future.
// IStringHashFunc will disappear apart from the consteval use case in HashedIString below.
// Instead we will have a custom hash map interface that converts to lowercase on input.
// This does incur a (temp) allocation per lookup, but means we convert only once and in 16 byte blocks instead of
// converting at least 3 times (1 hash and 1 comparison between two strings) per lookup and only doing so one byte at a time,
// which severely limits our speed and selection of hash functions.

TT_INLINE constexpr uint32 StringHashFunc32(StringView str);
TT_INLINE constexpr uint32 StringHashFunc32(WideStringView str);
TT_INLINE constexpr size_t StringHashFunc(StringView str);
TT_INLINE constexpr size_t StringHashFunc(WideStringView str);
TT_INLINE constexpr uint64_t StringHashFunc64(StringView str);
TT_INLINE constexpr uint64_t StringHashFunc64(WideStringView str);
TT_INLINE constexpr uint32 IStringHashFunc32(StringView str);
TT_INLINE constexpr uint32 IStringHashFunc32(WideStringView str);
TT_INLINE constexpr size_t IStringHashFunc(StringView str);
TT_INLINE constexpr size_t IStringHashFunc(WideStringView str);
TT_INLINE constexpr uint64_t IStringHashFunc64(StringView str);
TT_INLINE constexpr uint64_t IStringHashFunc64(WideStringView str);

// NOTE: Since pointers eagerly convert to StringView and StringClass eagerly converts to pointers, we need separate template helper functions
// for raw char pointers so we don't run into ambiguous overloads.
// We want a separate implementation so we don't unnecessarily iterate over the string twice (once to get the length, once to hash it).
// If we move to a hash function where having the length is beneficial enough in the future, we can still change the implementation to just call the StringView variant.

TT_INLINE constexpr size_t StringPtrHashFunc(const char* str);
TT_INLINE constexpr size_t StringPtrHashFunc(const wchar_t* str);
TT_INLINE constexpr size_t IStringPtrHashFunc(const char* str);
TT_INLINE constexpr size_t IStringPtrHashFunc(const wchar_t* str);
TT_INLINE constexpr uint32_t StringPtrHashFunc32(const char* str);
TT_INLINE constexpr uint32_t StringPtrHashFunc32(const wchar_t* str);
TT_INLINE constexpr uint32_t IStringPtrHashFunc32(const char* str);
TT_INLINE constexpr uint32_t IStringPtrHashFunc32(const wchar_t* str);
TT_INLINE constexpr uint64_t StringPtrHashFunc64(const char* str);
TT_INLINE constexpr uint64_t StringPtrHashFunc64(const wchar_t* str);
TT_INLINE constexpr uint64_t IStringPtrHashFunc64(const char* str);
TT_INLINE constexpr uint64_t IStringPtrHashFunc64(const wchar_t* str);

#define IS_CHAR_PTR(T) \
	(std::is_same_v<std::add_const_t<std::remove_pointer_t<T>>*, const char*> || \
     std::is_same_v<std::add_const_t<std::remove_pointer_t<T>>*, const wchar_t*>)

template<typename T, typename = std::enable_if_t<IS_CHAR_PTR(T)>>
TT_INLINE constexpr size_t StringHashFunc(T str) { return  StringPtrHashFunc(str); }
template<typename T, typename = std::enable_if_t<IS_CHAR_PTR(T)>>
TT_INLINE constexpr size_t IStringHashFunc(T str) { return IStringPtrHashFunc(str); }
template<typename T, typename = std::enable_if_t<IS_CHAR_PTR(T)>>
TT_INLINE constexpr uint64_t StringHashFunc64(T str) { return  StringPtrHashFunc64(str); }
template<typename T, typename = std::enable_if_t<IS_CHAR_PTR(T)>>
TT_INLINE constexpr uint64_t IStringHashFunc64(T str) { return IStringPtrHashFunc64(str); }
template<typename T, typename = std::enable_if_t<IS_CHAR_PTR(T)>>
TT_INLINE constexpr uint32_t StringHashFunc32(T str) { return  StringPtrHashFunc32(str); }
template<typename T, typename = std::enable_if_t<IS_CHAR_PTR(T)>>
TT_INLINE constexpr uint32_t IStringHashFunc32(T str) { return IStringPtrHashFunc32(str); }

#undef IS_CHAR_PTR

#include "HashTemplateKeyClass.inl"

// String pre-hashed in a case-insensitive way, if possible at compile time (can only be enforced with C++20 consteval, MSVC will *never* do it otherwise).
struct HashedIString {
	const char* Str;
	uint64_t Hash;
#if _MSVC_LANG >= 202002L
	template<size_t N> // this will *actually* enforce compile time hashing
	consteval HashedIString(const char(&str)[N]) : Str(str), Hash(IStringHashFunc64(StringView(str,N-1))) {}
#else
	template<size_t N>
	constexpr HashedIString(const char(&str)[N]) : Str(str), Hash(IStringHashFunc64(str)) {}
#endif
	template<typename T, typename = std::enable_if_t<std::is_same_v<T, char*> || std::is_same_v<T, const char*>>>
	HashedIString(const T& str) : Str(str), Hash(IStringHashFunc64(str)) {}
};

#endif