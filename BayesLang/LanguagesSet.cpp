#include "LanguagesSet.h"

LanguagesSet::LanguagesSet( void )
:m_NumberOfLanguages(0),m_Languages(NULL)
{
}

LanguagesSet::LanguagesSet( unsigned iNumberOfLanguages )
:m_NumberOfLanguages(0),m_Languages(NULL)
{
    Init( iNumberOfLanguages );
}

LanguagesSet::~LanguagesSet()
{
    if( m_NumberOfLanguages && m_Languages )
    {
        delete[] m_Languages;
    }
}

void LanguagesSet::Init( unsigned iNumberOfLanguages )
{
    //Preconditions:
    assert( iNumberOfLanguages > 0 );

    if( m_NumberOfLanguages && m_Languages )
    {
        delete[] m_Languages;
    }

    m_NumberOfLanguages = iNumberOfLanguages;
    m_Languages = new Language[ m_NumberOfLanguages ];
    if( !m_Languages )
    {
        cerr << "Not enough memory!" << endl;
        assert( false );
        exit( 1 );
    }
}

//Print it to file:
void LanguagesSet::Print( FILE *iFile )
{
    fprintf( iFile, "There are %d languages: ", m_NumberOfLanguages );
    for( unsigned i = 0; i < m_NumberOfLanguages; ++i )
    {
        m_Languages[i].Print( iFile );
        if(i < m_NumberOfLanguages-1)
        {
            fprintf( iFile, "," );
        }
    }
    fprintf( iFile, "\n" );
}

