// Copyright (c) 2014, ultramailman
// This file is licensed under the MIT License.

#include <iostream>
#include <cassert>
#include <ctime>
#include "chtable.hpp"
#include "hash_mixer.hpp"

static inline unsigned random(unsigned x)
{
	return (x * 16807) % ((2 << 31) - 1);
}



template<unsigned LENGTH>
struct TableTester{
	std::vector<unsigned> data;
	std::vector<bool> membership;
	cuckoo::Table<unsigned, unsigned> t;
	double maxLoad;
	double count;
	double capacity;
	double totalLoad;
	TableTester()
	:
		data(LENGTH),
		membership(LENGTH)
	{
		unsigned seed = 10;
		for(int i = 0; i < LENGTH; i++)
		{
			seed = random(seed);
			data[i] = seed;
			membership[i] = 0;
		}
		
		maxLoad = 0;
		totalLoad = 0;
	}
	
	clock_t testInsert()
	{
		clock_t t1 = clock();
		for(unsigned i = 0; i < LENGTH; i++) {
			t.Set(data[i], i);

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
	clock_t testFind()
	{
		clock_t t1 = clock();
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
		clock_t t2 = clock();
		return t2 - t1;
	}
	
	clock_t testDelete()
	{
		clock_t t1 = clock();
		for(unsigned i = 0; i < LENGTH; i++)
		{
			bool found;
			unsigned val;
			std::tie(val, found) = t.Get(data[i]);
			if( found )
			{
				bool good = t.Delete(data[i]);
				assert(good);
				std::tie(val, found) = t.Get(data[i]);
				if( found )
					puts("removal error");
			}
		}
		clock_t t2 = clock();
		return t2 - t1;
	}
	clock_t testIterator()
	{
		unsigned count = 0;
		clock_t t1 = clock();
		for(auto pair : t)
		{
			unsigned i = pair.val;
			if(data[i] != pair.key)
				fputs("iterator error\n", stderr);
			count++;
		}
		clock_t t2 = clock();
		assert(count == LENGTH);
		return t2 - t1;
	}
};
int main(void)
{
	
	TableTester<300000> t;
	clock_t insertTime = t.testInsert();
	
	std::cout << "count: " << t.count << std::endl <<
		"capacity: " << t.capacity << std::endl <<
		"max load: " << t.maxLoad << std::endl <<
		"current load: " << t.count / t.capacity << std::endl << 
		"average load: " << t.totalLoad / t.count << std::endl <<
		"insert time: " << insertTime << std::endl;
		
	clock_t findTime = t.testFind();
	std::cout << "find time: " << findTime << std::endl;
	
	clock_t iterTime = t.testIterator();
	std::cout << "iter time: " << iterTime << std::endl;
	
	clock_t deleteTime = t.testDelete();
	std::cout << "delete time: " << deleteTime << std::endl;

	return 0;
 }
