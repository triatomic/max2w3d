#pragma once

// Named vertex selection sets — used by the WWSkin Binding modifier so users
// can save groups of selected vertices (per-bone or arbitrary) and recall them
// from the modifier panel's named selection drop-down. Phase 5 of the WWSkin
// port. The on-disk chunk layout (0x0021/0x0022/0x0023) and ordering match
// EA's namedsel.cpp byte-for-byte so legacy scenes round-trip.

#include <max.h>
#include <geom/bitarray.h>

namespace W3D::MaxTools
{
	class NamedSelSetList
	{
	public:
		Tab<BitArray*> Sets;
		Tab<MSTR*>     Names;

		NamedSelSetList() = default;
		~NamedSelSetList();

		BitArray& operator[](int i) { return *Sets[i]; }
		int       Count() const { return Sets.Count(); }

		int  Find_Set(MSTR& setname);
		void Delete_Set(int i);
		void Delete_Set(MSTR& setname);
		void Reset();
		void Append_Set(BitArray& nset, MSTR& setname);
		void Set_Size(int size);

		// Deep-copy assignment (matches EA's behaviour — caller drops their list).
		NamedSelSetList& operator=(const NamedSelSetList& from);

		IOResult Save(ISave* isave);
		IOResult Load(ILoad* iload);
		IOResult Load_Set(ILoad* iload);

		enum {
			NAMED_SEL_SET_CHUNK  = 0x0021,
			NAMED_SEL_BITS_CHUNK = 0x0022,
			NAMED_SEL_NAME_CHUNK = 0x0023
		};

	private:
		// Non-copyable except via the explicit deep-copy operator= above.
		NamedSelSetList(const NamedSelSetList&) = delete;
	};
}
