#pragma once
#include "fileclass.h"
class RAMFileClass final : public FileClass
{
private:
	char* Buffer;
	int MaxLength;
	int Length;
	int Offset = 0;
	int Access = OpenMode::OPEN_READ;
	bool IsOpen = false;
	bool IsAllocated = false;
	bool Reallocate = false;
	bool IsHashChecked = false;
public:
	RAMFileClass(std::unique_ptr<unsigned char[]>&& data, int len, bool IsHashChecked = false) : Buffer((char*)data.release()), MaxLength(len), Length(len), IsAllocated(true), IsHashChecked(IsHashChecked) {}
	RAMFileClass(void* buffer, int len, bool deletemem = false, bool IsHashChecked = false) : Buffer((char*)buffer), MaxLength(len), Length(len), IsHashChecked(IsHashChecked)
	{
		if (!buffer && len > 0)
		{
			Buffer = new char[len];
			IsAllocated = true;
		}
		else if (buffer && deletemem)
		{
			IsAllocated = true;
		}
	}
	virtual ~RAMFileClass() override
	{
		IsOpen = false;
		if (IsAllocated)
		{
			delete[] Buffer;
			Buffer = nullptr;
			IsAllocated = false;
		}
	}
	virtual char const* File_Name() const override
	{
		return("UNKNOWN");
	}
	virtual char const* Set_Name(char const*) override
	{
		return File_Name();
	}
	virtual bool Create() override
	{
		if (Is_Open())
		{
			return false;
		}
		Length = 0;
		return true;
	}
	virtual bool Delete() override
	{
		if (Is_Open())
		{
			return false;
		}
		Length = 0;
		return true;
	}
	virtual bool Is_Available(int forced) override
	{
		return true;
	}
	virtual bool Is_Open() const override
	{
		return IsOpen;
	}
	virtual int Open(char const* filename, int access) override
	{
		return Open(access);
	}
	virtual int Open(int access) override
	{
		if (!Buffer || Is_Open())
		{
			return 0;
		}
		Offset = 0;
		Access = access;
		IsOpen = true;
		if (access == OpenMode::OPEN_WRITE)
		{
			Length = 0;
		}
		return Is_Open();
	}
	virtual int Read(void* buffer, int size) override
	{
		if (!Buffer || !buffer || !size)
		{
			return 0;
		}
		bool close = false;
		if (!Is_Open()) {
			Open(OpenMode::OPEN_READ);
			close = true;
		}
		if (!IsOpen || !(Access & OpenMode::OPEN_READ))
		{
			return 0;
		}
		int len = Length - Offset;
		if (size < len)
		{
			len = size;
		}
		memmove(buffer, &Buffer[Offset], len);
		Offset += len;
		if (close)
		{
			Close();
		}
		return len;
	}
	virtual int Seek(int pos, int dir) override
	{
		if (!Buffer || !Is_Open())
		{
			return Offset;
		}
		int len = Length;
		if (Access & OpenMode::OPEN_WRITE)
		{
			len = MaxLength;
		}
		switch (dir)
		{
		case ORIGIN_START:
			Offset = pos;
			break;
		case ORIGIN_CURRENT:
			Offset += pos;
			break;
		case ORIGIN_END:
			Offset = len + pos;
			break;
		}
		if (Offset < 0)
		{
			Offset = 0;
		}
		if (Offset > len)
		{
			Offset = len;
		}
		if (Offset > Length)
		{
			Length = Offset;
		}
		return Offset;
	}
	virtual int Size() override
	{
		return Length;
	}
	virtual int Write(void const* buffer, int size) override
	{
		if (!Buffer || !buffer || !size)
		{
			return 0;
		}
		bool close = false;
		if (!Is_Open())
		{
			Open(OpenMode::OPEN_WRITE);
			close = true;
		}
		if (!IsOpen || !(Access & OpenMode::OPEN_WRITE))
		{
			return 0;
		}
		if (Reallocate)
		{
			int new_maxlength = MaxLength;
			while (size >= (new_maxlength - Offset)) {
				new_maxlength *= 2;
			}
			char* old_buffer = Buffer;
			Buffer = new char[new_maxlength];
			memcpy(Buffer, old_buffer, Length);
			delete[] old_buffer;
			MaxLength = new_maxlength;
		}
		if (size >= MaxLength - Offset)
		{
			size = MaxLength - Offset;
		}
		memmove(&Buffer[Offset], buffer, size);
		Offset += size;
		if (Offset > Length)
		{
			Length = Offset;
		}
		if (close)
		{
			Close();
		}
		return size;
	}
	virtual void Close() override
	{
		IsOpen = false;
	}
	virtual unsigned long Get_Date_Time() override
	{
		return 0;
	}
	virtual bool Set_Date_Time(unsigned long datetime) override
	{
		return true;
	}
	virtual void Error(int a, int b, const char* c) override
	{
	}
	virtual void Bias(int start, int length) override
	{
		Buffer += start;
		int len = Length;
		if (len >= start + length)
		{
			len = start + length;
		}
		Length = len - start;
		int mlen = MaxLength;
		if (mlen >= start + length)
		{
			mlen = start + length;
		}
		MaxLength = mlen - start;
		if (Is_Open())
		{
			Seek(0, ORIGIN_START);
		}
	}
	// NOTE: files inside mix files are "already checked", see also RawFileClass::Is_Hash_Checked
	virtual bool Is_Hash_Checked() const override
	{
		return IsHashChecked;
	}
	void Set_Reallocate(bool reallocate)
	{
		Reallocate = reallocate;
	}
	char* Get_Buffer()
	{
		return Buffer;
	}
	int Get_Length()
	{
		return Length;
	}
};