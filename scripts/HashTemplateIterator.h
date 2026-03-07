#ifndef TT_INCLUDE__HASHTEMPLATEITERATOR_H
#define TT_INCLUDE__HASHTEMPLATEITERATOR_H



#include "HashTemplateClass.h"



template<typename Key, typename Value> class HashTemplateIterator
{
	uint hash; // 0000
	int index; // 0004
	HashTemplateClass<Key, Value>* table; // 0008


	void increment()
	{
		index = table->entries[index].next;
		if (index == -1)
		{
			for (hash++; hash < table->maxHashCount; hash++)
			{
				index = table->indices[hash];
				if (index != -1)
					break;
			}
		}
	}



public:


	
	HashTemplateIterator(HashTemplateClass<Key, Value>& _table)
	{
		table = &_table;
		reset();
	}
	
	

	void reset()
	{
		index = -1;
		
		for (hash = 0; hash < table->maxHashCount; hash++)
		{
			index = table->indices[hash];
			if (index != -1)
				break;
		}
	}

	// Removes the current element and advances to the next element.
	void remove()
	{
		TT_ASSERT(*this);
		const Key &k = getKey();
		increment();
		table->Remove(k);
	}



	const Key& getKey() { return table->entries[index].key; }
	Value& getValue() { return table->entries[index].value; }
	
	void operator++() { increment(); }
	operator bool() const { return index != -1; }

}; // 000C



#endif