#include <iostream>
#include "chtable.hpp"
#include "hash_mixer.hpp"
#include <cassert>

#define LENGTH 30000
#define NESTCOUNT 2

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

unsigned data [LENGTH];
void init(void)
{
	unsigned seed = 10;
	for(int i = 0; i < LENGTH; i++)
	{
		seed = random(seed);
		data[i] = seed;
	}
}
int main(void)
{
	init();
	Chtable<unsigned, unsigned > t;
	unsigned cycles = 0;
	for(unsigned i = 0; i < LENGTH; i++) {
		bool good = t.Set(data[i], i);
		
		if(good) {
			unsigned val;
			std::tie(val, good) = t.Get(data[i]);
			assert(good);
			assert(val == i);
		} else {
			cycles++;
		}
	}
	std::cout << "cycles: " << cycles << std::endl;
	
	
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
	
	// test removal
	
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
	
	return 0;
 }
