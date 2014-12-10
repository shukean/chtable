// Copyright (c) 2014, ultramailman
// This file is licensed under the MIT License.

#ifndef CHTABLE_H
#define CHTABLE_H
#include <vector>
#include <tuple>
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
class Chtable{
	unsigned count_;
	unsigned slots_;
	unsigned const tables_;
	std::vector< bool > present_;
	std::vector< K > keys_;
	std::vector< V > values_;
	chtable::Hash<K> uhash_;
	
	unsigned index(unsigned table, unsigned hash) const
	{
		return table * slots_ + hash;
	}
	
	unsigned totalSlots() const
	{
		return tables_ * slots_;
	}
public:
	Chtable( unsigned size , unsigned tableCount = 2)
	:
		count_(0),
		
		slots_( nextPrime(size / tableCount + 1) ),
		tables_(tableCount),
		
		present_(totalSlots()),
		keys_(totalSlots()),
		values_(totalSlots()),
		
		uhash_(tables_, slots_)
	{}

	std::tuple<V, bool> Get(K const & key) const
	{
		for(unsigned i = 0; i < tables_; i++) {
			unsigned hash = uhash_(i, key) % slots_;
			unsigned j = index(i, hash);
			if(present_[j] and key == keys_[j]) {
				return std::make_tuple(values_[j], true);
			}
		}
		return std::make_tuple(V(), false);
	}
	
private:
	bool replace(K const & key, V val)
	{
		for(unsigned i = 0; i < tables_; i++) {
			unsigned hash = uhash_(i, key) % slots_;
			unsigned j = index(i, hash);
			if(present_[j] and key == keys_[j]) {
				values_[j] = val;
				return true;
			}
		}
		return false;
	}
public:
	bool Set(K key, V val, unsigned timeout)
	{
		if(replace(key, val)) {
			return true;
		}
		
		for(unsigned i = 0; timeout != 0; timeout--) {
			unsigned hash = uhash_(i, key) % slots_;
			unsigned j = index(i, hash);
			
			std::swap(keys_[j], key);
			std::swap(values_[j], val);
			
			if(present_[j]) {
				i++;
				if(i == tables_) {
					i = 0;
				}
			} else {
				present_[j] = true;
				count_++;
				return true;
			}
		}
		return false;
	}
	
	bool Set(K key, V val)
	{
		return Set(key, val, count_ + 1);
	}
	
	bool Delete(K const & key)
	{
		for(unsigned i = 0; i < tables_; i++) {
			unsigned hash = uhash_(i, key) % slots_;
			unsigned j = index(i, hash);
			if(present_[j] and key == keys_[j]) {
				present_[j] = false;
				values_[j] = V();
				count_--;
				return true;
			}
		}
		return false;
	}
	
};
#endif
