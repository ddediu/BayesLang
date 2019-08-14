#include "Definitions.h"
#include "Permutation.h"

Permutation::Permutation( void )
:m_Size(0),m_Permutation(NULL),m_Generated(NULL)
{
}

Permutation::~Permutation()
{
	if( m_Permutation && m_Size )
		delete[] m_Permutation;
	if( m_Generated && m_Size )
		delete[] m_Generated;
}

void Permutation::Initialize( unsigned iSize )
{
	if( m_Permutation && m_Size )
		delete[] m_Permutation;
	if( m_Generated && m_Size )
		delete[] m_Generated;

	m_Permutation = new unsigned[ m_Size = iSize ];
	if( !m_Permutation )
	{
	    cerr << "Not enough memory!" << endl;
	    assert( false );
	    exit( 1 );
	}

	m_Generated = new bool[ m_Size ];
	if( !m_Generated )
	{
	    cerr << "Not enough memory!" << endl;
	    assert( false );
	    exit( 1 );
	}

	for( unsigned i = 0; i < iSize; ++i )
	{
	    m_Permutation[i] = i;
	}
}

void Permutation::GenerateRandomPermutation( void )
{
	assert( m_Permutation && m_Generated && m_Size );

	memset( m_Generated, 0, m_Size );

	for( unsigned i = 0; i < m_Size; ++i )
	{
		unsigned value = rand() % (m_Size-i);
        unsigned j,k;

		for( j = k = 0; j < m_Size; ++j )
		{
			if( m_Generated[j] == 0 && k++ == value )
				break;
		}

		assert( j != m_Size );

		m_Permutation[i] = j;
		m_Generated[j] = 1;
	}
}

