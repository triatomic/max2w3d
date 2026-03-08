#pragma once
#include <stddef.h>
#include <type_traits>

template<class X, class Y, class Op>
struct op_valid_impl
{
	template<class U, class L, class R>
	static auto test(int) -> decltype(std::declval<U>()(std::declval<L>(), std::declval<R>()),
		void(), std::true_type());

	template<class U, class L, class R>
	static auto test(...)->std::false_type;

	using type = decltype(test<Op, X, Y>(0));
};

template<class X, class Y, class Op> using op_valid = typename op_valid_impl<X, Y, Op>::type;

template<class X, class Y = X> using has_equality = op_valid<X, Y, std::equal_to<>>;
template<class X, class Y = X> using has_inequality = op_valid<X, Y, std::not_equal_to<>>;

// NOTE(Mara): This class is a Frankenstein's monster combination of *four* original vector implementations, so there are lots of duplicated interfaces.
//             In new code, probably prefer std::vector unless you have a specific reason. I've decided on a space-efficient implementation (16 bytes).

template <class T, int VEC_GROWTH_STEP = 0>
class VectorClassBase {
protected:
	T* Vector = nullptr;
	int ActiveCount = 0;
	int VectorMax = 0;

	// Allocates uninitialised memory sized and aligned for T
	static T* AllocateVectorUninit(int count)
	{
		if constexpr (alignof(T) > PLATFORM_MEMORY_ALIGNMENT)
		{
			return (T*)(operator new(count * sizeof(T), std::align_val_t(alignof(T))));
		}
		else
		{
			return (T*)operator new(count * sizeof(T));
		}
	}
	// Only call once elements have been destroyed!
	static void DeallocateVector(T* buffer)
	{
		if (buffer)
		{
			if constexpr (alignof(T) > PLATFORM_MEMORY_ALIGNMENT)
			{
				operator delete((void*)buffer, std::align_val_t(alignof(T)));
			}
			else
			{
				operator delete((void*)buffer);
			}
		}
	}

	static void CopyElementToUninit(T* dst, const T* src)
	{
		if constexpr (std::is_trivial_v<T>)
		{
			memcpy(dst, src, sizeof(T));
		}
		// Placement new to the address
		else if constexpr (std::is_copy_constructible_v<T>)
		{
			new (dst) T(*src);
		}
		// Assign if trivially assignable (no side effects, safe to call on uninit memory)
		else if constexpr (std::is_trivially_copy_assignable_v<T>)
		{
			*dst = *src;
		}
		// Final fallback. Placement new and then copy assign
		else if constexpr (std::is_default_constructible_v<T> && std::is_copy_assignable_v<T>)
		{
			new (dst) T();
			*dst = *src;
		}
		// Your type just isn't going to work, sorry.
		else
		{
			static_assert(false, "You are attempting to use a vector with a type that cannot be moved or copied. This is not valid");
		}
	}

	static void CopyElementsToUninit(T* dst, const T* src, size_t count)
	{
		if (count == 0)
			return;

		// Memcpy if trivial
		if constexpr (std::is_trivial_v<T>)
		{
			memcpy(dst, src, sizeof(T) * count);
		}
		else
		{
			// Per-element copy
			for (size_t i = 0; i < count; ++i)
			{
				CopyElementToUninit(dst++, src++);
			}
		}
	}

	static void MoveElementToUninit(T* dst, T* src)
	{
		if constexpr (std::is_trivial_v<T>)
		{
			memcpy(dst, src, sizeof(T));
		}
		// Placement new move
		else if constexpr (std::is_move_constructible_v<T>)
		{
			new (dst) T(std::move(*src));
		}
		else if constexpr (std::is_trivially_move_assignable_v<T>)
		{
			*dst = std::move(*src);
		}
		// Placement new, assign
		else if constexpr (std::is_move_assignable_v<T> && std::is_default_constructible_v<T>)
		{
			// Can't assume that moving from uninit memory is valid (think pointer swaps)
			// So placement new and then assign
			new (dst) T;
			*dst = std::move(*src);
		}
		// Fallback to copy
		else
		{
			CopyElementToUninit(dst, src);
		}
	}

