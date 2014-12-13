// Copyright (c) 2014, ultramailman
// This file is licensed under the MIT License.

#ifndef HASH_MIXER_H
#define HASH_MIXER_H
#include "matrix_hash.hpp"
#include <random>
#include <vector>

/* HashMixer is an example hash function for cuckoo hash table.
Use this if you don't have a better parameterized hash function.
*/
template < class InType >
class HashMixer{
	std::vector< MatrixHash<unsigned, unsigned, std::mt19937 > > mats_;
	std::hash<InType> hashf;
public:
	/* n is the number of functions.
	seed is the seed of the random number generator.
	*/
	HashMixer(unsigned n, unsigned seed)
	{
		std::mt19937 g (seed);
		for(unsigned i = 0; i < n; i++) {
			mats_.emplace_back(g);
		}
	}
	unsigned operator () (unsigned i, InType k) const
	{
		return mats_[i](hashf(k));
	}
};
#endif
