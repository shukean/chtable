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


namespace chtable{
	template <class K>
	struct Hash;

}
template <class K, class V>
struct Slot{
	unsigned present;
	K key;
	V val;
};

template <class K, class V, class Alloc = std::allocator< Slot<K, V> >>
class Chtable{
	unsigned count_;
	unsigned slots_;
	unsigned tables_;
	std::vector< Slot<K, V>, Alloc > array_;
	chtable::Hash<K> uhash_;
	
	unsigned index(unsigned table, unsigned hash) const
	{
		return table * slots_ + hash;
	}
	
	bool replace(K const & key, V val);
	bool insert(K & key, V & val);
public:
	Chtable( unsigned size , unsigned tableCount)
	:
		count_(0),
		
		slots_( nextPrime(size / tableCount + 1) ),
		tables_(tableCount),
		
		array_(capacity()),
		
		uhash_(tables_, slots_)
	{}
	
	Chtable() : Chtable(13, 2) {}

	unsigned count() const { return count_; }
	unsigned capacity() const { return tables_ * slots_; }
	std::tuple<V, bool> Get(K const & key) const;
	bool Set(K key, V val);
	bool Delete(K const & key);
	
	class Iter{
		unsigned i;
		Chtable<K, V, Alloc> const * t;
	public:
		Iter(unsigned index, Chtable<K, V, Alloc> * table)
		{
			i = index;
			t = table;
		}
		bool operator != (Iter const & other)
		{
			return other.i != i and other.t != t;
		}
		const Iter & operator++()
		{
			if(i == t->capacity())
				return *this;
			while(1) {
				i++;
				if(i < t->capacity())
					return *this;
				if(t->array_[i].present)
					return *this;
			}
			return *this;
		}
		const Slot<K, V> & operator*()
		{
			return t->array_[i];
		}
	};
	
	Iter begin()
	{
		unsigned i = 0;
		while(i < capacity() and not array_[i].present)
			i++;
		return Iter(i, this);
	}
	Iter end()
	{
		Iter iter(capacity(), this);
		return Iter(capacity(), this);
	}
};

//.......................... SEARCH ......................................
template<class K, class V, class Alloc>
std::tuple<V, bool> Chtable<K,V, Alloc>::
Get(K const & key) const
{
	for(unsigned i = 0; i < tables_; i++) {
		unsigned hash = uhash_(i, key) % slots_;
		unsigned j = index(i, hash);
		auto const & slot = array_[j];
		if(slot.present and key == slot.key) {
			return std::make_tuple(slot.val, true);
		}
	}
	return std::make_tuple(V(), false);
}

//............................ INSERTION ..................................
template<class K, class V, class Alloc>
bool Chtable<K, V, Alloc>::
replace(K const & key, V val)
{
	for(unsigned i = 0; i < tables_; i++) {
		unsigned hash = uhash_(i, key) % slots_;
		unsigned j = index(i, hash);
		auto & slot = array_[j];
		if(slot.present and key == slot.key) {
			slot.val = val;
			return true;
		}
	}
	return false;
}

template<class K, class V, class Alloc>
bool Chtable<K, V, Alloc>::
insert(K & key, V & val)
{
	for(unsigned i = 0, timeout = tables_ * count_ + 1; timeout != 0; timeout--) {
		unsigned hash = uhash_(i, key) % slots_;
		unsigned j = index(i, hash);
		auto & slot = array_[j];
		std::swap(slot.key, key);
		std::swap(slot.val, val);
		
		if(slot.present) {
			i++;
			if(i == tables_) {
				i = 0;
			}
		} else {
			slot.present = true;
			count_++;
			return true;
		}
	}
	return false;
}
template<class K, class V, class Alloc>
bool Chtable<K, V, Alloc>::
Set(K key, V val)
{
	if(replace(key, val)) {
		return true;
	}
	while(not insert(key, val)) {
		Chtable<K, V> bigger( nextPrime(capacity() * 2) , tables_);
		for(auto & slot : array_) {
			if(slot.present) {
				bigger.Set(slot.key, slot.val);
			}
		}
		std::swap(*this, bigger);
	}
	return true;
}

//................................... DELETION .......................
template<class K, class V, class Alloc>
bool Chtable<K, V, Alloc>::
Delete(K const & key)
{
	for(unsigned i = 0; i < tables_; i++) {
		unsigned hash = uhash_(i, key) % slots_;
		unsigned j = index(i, hash);
		auto & slot = array_[j];
		if(slot.present and key == slot.key) {
			slot.present = false;
			slot.val = V();
			count_--;
			return true;
		}
	}
	return false;
}

#endif