	static void MoveElementsToUninit(T* dst, T* src, size_t count)
	{
		if constexpr (std::is_trivial_v<T>)
		{
			memcpy(dst, src, sizeof(T) * count);
		}
		else
		{
			for (size_t i = 0; i < count; ++i)
			{
				MoveElementToUninit(dst++, src++);
			}
		}
	}

	TT_INLINE void MoveElementsForward(T* dst, T* src, size_t count)
	{
		if (count == 0)
			return;

		if constexpr (std::is_trivial_v<T>)
		{
			memmove(dst, src, count * sizeof(T));
		}
		else
		{
			// Moving forward to index 0. All elements should be initialised memory
			const T* srcLast = src + count;
			while(src != srcLast)
			{
				*dst++ = std::move(*src++);
			}
		}
	}

	TT_INLINE void MoveElementsReverse(T* dst, T* src, size_t count)
	{ 
		// for overlapping move where dest > src
		if (count == 0)
			return;

		if constexpr (std::is_trivial_v<T>)
		{
			memmove(dst, src, count * sizeof(T));
		}
		else
		{
			const T* srcFirst = src;
			dst = dst + count;
			src = src + count;
			while (src != srcFirst) {
				*(--dst) = std::move(*(--src));
			}
		}
	}

	void ReallocVector(int newSize)
	{
		if (newSize == VectorMax)
			return;

		if (newSize <= 0)
		{
			Clear();
			return;
		}

		T* const oldStart = Vector;
		const int oldCount = ActiveCount;

		Vector = AllocateVectorUninit(newSize);

		if (oldStart != nullptr)
		{
			const int copyCount = (std::min)(oldCount, newSize);
			MoveElementsToUninit(Vector, oldStart, copyCount);
			ActiveCount = copyCount;

			// Invoke destructors on all of the elements of the old vector (moved from or abandoned)
			if constexpr (std::is_trivially_destructible_v<T> == false)
			{
				T* const begin = oldStart;
				const T* const end = oldStart + oldCount;
				for (T* it = begin; it != end; ++it)
				{
					it->~T();
				}
			}

			DeallocateVector(oldStart);
		}

		VectorMax = newSize;
		ASAN_POISON_MEMORY_REGION(Vector + ActiveCount, (VectorMax - ActiveCount) * sizeof(T));
	}

	void Delete_All_Impl(bool allow_shrink) {
		if (allow_shrink)
			Clear();
		else
			Reset_Active();
	}

	bool Insert_Move(int index) 
	{
		if (index < 0)
			return false;

		if (index > ActiveCount)
			return false;

		if (ActiveCount == VectorMax)
			Grow();

		static_assert(std::is_default_constructible_v<T>, "You can't use insert_move with a type that isn't default construcible");

		ASAN_UNPOISON_MEMORY_REGION(Vector + ActiveCount, sizeof(T));

		// If we cannot assume that the item past the end is safe to move assign to, initialise it
		if constexpr (std::is_trivially_move_assignable_v<T> == false)
		{
			new (Vector + ActiveCount) T;
		}
		MoveElementsReverse(Vector + index + 1, Vector + index, size_t(ActiveCount - index));
		++ActiveCount;
		return true;
	}

	void Copy_Assign_Impl(int newsize, T* otherVector)
	{
		if (newsize <= 0)
		{
			Clear();
			return;
		}

		// Must call first in order to destroy any existing elements
		Reset_Active();

		// Grow if needed
		if (newsize > VectorMax)
		{
			ReallocVector(newsize);
		}

		ASAN_UNPOISON_MEMORY_REGION(Vector, newsize * sizeof(T));
		CopyElementsToUninit(Vector, otherVector, newsize);
		ActiveCount = newsize;
	}

	void Move_Assign_Impl(T*& otherVector, int& otherActiveCount, int& otherVectorMax)
	{
		// swap members
		T* thisVector = Vector;
		int thisActiveCount = ActiveCount;
		int thisVectorMax = VectorMax;
		Vector = otherVector;
		ActiveCount = otherActiveCount;
		VectorMax = otherVectorMax;
		otherVector = thisVector;
		otherActiveCount = thisActiveCount;
		otherVectorMax = thisVectorMax;
	}

public:
	using ThisClass = VectorClassBase<T, VEC_GROWTH_STEP>;
	template<int G>
	using CompatibleClass = VectorClassBase<T, G>;
	template<typename U, int G> friend class VectorClassBase;

