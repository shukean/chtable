// Copyright (c) 2014, ultramailman
// This file is licensed under the MIT License.

#ifndef CHTABLE_H
#define CHTABLE_H
#include <vector>
#include <tuple>
#include <memory>
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
struct Hash;

template <class K, class V, unsigned slots>
struct Bucket{
	unsigned count;
	K keys[slots];
	V vals[slots];
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

template <	
	class K, class V,
	unsigned slots = 2,
	class Alloc = std::allocator< Bucket<K, V, slots> >
>
class Table{
	unsigned count_;
	unsigned tables_;
	unsigned table_buckets_;
	unsigned total_buckets_;
	unsigned total_slots_;
	
	std::vector< Bucket<K, V, slots>, Alloc > array_;
	Hash<K> uhash_;
	
public:
	Table( unsigned size , unsigned tables )
	:
		count_(0),
		
		tables_(tables),
		table_buckets_( nextPrime(size / tables / slots + 1) ),
		total_buckets_( tables_ * table_buckets_ ),
		total_slots_(total_buckets_ * slots),
		
		array_(total_buckets_),
		
		uhash_(tables_, table_buckets_)
	{}
	
	Table() : Table(13, 2) {}

	unsigned count() const
	{
		return count_;
	}
	unsigned capacity() const
	{
		return total_slots_;
	}
private:
	unsigned index(unsigned table, unsigned hash) const
	{
		return table * table_buckets_ + hash;
	}
public:
	//..................................ITERATOR
	struct Pair{
		K key;
		V val;
		Pair(K k, V v):key(std::move(k)), val(std::move(v)){}
	};
	class Iter{
		unsigned bucket_i;
		unsigned slot_i;
		Table<K, V, slots, Alloc> const * t;
	public:
		Iter(unsigned bucket, unsigned slot, Table<K, V, slots, Alloc> const * table)
		:
			bucket_i(bucket),
			slot_i(slot),
			t(table)
		{}
		bool operator != (Iter const & other)
		{
			return other.bucket_i != bucket_i or other.slot_i != slot_i or other.t != t;
		}
		const Iter & operator++()
		{
			if(t == nullptr)
			{
				return *this;
			}
			
			slot_i++;
			if(slot_i < t->array_[bucket_i].count)
			{
				return *this;
			}
			
			slot_i = 0;
			bucket_i++;
			while(bucket_i < t->total_buckets_)
			{
				if(slot_i < t->array_[bucket_i].count)
				{
					return *this;
				}
				bucket_i++;
			}
			slot_i = -1;
			bucket_i = -1;
			t = nullptr;
			return *this;
		}
		Pair operator*()
		{
			auto & bucket = t->array_[bucket_i];
			auto key = bucket.keys[slot_i];
			auto val = bucket.vals[slot_i];
			return Pair(key, val);
		}
	};
	
	Iter begin() const
	{
		Iter iter(0, -1, this);
		++iter;
		return iter;
	}
	Iter end() const
	{
		return Iter(-1, -1, nullptr);
	}
	//.......................... SEARCH ......................................
	std::tuple<V, bool> Get(K const & key) const
	{
		for(unsigned i = 0; i < tables_; i++)
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
private:
	// update val if key is found
	bool update(K const & key, V val)
	{
		for(unsigned i = 0; i < tables_; i++) {
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
	// try to infiltrate a table.
	// that means inserting the key into one of its designated buckets.
	// returns true for peaceful infiltration.
	// returns false for eviction.
	bool infiltrate(K & key, V & val, unsigned i)
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

	// try to set a new value.
	// true if successful.
	// false if cycle.
	// pre condition: key is not in the table
	bool trySet(K key, V val)
	{
		K const original = key;
		unsigned tries = tables_ + 1;
		unsigned i = 0;
		while(1)
		{
			if(infiltrate(key, val, i))
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
			if(i == tables_)
			{
				i = 0;
			}
		}
		return false;
		
	}
	bool grow(unsigned factor)
	{
		unsigned newSize = nextPrime(capacity() * factor);
		Table<K, V, slots, Alloc> bigger( newSize , tables_);
		for(auto slot : *this)
		{
			if(not bigger.trySet(slot.key, slot.val))
			{
				return false;
			}
		}
		std::swap(*this, bigger);
		return true;
	}
public:
	void Set(K key, V val)
	{
		if(update(key, val))
		{
			return;
		}
		while(not trySet(key, val))
		{
			unsigned factor = 2;
			while(not grow( factor ))
			{
				factor++;
			}
		}
	}
	//................................... DELETION .......................
	bool Delete(K const & key)
	{
		for(unsigned i = 0; i < tables_; i++)
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
