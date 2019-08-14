#ifndef PERMUTATION_H
#define PERMUTATION_H

#include "Definitions.h"
//Generates radom permutations:
class Permutation
{
public:
	Permutation( void );
	virtual ~Permutation();

	void Initialize( unsigned iSize );
	void GenerateRandomPermutation( void );
	unsigned operator[]( unsigned index ){ return m_Permutation[index]; }

private:
	unsigned m_Size;
	unsigned *m_Permutation;
	bool *m_Generated;

};

#endif // PERMUTATION_H