	VectorClassBase() = default;

	explicit VectorClassBase(int size)
	{
		if (size > 0)
		{
			ReallocVector(size);
		}
	}

	// NOTE(Mara): we *need* non-template versions of copy/move constructors/assignment, otherwise the compiler will generate a default implementation that breaks everything.
	template<int G>
	VectorClassBase(const CompatibleClass<G>& other)
	{
		Copy_Assign_Impl(other.ActiveCount, other.Vector);
	}

	VectorClassBase(const ThisClass& other)
	{
		Copy_Assign_Impl(other.ActiveCount, other.Vector);
	}

	template<int G>
	ThisClass& operator=(const CompatibleClass<G>& other)
	{
		if ((void*)this != (void*)&other)
			Copy_Assign_Impl(other.ActiveCount, other.Vector);
		return *this;
	}

	ThisClass& operator=(const ThisClass& other)
	{
		if (this != &other)
			Copy_Assign_Impl(other.ActiveCount, other.Vector);
		return *this;
	}

	template<int G>
	VectorClassBase(CompatibleClass<G>&& other) noexcept
	{
		*this = std::move(other);
	}

	VectorClassBase(ThisClass&& other) noexcept
	{
		*this = std::move(other);
	}

	template<int G>
	ThisClass& operator=(CompatibleClass<G>&& other) noexcept {
		if ((void*)this != (void*)&other)
			Move_Assign_Impl(other.Vector, other.ActiveCount, other.VectorMax);
		return *this;
	}

	ThisClass& operator=(ThisClass&& other) noexcept {
		if (this != &other)
			Move_Assign_Impl(other.Vector, other.ActiveCount, other.VectorMax);
		return *this;
	}

	// NOTE: this fully deallocates the vector! Use Reset_Active() to only destroy the elements.
	void Clear()
	{
		if (Vector == nullptr)
			return;

		// Reset_Active first to make sure elements are destroyed
		Reset_Active();
		DeallocateVector(Vector);
		Vector = nullptr;
		VectorMax = 0;
	}

	~VectorClassBase()
	{
		Clear();
	}

	TT_INLINE T & operator[](int index)
	{
#if ENABLE_VECTOR_RANGE_CHECKS
		TT_RELEASE_ASSERT(index >= 0 && index < ActiveCount);
#endif
		return Vector[index];
	}

	TT_INLINE T const & operator[](int index) const
	{
#if ENABLE_VECTOR_RANGE_CHECKS
		TT_RELEASE_ASSERT(index >= 0 && index < ActiveCount);
#endif
		return Vector[index];
	}

	TT_INLINE T & Back()
	{
#if ENABLE_VECTOR_RANGE_CHECKS
		TT_RELEASE_ASSERT(ActiveCount > 0);
#endif
		return Vector[ActiveCount - 1];
	}
	
	TT_INLINE T const & Back() const
	{
#if ENABLE_VECTOR_RANGE_CHECKS
		TT_RELEASE_ASSERT(ActiveCount > 0);
#endif
		return Vector[ActiveCount - 1];
	}
	
	TT_INLINE T & Front()
	{
#if ENABLE_VECTOR_RANGE_CHECKS
		TT_RELEASE_ASSERT(ActiveCount > 0);
#endif
		return Vector[0];
	}
	
	TT_INLINE T const & Front() const
	{
#if ENABLE_VECTOR_RANGE_CHECKS
		TT_RELEASE_ASSERT(ActiveCount > 0);
#endif
		return Vector[0];
	}


	template<int G, typename U = T, typename std::enable_if_t<has_inequality<U>::value, int> = 0>
	bool operator== (CompatibleClass<G> const& other) const
	{
		if (ActiveCount == other.ActiveCount)
		{
			for (const T* p1 = Vector, *p2 = other.Vector; p1 != (Vector + ActiveCount);) {
				if (*p1++ != *p2++)
					return false;
			}
			return true;
		}
		return false;
	}

	template<int G, typename U = T, typename std::enable_if_t<!has_inequality<U>::value && has_equality<U>::value, int> = 0>
	bool operator== (CompatibleClass<G> const &other) const
	{
		if (ActiveCount == other.ActiveCount)
		{
			for (const T* p1 = Vector, *p2 = other.Vector; p1 != (Vector + ActiveCount);) {
				if (!(*p1++ == *p2++))
					return false;
			}
			return true;
		}
		return false;
	}

