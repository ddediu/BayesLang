#include "TreeTemplatesSet.h"

TreeTemplatesSet::TreeTemplatesSet()
:m_NumberOfTreeTremplets(0),m_TreeTemplates(NULL)
{
    //ctor
}

TreeTemplatesSet::~TreeTemplatesSet()
{
    FreeMemory();
}

void TreeTemplatesSet::Init( unsigned iNumberOfTreeTremplets )
{
    AllocMemory( iNumberOfTreeTremplets );
}

//Memory management:
void TreeTemplatesSet::AllocMemory( unsigned iNumberOfTreeTremplets )
{
    assert( iNumberOfTreeTremplets > 0 );

    //Free previously allocated memory:
    FreeMemory();

    m_NumberOfTreeTremplets = iNumberOfTreeTremplets;
    m_TreeTemplates = new TreeTemplate[ m_NumberOfTreeTremplets ];
    if( !m_TreeTemplates )
    {
        cerr << "Not enough memory!" << endl;
        assert( false );
        exit( 1 );
    }
}

void TreeTemplatesSet::FreeMemory( void )
{
    if( m_NumberOfTreeTremplets && m_TreeTemplates )
    {
        delete[] m_TreeTemplates;
    }
}

void TreeTemplatesSet::Print( FILE *iFile, bool paranthesized, int iWhichFeature )
{
    fprintf( iFile, "There are %d tree templates:\n", m_NumberOfTreeTremplets );
    for( unsigned i = 0; i < m_NumberOfTreeTremplets; ++i )
    {
        fprintf( iFile, "  " );
        m_TreeTemplates[i].Print( iFile, paranthesized, iWhichFeature );
        fprintf( iFile, "\n" );
    }
    fprintf( iFile, "\n" );
}


