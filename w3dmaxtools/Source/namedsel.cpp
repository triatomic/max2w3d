// Named vertex selection sets — Phase 5 of the WWSkin port. See namedsel.h.

#include "namedsel.h"

namespace W3D::MaxTools
{
	NamedSelSetList::~NamedSelSetList()
	{
		Reset();
	}

	void NamedSelSetList::Append_Set(BitArray& nset, MSTR& setname)
	{
		BitArray* b = new BitArray(nset);
		MSTR*     n = new MSTR(setname);
		Sets.Append(1, &b);
		Names.Append(1, &n);
		DbgAssert(Sets.Count() == Names.Count());
	}

	void NamedSelSetList::Delete_Set(int i)
	{
		if (i < 0 || i >= Sets.Count()) return;
		delete Sets[i]; Sets.Delete(i, 1);
		delete Names[i]; Names.Delete(i, 1);
		DbgAssert(Sets.Count() == Names.Count());
	}

	void NamedSelSetList::Delete_Set(MSTR& setname)
	{
		const int i = Find_Set(setname);
		if (i >= 0) Delete_Set(i);
	}

	void NamedSelSetList::Reset()
	{
		while (Sets.Count() > 0) Delete_Set(0);
	}

	void NamedSelSetList::Set_Size(int size)
	{
		for (int i = 0; i < Sets.Count(); ++i) Sets[i]->SetSize(size, TRUE);
	}

	int NamedSelSetList::Find_Set(MSTR& setname)
	{
		for (int i = 0; i < Names.Count(); ++i)
		{
			if (Names[i] && setname == *Names[i]) return i;
		}
		return -1;
	}

	NamedSelSetList& NamedSelSetList::operator=(const NamedSelSetList& from)
	{
		if (this == &from) return *this;
		Reset();
		// from.Count() / from.Names[i] aren't const-qualified — cast around it.
		auto& src = const_cast<NamedSelSetList&>(from);
		for (int i = 0; i < src.Count(); ++i)
		{
			Append_Set(src[i], *src.Names[i]);
		}
		return *this;
	}

	IOResult NamedSelSetList::Save(ISave* isave)
	{
		DbgAssert(Sets.Count() == Names.Count());
		for (int i = 0; i < Sets.Count(); ++i)
		{
			isave->BeginChunk(NAMED_SEL_SET_CHUNK);

			isave->BeginChunk(NAMED_SEL_NAME_CHUNK);
			isave->WriteWString(Names[i]->data());
			isave->EndChunk();

			isave->BeginChunk(NAMED_SEL_BITS_CHUNK);
			Sets[i]->Save(isave);
			isave->EndChunk();

			isave->EndChunk();
		}
		return IO_OK;
	}

	IOResult NamedSelSetList::Load(ILoad* iload)
	{
		IOResult res;
		while (IO_OK == (res = iload->OpenChunk()))
		{
			if (iload->CurChunkID() == NAMED_SEL_SET_CHUNK)
			{
				res = Load_Set(iload);
			}
			iload->CloseChunk();
			if (res != IO_OK) return res;
		}
		return IO_OK;
	}

	IOResult NamedSelSetList::Load_Set(ILoad* iload)
	{
		BitArray set;
		const MCHAR* name = nullptr;
		BOOL gotset  = FALSE;
		BOOL gotname = FALSE;

		IOResult res;
		while (IO_OK == (res = iload->OpenChunk()))
		{
			switch (iload->CurChunkID())
			{
				case NAMED_SEL_BITS_CHUNK:
					res = set.Load(iload);
					gotset = TRUE;
					break;
				case NAMED_SEL_NAME_CHUNK:
					res = iload->ReadWStringChunk(const_cast<MCHAR**>(&name));
					gotname = TRUE;
					break;
			}
			iload->CloseChunk();
			if (res != IO_OK) return res;
		}

		if (gotset && gotname)
		{
			MSTR n(name);
			Append_Set(set, n);
		}
		return IO_OK;
	}
}