	void Uninitialized_Resize(int newSize, bool shrink = false) // from SimpleVecClass, sets active elements
	{
		// NOTE(Mara): this should be *at least* trivially copyable/moveable, possibly full is_trivial, but the existing usage code with math vector classes (e.g. Vector2) prevents this
		static_assert(std::is_trivially_destructible_v<T>, "Do not use uninitialized resize with non-trivial types");

		if (newSize > VectorMax || (shrink && newSize < VectorMax))
		{
			ReallocVector(newSize);
		}
		ASAN_UNPOISON_MEMORY_REGION(Vector + ActiveCount, (std::max)(0, (newSize - ActiveCount)) * sizeof(T));
		ActiveCount = VectorMax;
	}

	// Returns false if we don't need to grow (current length is greater than newlen).
	// Equivalent to std::vector::reserve()
	bool Grow(int newlen)
	{
		if (newlen > VectorMax)
		{
			ReallocVector(newlen);
			return true;
		}
		return false;
	}

	// Grows by 1.5x if growth step is 0, otherwise grows according to step size.
	void Grow()
	{
		if constexpr (VEC_GROWTH_STEP == 0)
		{
			int newlen = VectorMax + (VectorMax >> 1); // 1.5x
			if (newlen < 10) newlen = 10; // minimum length 10
			Grow(newlen);
		}
		else
		{
			Grow(VectorMax + VEC_GROWTH_STEP);
		}
	}

	void Grow_Hint(int size_hint)
	{
		if (size_hint > VectorMax)
			Grow(size_hint);
		else
			Grow();
	}

	bool Uninitialized_Grow(int newsize) // from SimpleVecClass
	{
		// NOTE(Mara): this should be *at least* trivially copyable/moveable, possibly full is_trivial, but the existing usage code with math vector classes (e.g. Vector2) prevents this
		static_assert(std::is_trivially_destructible_v<T>, "Do not use uninitialized grow with complex types");
		if (newsize > VectorMax)
		{
			ReallocVector(newsize);
			ASAN_UNPOISON_MEMORY_REGION(Vector + ActiveCount, (std::max)(0, (newsize - ActiveCount)) * sizeof(T));
			ActiveCount = newsize;
			return true;
		}
		return false;
	}

	bool Shrink() // from SimpleDynVecClass
	{
		if (ActiveCount < (VectorMax >> 2))
		{
			int newSize = ActiveCount;
			ReallocVector(newSize);
		}
		return true;
	}

	TT_INLINE int Capacity() const
	{
		return VectorMax;
	}

	TT_INLINE int Length() const
	{
		return ActiveCount;
	}

	TT_INLINE int Count() const
	{
		return ActiveCount;
	}

	TT_INLINE bool Empty() const
	{
		return ActiveCount == 0;
	}

	TT_INLINE bool isEmpty() const
	{ 
		return ActiveCount == 0;
	}

	TT_INLINE void Reset_Active()
	{
		// Destroy all elements
		if constexpr (std::is_trivially_destructible_v<T> == false)
		{
			T* const begin = Vector;
			T* const end = Vector + ActiveCount;
			for (T* it = begin; it != end; ++it)
			{
				it->~T();
			}
		}

		ActiveCount = 0;
		ASAN_POISON_MEMORY_REGION(Vector, VectorMax * sizeof(T));
	}

	TT_INLINE void Set_Active(int count)
	{
		// We may need to grow
		if (count > VectorMax)
		{
			ReallocVector(count);
		}

		// Grow
		// Initialise if not trivial

		if (count > ActiveCount)
		{
			ASAN_UNPOISON_MEMORY_REGION(Vector + ActiveCount, (count - ActiveCount) * sizeof(T));
			if constexpr (std::is_trivial_v<T> == false)
			{
				T* const begin = Vector + ActiveCount;
				T* const end = Vector + count;
				for (T* it = begin; it != end; ++it)
				{
					new (it) T;
				}
			}
		}

		// Shrink
		// Destroy if not trivial
		if constexpr (std::is_trivially_destructible_v<T> == false)
		{
			if (count < ActiveCount)
			{
				T* const begin = Vector + count;
				const T* const end = Vector + ActiveCount;
				for (T* it = begin; it != end; ++it)
				{
					it->~T();
				}
			}
		}

		ActiveCount = count;
		ASAN_POISON_MEMORY_REGION(Vector + ActiveCount, (VectorMax - ActiveCount) * sizeof(T));
	}

