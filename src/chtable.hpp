// Copyright (c) 2014, ultramailman
// This file is licensed under the MIT License.

#ifndef CHTABLE_H
#define CHTABLE_H
#include <vector>
#include <tuple>
#include <memory>
#include "hash_mixer.hpp"
namespace{
	bool isPrime(unsigned x)
	{
		if((x & 1) != 1) {
			return false;
		}
		char easy [] = {0, 1, 1, 1, 0, 1, 0, 1};
		if(x < sizeof easy) {
			return easy[x] != 0;
		}
		
		unsigned const half = x / 2;
		for(unsigned i = 3; i < half; i+=2){
			if(x % i == 0)
				return false;
		}
		return true;
	}
	unsigned nextPrime(unsigned const x)
	{
		unsigned n = x;
		if(n / 2 * 2 == n) {
			n++;
		}
		while(!isPrime(n)) {
			n += 2;
		}
		
		if(n >= x) {
			return n;
		} else {
			return -1;
		}
	}
}


namespace cuckoo{
template <class K>
class Hash{
	HashMixer<K> hashf;
public:
	Hash(unsigned n, unsigned seed)
	:
		hashf(n, seed)
	{}
	unsigned operator() (unsigned i, const K & key) const
	{
		return hashf(i, key);
	}
};

/* This is a bucket.
A bucket is an item in the hash table array.
Each bucket can have one or more key-value pairs.
*/
template <class K, class V, unsigned slots>
struct Bucket{
	unsigned count;
	K keys[slots];
	V vals[slots];
	Bucket():count(0){}
	bool full() const
	{
		return count == slots;
	}
	int Find(K const & key) const
	{
		for(unsigned i = 0; i < count; i++)
		{
			if(keys[i] == key)
			{
				return i;
			}
		}
		return -1;
	}
	void Insert(K key, V val)
	{
		keys[count] = key;
		vals[count] = val;
		count++;
	}
	bool Delete(K const & key)
	{
		for(unsigned i = 0; i < count; i++)
		{
			if(keys[i] == key)
			{
				count--;
				keys[i] = std::move(keys[count]);
				vals[i] = std::move(vals[count]);
				return true;
			}
		}
		return false;
	}
};

/* This is THE table implementation.
K: key type
V: value type
tables: the number of subtables. Must be greater than 1.
slots: the number of slots per bucket. Must be greater than 0.
Alloc: the allocator of the array.
*/
template <	
	class K, class V,
	unsigned tables = 2,
	unsigned slots = 2,
	class Alloc = std::allocator< Bucket<K, V, slots> >
>
class Table{
	// current number of key value pairs
	unsigned count_;
	
	// buckets per subtable
	unsigned table_buckets_; 
	
	// buckets per Table
	unsigned total_buckets_; 
	
	// slots per Table
	unsigned total_slots_; 
	
	// The array of buckets. It looks like one array, but it is
	// logically a two dimensional array.
	std::vector< Bucket<K, V, slots>, Alloc > array_;
	
	// A family of hash functions.
	// A hash function is chosen from the family, depending on which subtable
	//	a key value pair is going to be stored.
	Hash<K> uhash_;
	
public:
	/* Constructor.
	The size parameter is the number of pairs the table can store.
	If it is not prime, it will be raised to the next prime number.
	*/
	Table( unsigned size)
	:
		count_(0),
		
		table_buckets_( nextPrime(size / tables / slots + 1) ),
		total_buckets_( tables * table_buckets_ ),
		total_slots_(total_buckets_ * slots),
		
		array_(total_buckets_),
		
		uhash_(tables, table_buckets_)
	{}
	
	Table() : Table(13) {}

	unsigned count() const
	{
		return count_;
	}
	unsigned capacity() const
	{
		return total_slots_;
	}
private:
	// this turns a 2d array index into a 1d array index
	unsigned index(unsigned table, unsigned hash) const
	{
		return table * table_buckets_ + hash;
	}
public:
	//..................................ITERATOR
	
	// This is the container returned by the iterator * operator.
	struct Pair{
		K key;
		V val;
		Pair(K k, V v):key(std::move(k)), val(std::move(v)){}
	};
	
	// This is the iterator class.
	class Iter{
		// point to a table while the iterator is valid.
		// point to null when it is invalid.
		Table<K, V, tables, slots, Alloc> const * t;
		
		// current bucket index
		unsigned bucket_i;
		
		// current slot index within the bucket.
		unsigned slot_i;
		
