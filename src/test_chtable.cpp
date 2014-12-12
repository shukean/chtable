#include <iostream>
#include <cassert>
#include <ctime>
#include "chtable.hpp"
#include "hash_mixer.hpp"



namespace chtable{
template <>
class Hash<unsigned> {
	HashMixer<unsigned> hashf_;
public:
	Hash(unsigned k, unsigned seed) : hashf_(k, seed){}
	unsigned operator () (unsigned i, unsigned k) const
	{
		return hashf_(i, k);
	}
};
}
static inline unsigned random(unsigned x)
{
	return (x * 16807) % ((2 << 31) - 1);
}



template<unsigned LENGTH>
struct ChtableTester{
	unsigned data [LENGTH];
	Chtable<unsigned, unsigned > t;
	double maxLoad;
	double count;
	double capacity;
	double totalLoad;
	ChtableTester()
		:t(13, 2)
	{
		unsigned seed = 10;
		for(int i = 0; i < LENGTH; i++)
		{
			seed = random(seed);
			data[i] = seed;
		}
		
		maxLoad = 0;
		totalLoad = 0;
	}
	
	clock_t testInsert()
	{
		clock_t t1 = clock();
		for(unsigned i = 0; i < LENGTH; i++) {
			bool good = t.Set(data[i], i);
			assert(good);

			bool found;
			unsigned val;
			std::tie(val, found) = t.Get(data[i]);
			assert(found);
			assert(val == i);
			
			count = t.count();
			capacity = t.capacity();
			double load = count / capacity;
			totalLoad += load;
			if(load > maxLoad)
				maxLoad = load;
		}
		clock_t t2 = clock();
		return t2 - t1;
	}
	void testFind()
	{
		for(unsigned i = 0; i < LENGTH; i++)
		{
			bool found;
			unsigned val;
			std::tie(val, found) = t.Get(data[i]);
			if( !found )
				fputs("find error\n", stderr);
			else if( val != i)
				fputs("data error 3\n", stderr);
		}
	}
	
	void testDelete()
	{
		for(unsigned i = 0; i < LENGTH; i++)
		{
			bool found;
			unsigned val;
			std::tie(val, found) = t.Get(data[i]);
			if( found )
			{
				t.Delete(data[i]);
				
				std::tie(val, found) = t.Get(data[i]);
				if( found )
					puts("removal error");
			}
		}
	}
};
int main(void)
{
	
	ChtableTester<30000> t;
	clock_t insertTime = t.testInsert();
	
	std::cout << "count: " << t.count << std::endl <<
		"capacity: " << t.capacity << std::endl <<
		"max load: " << t.maxLoad << std::endl <<
		"current load: " << t.count / t.capacity << std::endl << 
		"average load: " << t.totalLoad / t.count << std::endl <<
		"time: " << insertTime << std::endl;
	t.testFind();
	t.testDelete();
	
	/*
	// test iterator
	int membership[128]={0};
	struct chtable_Iterator it;
	chtable_IteratorInit(&it, map);
	while(chtable_IteratorNext(&it))
	{
		int key, val;
		if(chtable_IteratorGet(&it, &key, &val) != 0)
			fputs("iterator failure\n", stderr);
		if(key != val)
			fputs("data error\n", stderr);
		if(key < 0 || key > 127)
			fputs("data error 2\n", stderr);
		membership[key]++;
	}
	for(int i = 0; i < 128; i++)
	{
		if(membership[i] != 1)
			fputs("membership error\n", stderr);
	}
	*/
	
	// test find
	
	
	// test removal
	
	
	
	return 0;
 }