	TT_INLINE void Set_Active(int count, const T& initValue)
	{
		// We may need to grow
		if (count > VectorMax)
		{
			ReallocVector(count);
		}

		// Grow
		if (count > ActiveCount)
		{
			T* const begin = Vector + ActiveCount;
			T* const end = Vector + count;
			ASAN_UNPOISON_MEMORY_REGION(begin, (end - begin) * sizeof(T));
			for (T* it = begin; it != end; ++it)
			{
				CopyElementToUninit(it, &initValue);
			}
		}

		if constexpr (std::is_trivially_destructible_v<T> == false)
		{
			// Shrink, call destructors
			if (count < ActiveCount)
			{
				T* const begin = Vector + count;
				const T* const end = Vector + ActiveCount;
				for (T* it = begin; it != end; ++it)
				{
					it->~T();
				}
			}
		}
		
		ActiveCount = count;
		ASAN_POISON_MEMORY_REGION(Vector + ActiveCount, (VectorMax - ActiveCount) * sizeof(T));
	}


	TT_INLINE int ID(T const *ptr) const
	{
		return int(ptr - Vector);
	}

	template<typename U = T, typename std::enable_if_t<has_equality<U>::value, int> = 0>
	int ID(T const &object) const
	{
		for (int index = 0; index < ActiveCount; index++)
		{
			if (Vector[index] == object)
			{
				return index;
			}
		}
		return -1;
	}

	template<typename U = T, typename std::enable_if_t<has_equality<U>::value, int> = 0>
	int Find_Index(T const& object) const // from SimpleDynVecClass
	{
		return ID(object);
	}

	bool Add(const T& object, int new_size_hint = 0)
	{
		if (ActiveCount == VectorMax)
			Grow_Hint(new_size_hint);

		ASAN_UNPOISON_MEMORY_REGION(Vector + ActiveCount, sizeof(T));
		CopyElementToUninit(Vector + ActiveCount, &object);
		++ActiveCount;
		return true;
	}

	bool Add(T&& object)
	{
		Emplace_Back(std::move(object));
		return true;
	}

	template <typename... Args>
	T& Emplace_Back(Args&&... args)
	{
		if (ActiveCount == VectorMax)
			Grow();

		ASAN_UNPOISON_MEMORY_REGION(Vector + ActiveCount, sizeof(T));
		T* const emplaced = new (Vector + ActiveCount) T(std::forward<Args>(args)...);
		++ActiveCount;
		return *emplaced;
	}

	T* Uninitialized_Add()
	{
		// Should probably be is_trivial_v, but Vector and other basic classes have non-trivial constructors
		static_assert(std::is_trivially_destructible_v<T> && std::is_trivially_move_assignable_v<T>, "Do not use uninitialized add with non-trivial types");

		if (ActiveCount == VectorMax)
			Grow();
		ASAN_UNPOISON_MEMORY_REGION(Vector + ActiveCount, sizeof(T));
		return &Vector[ActiveCount++];
	}

	template<int G>
	void Add_Multiple(const CompatibleClass<G>& elements)
	{
		Add_Multiple(elements.begin(), elements.Count());
	}
	void Add_Multiple(const T* elements, int count)
	{
		int newcount = ActiveCount + count;
		Grow(newcount);
		ASAN_UNPOISON_MEMORY_REGION(Vector + ActiveCount, count * sizeof(T));
		CopyElementsToUninit(Vector + ActiveCount, elements, count);
		ActiveCount = newcount;
	}
	void Add_Multiple(int count)
	{
		int newcount = ActiveCount + count;
		Grow(newcount);
		Set_Active(newcount);
	}

	bool Add_Head(const T&  object)
	{
		return Insert(0, object);
	}

	bool Add_Head(T&& object)
	{
		Emplace_Head(std::move(object));
		return true;
	}