	public:
		Iter(Table<K, V, tables, slots, Alloc> const * table, unsigned bucket, unsigned slot)
		:
			t(table),
			bucket_i(bucket),
			slot_i(slot)
		{}
		bool operator != (Iter const & other)
		{
			return not (other.t == t and other.bucket_i == bucket_i and other.slot_i == slot_i);
		}
		// increment slot index until it reaches the end of the bucket.
		// then increment bucket index until the next non empty bucket is found
		const Iter & operator++()
		{
			slot_i++;
			do{
				if(slot_i < t->array_[bucket_i].count)
				{
					return *this;
				}
				else
				{
					slot_i = 0;
					bucket_i++;
				}
			}while(bucket_i < t->total_buckets_);
	
			slot_i = 0;
			bucket_i = 0;
			t = nullptr;
			return *this;
		}
		Pair operator*()
		{
			auto const & bucket = t->array_[bucket_i];
			auto key = bucket.keys[slot_i];
			auto val = bucket.vals[slot_i];
			return Pair(key, val);
		}
	};
	
	// set the iterator to one before 0, then increment it.
	Iter begin() const
	{
		Iter iter(this, 0, -1);
		++iter;
		return iter;
	}
	// an end iterator is all null and zeros.
	Iter end() const
	{
		return Iter(nullptr, 0, 0);
	}
	//.......................... SEARCH ......................................
	// go through each sub table and try to find the key.
	// returns the value, and a bool indicating if the key was found.
	std::tuple<V, bool> Get(K const & key) const
	{
		for(unsigned i = 0; i < tables; i++)
		{
			unsigned hash = uhash_(i, key) % table_buckets_;
			unsigned bucket_i = index(i, hash);
			
			auto const & bucket = array_[bucket_i];
			int slot_i = bucket.Find(key);
			
			if(slot_i >= 0)
			{
				V ret = bucket.vals[slot_i];
				return std::make_tuple(ret, true);
			}
		}
		return std::make_tuple(V(), false);
	}
	//............................ INSERTION ..................................
	// insertion is more complicated, so it is split in many functions.
private:
	// Try to find the key, and update its val if it is found.
	// returns a bool indicating if the operation was successful.
	// this only needs to be executed once per insertion.
	bool update(K const & key, V val)
	{
		for(unsigned i = 0; i < tables; i++) {
			unsigned hash = uhash_(i, key) % table_buckets_;
			unsigned bucket_i = index(i, hash);
			
			auto & bucket = array_[bucket_i];
			int slot_i = bucket.Find(key);
			
			if(slot_i >= 0)
			{
				bucket.vals[slot_i] = val;
				return true;
			}
		}
		return false;
	}
	// Insert a key value pair into a subtable.
	// This operation may kick out an old occupant.
	// i is the subtable number.
	// returns true for peaceful insertion.
	// returns false for eviction.
	bool insert(K & key, V & val, unsigned i)
	{
		unsigned hash = uhash_(i, key) % table_buckets_;
		unsigned bucket_i = index(i, hash);
		
		auto & bucket = array_[bucket_i];
		if(not bucket.full())
		{
			bucket.Insert(std::move(key), std::move(val));
			count_++;
			return true;
		}
		
		std::swap(bucket.keys[0], key);
		std::swap(bucket.vals[0], val);
		
		return false;
	}

	// Try to insert a new value.
	// It can fail by running into a graph cycle.
	// returns a bool indicating success.
	// pre condition: key is not in the table
	bool tryInsert(K key, V val)
	{
		K const original = key;
		unsigned tries = tables + 1;
		unsigned i = 0;
		while(1)
		{
			if(insert(key, val, i))
			{
				return true;
			}
			if(key == original)
			{
				tries--;
				if(tries == 0)
				{
					return false;
				}
			}
			i++;
			if(i == tables)
			{
				i = 0;
			}
		}
		return false;
		
	}
	
	// Make the table bigger.
	bool grow(unsigned factor)
	{
		unsigned newSize = nextPrime(capacity() * factor);
		Table<K, V, tables, slots, Alloc> bigger( newSize );
		for(auto slot : *this)
		{
			if(not bigger.tryInsert(slot.key, slot.val))
			{
				return false;
			}
		}
		std::swap(*this, bigger);
		return true;
	}
public:
	// Writes a key and value pair.
	// This operation may increase table size.
	void Set(K key, V val)
	{
		if(update(key, val))
		{
			return;
		}
		while(not tryInsert(key, val))
		{
			unsigned factor = 2;
			while(not grow( factor ))
			{
				factor++;
			}
		}
	}
	//................................... DELETION .......................
	// Deletes a key value pair.
	// returns a bool indicating success.
	bool Delete(K const & key)
	{
		for(unsigned i = 0; i < tables; i++)
		{
			unsigned hash = uhash_(i, key) % table_buckets_;
			unsigned bucket_i = index(i, hash);
			
			auto & bucket = array_[bucket_i];
			if(bucket.Delete(key))
			{
				return true;
			}
		}
		return false;
	}
	
};
}




#endif
