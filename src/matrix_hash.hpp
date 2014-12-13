// Copyright (c) 2014, ultramailman
// This file is licensed under the MIT License.

#ifndef MATRIX_HASH_H
#define MATRIX_HASH_H
#include <climits>
template <class InType>
static bool parity(InType in)
{
	int bit = 0;
	while(in)
	{
		bit ^= in & 1;
		in >>= 1;
	}
	return bit != 0;
}

/* MatrixHash computes a hash by multiplying
a randomly generated bit matrix on the input bit vector.
*/
template < class InType, class OutType, class Gen >
class MatrixHash {
	static constexpr unsigned cols = sizeof(InType) * CHAR_BIT;
	static constexpr unsigned rows = sizeof(OutType) * CHAR_BIT;
	
	// the matrix is in column layout. (an array of column vectors)
	OutType cols_ [cols];
	
	// generate a bit string using the random generator
	OutType genString(Gen & gen)
	{
		OutType col = 0;
		
		for(unsigned i = 0; i < rows; i++)
		{
			auto num = gen();
			col |= parity(num);
			col <<= 1;
		}
		return col;
	}
public:
	MatrixHash (Gen & gen)
	{
		// initialize the matrix with random bits
		for(OutType & col: cols_)
		{
			col = genString(gen);
		}
	}
	
	// compute bit matrix multiplication on bit vector.
	OutType operator () (InType b) const
	{
		OutType x = 0;
		for(unsigned i = 0; i < cols; i++)
		{
			OutType bi = (b >> i) & 1;
			OutType mask = 0 - bi;
			OutType xi = cols_[i] & mask;
			x = x ^ xi;
		}
		return x;
	}
};
#endif