	template <typename... Args>
	T& Emplace_Head(Args&&... args)
	{
		return Emplace(0, std::forward<Args>(args)...);
	}

	bool Insert(int index, const T&  object)
	{
		bool index_valid = Insert_Move(index);
		if (!index_valid)
			return false;

		Vector[index] = object;
		return true;
	}

	bool Insert(int index, T&& object)
	{
		bool index_valid = Insert_Move(index);
		if (!index_valid)
			return false;

		Vector[index] = std::move(object);
		return true;
	}

	template <typename... Args>
	T& Emplace(int index, Args&&... args)
	{
		const bool index_valid = Insert_Move(index);
		if (!index_valid)
		{
			TT_UNREACHABLE
		}
		return *new (Vector + index) T(std::forward<Args>(args)...);
	}

	template<typename U = T, typename std::enable_if_t<has_equality<U>::value, int> = 0>
	bool DeleteObj(const T&  object)
	{
		int id = ID(object);
		if (id != -1)
		{
			return Delete(id);
		}
		return false;
	}
	
	// from SimpleDynVecClass, disabled if type is int because it would conflict with the function that deletes by index
	template<typename U = T, typename std::enable_if_t<has_equality<U>::value && !std::is_same_v<U,int>, int> = 0>
	bool Delete(T const& object)
	{
		int id = ID(object);
		if (id != -1)
		{
			return Delete(id);
		}
		return false;
	}

	bool Delete(int index)
	{
		TT_ASSERT(index >= 0);
		if (index >= ActiveCount)
			return false;

		MoveElementsForward(Vector + index, Vector + index + 1, size_t(ActiveCount - index - 1));
		ActiveCount--;

		// Invoke the destructor in the final element that has fallen out of the acive range
		if constexpr (std::is_trivially_destructible_v<T> == false)
		{
			(Vector + ActiveCount)->~T();
		}

		ASAN_POISON_MEMORY_REGION(Vector + ActiveCount, sizeof(T));
		return true;
	}

	template<typename U = T, typename std::enable_if_t<has_equality<U>::value, int> = 0>
	bool DeleteObj_Unordered(const T& object)
	{
		int id = ID(object);
		if (id != -1)
		{
			return Delete_Unordered(id);
		}
		return false;
	}

	template<typename U = T, typename std::enable_if_t<has_equality<U>::value && !std::is_same_v<U, int>, int> = 0>
	bool Delete_Unordered(const T& object)
	{
		int id = ID(object);
		if (id != -1)
		{
			return Delete_Unordered(id);
		}
		return false;
	}
	bool Delete_Unordered(int index)
	{
		TT_ASSERT(index >= 0);
		if (index >= ActiveCount)
			return false;

		ActiveCount--;
		if (index < ActiveCount)
			Vector[index] = std::move(Vector[ActiveCount]);
		
		// Invoke the destructor on the deleted item
		if constexpr (std::is_trivially_destructible_v<T> == false)
		{
			(Vector + ActiveCount)->~T();
		}

		ASAN_POISON_MEMORY_REGION(Vector + ActiveCount, sizeof(T));
		return true;
	}
	bool Delete_Range(int start, int count) // from SimpleDynVecClass
	{
		TT_ASSERT(start >= 0 && (start + count) <= ActiveCount);
		if (start < ActiveCount - count)
		{
			MoveElementsForward(Vector + start, Vector + start + count, size_t(ActiveCount - start - count));
		}
		const int prevActiveCount = ActiveCount;
		ActiveCount -= count;

		// Invoke the destructor on the deleted items
		if constexpr (std::is_trivially_destructible_v<T> == false)
		{
			T* const begin = Vector + ActiveCount;
			T* const end = Vector + prevActiveCount;
			for (T* it = begin; it != end; ++it)
			{
				it->~T();
			}
		}

		ASAN_POISON_MEMORY_REGION(Vector + ActiveCount, count * sizeof(T));
		return true;
	}

