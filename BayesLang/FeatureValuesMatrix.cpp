#include "FeatureValuesMatrix.h"

//The missing data value:
const int FeatureValuesMatrix::c_MissingData = (-1);

FeatureValuesMatrix::FeatureValuesMatrix()
:m_NumberOfLanguages(0),m_LanguagesSet(NULL),m_LanguagesIndices(NULL),m_NumberOfFeatures(0),m_FeaturesSet(NULL),m_FeaturesIndices(NULL),m_Values(NULL)
{
}

FeatureValuesMatrix::FeatureValuesMatrix( unsigned iNumberOfLanguages, unsigned iNumberOfFeatures, LanguagesSet *iLanguagesSet, FeaturesSet *iFeaturesSet )
:m_NumberOfLanguages(0),m_LanguagesSet(NULL),m_LanguagesIndices(NULL),m_NumberOfFeatures(0),m_FeaturesSet(NULL),m_FeaturesIndices(NULL),m_Values(NULL)
{
    Init( iNumberOfLanguages, iNumberOfFeatures, iLanguagesSet, iFeaturesSet );
}

FeatureValuesMatrix::~FeatureValuesMatrix()
{
    FreeMemory();
}

void FeatureValuesMatrix::Init( unsigned iNumberOfLanguages, unsigned iNumberOfFeatures, LanguagesSet *iLanguagesSet, FeaturesSet *iFeaturesSet )
{
    AllocMemory( iNumberOfLanguages, iNumberOfFeatures );

    m_LanguagesSet = iLanguagesSet;
    m_FeaturesSet = iFeaturesSet;

    for( unsigned i = 0; i < iNumberOfLanguages; ++i )
    {
        m_LanguagesIndices[i] = i;
    }

    for( unsigned i = 0; i < iNumberOfFeatures; ++i )
    {
        m_FeaturesIndices[i] = i;
    }
}

//Memory management:
void FeatureValuesMatrix::AllocMemory( unsigned iNumberOfLanguages, unsigned iNumberOfFeatures )
{
    //Preconditions:
    assert( iNumberOfLanguages > 0 && iNumberOfFeatures > 0 );

    //Free any previously occupied memory:
    FreeMemory();

    m_NumberOfLanguages = iNumberOfLanguages;
    m_NumberOfFeatures = iNumberOfFeatures;

    m_LanguagesIndices = new unsigned[ m_NumberOfLanguages ];
    m_FeaturesIndices = new unsigned[ iNumberOfFeatures ];
    if( !m_LanguagesIndices || !m_FeaturesIndices )
    {
        cout << "Not enough memory!" << endl;
        assert( false );
        exit( 1 );
    }

    m_Values = new int*[ m_NumberOfLanguages ];
    if( !m_Values )
    {
        cout << "Not enough memory!" << endl;
        assert( false );
        exit( 1 );
    }
    for( unsigned i = 0; i < m_NumberOfLanguages; ++i )
    {
        m_Values[i] = new int[ iNumberOfFeatures ];
        if( !m_Values[i] )
        {
            cout << "Not enough memory!" << endl;
            assert( false );
            exit( 1 );
        }
        for( unsigned j = 0; j < iNumberOfFeatures; ++j )
        {
            m_Values[i][j] = c_MissingData;
        }
    }
}

void FeatureValuesMatrix::FreeMemory( void )
{
    if( m_NumberOfLanguages && m_LanguagesIndices )
    {
        delete[] m_LanguagesIndices;
    }

    if( m_NumberOfFeatures && m_FeaturesIndices )
    {
        delete[] m_FeaturesIndices;
    }

    if( m_NumberOfLanguages && m_NumberOfFeatures && m_Values )
    {
        for( unsigned i = 0; i < m_NumberOfLanguages; ++i )
        {
            if( m_Values[i] )
            {
                delete[] m_Values[i];
            }
        }
        delete[] m_Values;
    }

    m_NumberOfLanguages = m_NumberOfFeatures = 0;
    m_LanguagesIndices = m_FeaturesIndices = NULL;
    m_Values = NULL;
}

void FeatureValuesMatrix::SetValue( unsigned iLanguage, unsigned iFeature, int iValue )
{
    assert( iLanguage < m_NumberOfLanguages && iFeature < m_NumberOfFeatures );

    m_Values[ iLanguage ][ iFeature ] = iValue;
}

int FeatureValuesMatrix::GetValue( unsigned iLanguage, unsigned iFeature )
{
    assert( iLanguage < m_NumberOfLanguages && iFeature < m_NumberOfFeatures );

    return m_Values[ iLanguage ][ iFeature ];
}

//Print it to file:
void FeatureValuesMatrix::Print( FILE *iFile )
{
    unsigned i, j;
    fprintf( iFile, " L\\F" );
    for( j = 0; j < m_NumberOfFeatures; ++j )
    {
        fprintf( iFile, "%3d ", j );
    }
    fprintf( iFile, "\n" );

    for( i = 0; i < m_NumberOfLanguages; ++i )
    {
        fprintf( iFile, "%3d ", i );
        for( j = 0; j < m_NumberOfFeatures; ++j )
        {
            fprintf( iFile, "  %c ", m_Values[i][j] == c_MissingData ? '.' : (char)m_Values[i][j] );
        }
        fprintf( iFile, "\n" );
    }
    fprintf( iFile, "\n" );
}

//Return the language name if found or (-1) otherwise:
int FeatureValuesMatrix::GetLanguageIndexFromName( const char *iName )
{
    for( unsigned i = 0; i < m_NumberOfLanguages; ++i )
    {
        if( strcmp( iName, m_LanguagesSet->GetLanguage( m_LanguagesIndices[i] )->GetName() ) == 0 )
        {
            //Found!
            return i;
        }
    }

    return (-1);
}



