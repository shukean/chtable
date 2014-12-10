#ifndef MATRIX_HASH_H
#define MATRIX_HASH_H
#include <climits>
template <class InType>
static bool parity(InType in)
{
	int bit = 0;
	while(in) {
		bit ^= in & 1;
		in >>= 1;
	}
	return bit != 0;
}
template < class InType, class OutType, class Gen >
class MatrixHash {
	static constexpr unsigned cols = sizeof(InType) * CHAR_BIT;
	static constexpr unsigned rows = sizeof(OutType) * CHAR_BIT;
	OutType cols_ [cols];
public:
	MatrixHash (Gen & generator)
	{
		for(OutType & col: cols_) {
			col = 0;
	// initial every bit of the column with the parity of the generated number
			for(unsigned i = 0; i < rows; i++) {
				auto num = generator();
				col |= parity(num);
				col <<= 1;
			}
		}
	}
	OutType operator () (InType k) const
	{
		OutType sum = 0;
		for(unsigned i = 0; i < cols; i++) {
			if((k >> i) & 1) {
				sum = sum ^ cols_[i];
			}
		}
		return sum;
	}
};
#endif