	// from SimpleDynVecClass, NOTE(Mara): originally returned a T& but I decided that is too dangerous, all the existing usage code is for pointer types, so copy is cheap
	template<typename U = T, typename std::enable_if_t<!std::is_array_v<U>, int> = 0>
	TT_INLINE U Pop_Back()
	{
		U value = std::move(Vector[--ActiveCount]);
		// Invoke the destructor on the deleted items
		if constexpr (std::is_trivially_destructible_v<T> == false)
		{
			Vector[ActiveCount]->~T();
		}

		ASAN_POISON_MEMORY_REGION(Vector + ActiveCount, sizeof(T));
		return value;
	}

	void qsort(int(*compareCallback)(const void*, const void*))
	{
		::qsort(Vector, size_t(ActiveCount), sizeof(T), compareCallback);
	}
	void qsort(int(*compareCallback)(const T&, const T&))
	{
		::qsort(Vector, size_t(ActiveCount), sizeof(T), (int(*)(const void*, const void*))compareCallback);
	}

	void Zero_Memory() // from SimpleVecClass
	{
		static_assert(std::is_trivial_v<T>, "Zeroing memory on non-trivial types is not supported");
		if (Vector != nullptr)
		{
			memset(Vector, 0, size_t(VectorMax) * sizeof(T));
		}
	}

	// C++11 iterator/range-for support

	TT_INLINE T* begin() noexcept {
		return Vector;
	}
	TT_INLINE const T* begin() const noexcept {
		return Vector;
	}
	TT_INLINE T* end() noexcept {
		return Vector + ActiveCount;
	}
	TT_INLINE const T* end() const noexcept{
		return Vector + ActiveCount;
	}
};

// NOTE(Mara): We need these extra classes because:
//             1. There are a lot of forward declarations that would break with a using statement
//             2. The "dynamic" variants do not set the active count to the total size in both Resize and the constructor.
//                Breaking the original behavior would require hundreds of careful changes across the entire codebase.
//             3. The "simple" variants default to shrinking on deleting elements, I've decided to keep that behavior for Delete_All
//                just in case it's important at one of the usage sites.

// NOTE(Mara): need to use this to forward declare to get the default growth step parameter without compiler warnings/errors
#include "vector_forward_decl.h"

// boiler plate, calls base class copy/move constructors and assignment operators
#define DECLARE_CONSTRUCTORS_AND_ASSIGNMENTS(ClassName) \
	using ThisClass = ClassName<T, VEC_GROWTH_STEP>;                                  \
	template<int G>                                                                   \
	using CompatibleClass = ClassName<T, G>;                                          \
	template<int G>                                                                   \
	ClassName(CompatibleClass<G> const& other) : BaseClass(other) {}                  \
	ClassName(ThisClass const& other) : BaseClass(other) {}                           \
	template<int G>                                                                   \
	ClassName(CompatibleClass<G>&& other) noexcept : BaseClass(std::move(other)) {}   \
	ClassName(ThisClass&& other) noexcept : BaseClass(std::move(other)) {}            \
	template<int G>                                                                   \
	ThisClass& operator= (CompatibleClass<G> const& other) {                          \
		BaseClass::operator=(other);                                                  \
		return *this;                                                                 \
	}                                                                                 \
	ThisClass& operator= (ThisClass const& other) {                                   \
		BaseClass::operator=(other);                                                  \
		return *this;                                                                 \
	}                                                                                 \
	template<int G>                                                                   \
	ThisClass& operator=(CompatibleClass<G>&& other) noexcept {                       \
		BaseClass::operator=(std::move(other));                                       \
		return *this;                                                                 \
	}                                                                                 \
	ThisClass& operator=(ThisClass&& other) noexcept {                                \
		BaseClass::operator=(std::move(other));                                       \
		return *this;                                                                 \
	}

template <class T, int VEC_GROWTH_STEP> class VectorClass : public VectorClassBase<T, VEC_GROWTH_STEP> {
public:
	using BaseClass = VectorClassBase<T, VEC_GROWTH_STEP>;

	DECLARE_CONSTRUCTORS_AND_ASSIGNMENTS(VectorClass);

	// NOTE(Mara): The non-"dynamic" vector interfaces *do* create active/initialized elements on construction if size > 0, it's like std::vector(size)
	explicit VectorClass(int size = 0) : BaseClass(size) {
		BaseClass::Set_Active(size);
	}
	// NOTE(Mara): The non-"dynamic" vector interfaces *do* increase the number of active/initialized elements on Resize, it's like std::vector::resize
	bool Resize(int newSize)
	{
		BaseClass::ReallocVector(newSize);
		BaseClass::Set_Active(newSize);
		return true;
	}
	// NOTE(Mara): The non-"simple" vector interfaces default to *not* shrinking on Delete_All
	void Delete_All(bool allow_shrink = false) {
		BaseClass::Delete_All_Impl(allow_shrink);
	}
};

