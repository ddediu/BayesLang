#include "Language.h"

Language::Language( void )
:m_Name(NULL)
{
}

Language::Language( const char *iName )
:m_Name(NULL)
{
    Init( iName );
}

Language::Language( Language &iLanguage )
:m_Name(NULL)
{
    Init( iLanguage.m_Name );
}

Language::~Language()
{
    if( m_Name )
    {
        delete[] m_Name;
    }
}

void Language::Init( const char *iName )
{
    assert( iName );

    if( m_Name )
    {
        delete[] m_Name;
    }
    m_Name = new char[strlen(iName)+1];
    if( !m_Name )
    {
        cout << "Not enough memory!" <<  endl;
        assert(false);
        exit(1);
    }
    strcpy( m_Name, iName );
}

//Print it to file:
int Language::Print( FILE *iFile )
{
    return fprintf( iFile, "%s", m_Name );
}

//Print it to file:
unsigned Language::Print( char *iBuffer )
{
    char *curChar = iBuffer;
    curChar += sprintf( curChar, "%s", m_Name );

    return curChar - iBuffer;
}

//Copy operator:
Language &Language::operator=( Language &iLanguage )
{
    Init( iLanguage.m_Name );

    return *this;
}


