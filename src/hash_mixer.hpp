// Copyright (c) 2014, ultramailman
// This file is licensed under the MIT License.

#ifndef UNI_HASH_H
#define UNI_HASH_H
#include "matrix_hash.hpp"
#include <random>
#include <vector>
template < class InType >
class HashMixer{
	std::vector< MatrixHash<unsigned, unsigned, std::mt19937 > > mats_;
	std::hash<InType> hashf;
public:
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