template <class T, int VEC_GROWTH_STEP> class SimpleVecClass : public VectorClassBase<T, VEC_GROWTH_STEP> {
public:
	using BaseClass = VectorClassBase<T, VEC_GROWTH_STEP>;
	using ThisClass = SimpleVecClass<T, VEC_GROWTH_STEP>;
	template<int G>
	using CompatibleClass = SimpleVecClass<T, G>;

	DECLARE_CONSTRUCTORS_AND_ASSIGNMENTS(SimpleVecClass);

	// NOTE(Mara): The non-"dynamic" vector interfaces *do* create active/initialized elements on construction if size > 0, it's like std::vector(size)
	explicit SimpleVecClass(int size = 0) : BaseClass(size) {
		BaseClass::Set_Active(size);
	}
	// NOTE(Mara): The non-"dynamic" vector interfaces *do* increase the number of active/initialized elements on Resize, it's like std::vector::resize
	bool Resize(int newSize)
	{
		BaseClass::ReallocVector(newSize);
		BaseClass::Set_Active(newSize);
		return true;
	}
	// NOTE(Mara): The "simple" vector interfaces default to shrinking on Delete_All
	void Delete_All(bool allow_shrink = true) {
		BaseClass::Delete_All_Impl(allow_shrink);
	}
};

template <class T, int VEC_GROWTH_STEP> class DynamicVectorClass : public VectorClassBase<T, VEC_GROWTH_STEP> {
public:
	using BaseClass = VectorClassBase<T, VEC_GROWTH_STEP>;
	using ThisClass = DynamicVectorClass<T, VEC_GROWTH_STEP>;
	template<int G>
	using CompatibleClass = DynamicVectorClass<T, G>;

	DECLARE_CONSTRUCTORS_AND_ASSIGNMENTS(DynamicVectorClass);

	// NOTE(Mara): The "dynamic" vector interfaces do *not* create active/initialized elements on construction if size > 0, it acts like a std::vector::reserve(size)
	DynamicVectorClass() = default;
	explicit DynamicVectorClass(int size) : BaseClass(size) {}

	// NOTE(Mara): The "dynamic" vector interfaces do *not* increase the number of active/initialized elements on Resize,
	//             it acts like a std::vector::reserve if the new size is greater than current
	bool Resize(int newSize) {
		BaseClass::ReallocVector(newSize);
		return true;
	}
	// NOTE(Mara): The non-"simple" vector interfaces default to *not* shrinking on Delete_All
	void Delete_All(bool allow_shrink = false) {
		BaseClass::Delete_All_Impl(allow_shrink);
	}
};

template <class T, int VEC_GROWTH_STEP> class SimpleDynVecClass : public VectorClassBase<T, VEC_GROWTH_STEP> {
public:
	using BaseClass = VectorClassBase<T, VEC_GROWTH_STEP>;
	using ThisClass = SimpleDynVecClass<T, VEC_GROWTH_STEP>;
	template<int G>
	using CompatibleClass = SimpleDynVecClass<T, G>;

	DECLARE_CONSTRUCTORS_AND_ASSIGNMENTS(SimpleDynVecClass);

	// NOTE(Mara): The "dynamic" vector interfaces do *not* create active/initialized elements on construction if size > 0, it acts like a std::vector::reserve(size)
	SimpleDynVecClass() = default;

	explicit SimpleDynVecClass(int size) : BaseClass(size) {}

	// NOTE(Mara): The "dynamic" vector interfaces do *not* increase the number of active/initialized elements on Resize,
	//             it acts like a std::vector::reserve if the new size is greater than current
	bool Resize(int newSize) {
		BaseClass::ReallocVector(newSize);
		return true;
	}
	// NOTE(Mara): The "simple" vector interfaces default to shrinking on Delete_All
	void Delete_All(bool allow_shrink = true) {
		BaseClass::Delete_All_Impl(allow_shrink);
	}
};
