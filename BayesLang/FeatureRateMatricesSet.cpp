#include "FeatureRateMatricesSet.h"

FeatureRateMatricesSet::FeatureRateMatricesSet()
:m_FeaturesSet(NULL),m_NumberOfMatrices(0),m_RateMatrices(NULL)
{
}

FeatureRateMatricesSet::FeatureRateMatricesSet( FeaturesSet &iFeaturesSet )
:m_FeaturesSet(NULL),m_NumberOfMatrices(0),m_RateMatrices(NULL)
{
    Init( iFeaturesSet );
}

FeatureRateMatricesSet::~FeatureRateMatricesSet()
{
    FreeMemory();
}

//Memory management:
void FeatureRateMatricesSet::AllocMemory( unsigned iNumberOfFeatures )
{
    assert( iNumberOfFeatures );

    //Free already allocated memory:
    FreeMemory();

    m_NumberOfMatrices = iNumberOfFeatures;
    m_RateMatrices = new FeatureRateMatrix[ m_NumberOfMatrices ];
    if( !m_RateMatrices )
    {
        cerr << "Not enough memory!" << endl;
        assert( false );
        exit( 1 );
    }
}

void FeatureRateMatricesSet::FreeMemory( void )
{
    if( m_NumberOfMatrices && m_RateMatrices )
    {
        delete[] m_RateMatrices;
    }
    m_NumberOfMatrices = 0;
    m_RateMatrices = NULL;
}

void FeatureRateMatricesSet::Init( FeaturesSet &iFeaturesSet )
{
    //Alloc memory:
    AllocMemory( iFeaturesSet.GetNFeatures() );

    m_FeaturesSet = &iFeaturesSet;

    //Init the rate matrices:
    for( unsigned i = 0; i < m_NumberOfMatrices; ++i )
    {
        m_RateMatrices[i].SetFeature( m_FeaturesSet->GetFeature(i) );
    }
}

void FeatureRateMatricesSet::Print( FILE *iFile, int iWhichFeature )
{
    fprintf( iFile, "Intantaneous rate %s for %s:\n", iWhichFeature == (-1) ? "matrices" : "matrix",
                                                      iWhichFeature == (-1) ? "all features" : m_FeaturesSet->GetFeature(iWhichFeature)->GetName() );

    if( iWhichFeature == (-1) )
    {
        for( unsigned i = 0; i < m_FeaturesSet->GetNFeatures(); ++i )
        {
            m_RateMatrices[i].Print( iFile );
        }
    }
    else
    {
        m_RateMatrices[ iWhichFeature ].Print( iFile );
    }
}

unsigned FeatureRateMatricesSet::Print( char *iBuffer, int iWhichFeature )
{
    char *curChar = iBuffer;

    curChar += sprintf( curChar, "Intantaneous rate %s for %s:\n", iWhichFeature == (-1) ? "matrices" : "matrix",
                                                                   iWhichFeature == (-1) ? "all features" : m_FeaturesSet->GetFeature(iWhichFeature)->GetName() );

    if( iWhichFeature == (-1) )
    {
        for( unsigned i = 0; i < m_FeaturesSet->GetNFeatures(); ++i )
        {
            curChar += m_RateMatrices[i].Print( curChar );
        }
    }
    else
    {
        curChar += m_RateMatrices[ iWhichFeature ].Print( curChar );
    }

    return curChar - iBuffer;
}

void FeatureRateMatricesSet::WriteParamsHeader( FILE *iFile )
{
    for( unsigned i = 0; i < m_NumberOfMatrices; ++i )
    {
        m_RateMatrices[i].WriteParamsHeader( iFile );
    }
}

void FeatureRateMatricesSet::WriteParamsValues( FILE *iFile )
{
    for( unsigned i = 0; i < m_NumberOfMatrices; ++i )
    {
        m_RateMatrices[i].WriteParamsValues( iFile );
    }
}

unsigned FeatureRateMatricesSet::WriteParamsValues( char *iBuffer )
{
    char *curChar = iBuffer;

    for( unsigned i = 0; i < m_NumberOfMatrices; ++i )
    {
        curChar += m_RateMatrices[i].WriteParamsValues( curChar );
    }

    return curChar - iBuffer;
}

//Generate a new rates matrix (return the Hastings ratio or -1):
DblType FeatureRateMatricesSet::NewCandidate( int iWhichFeature )
{
    assert( iWhichFeature >= 0 && iWhichFeature < (int)m_NumberOfMatrices );
    return m_RateMatrices[iWhichFeature].NewCandidate();
}

//Copy operator:
FeatureRateMatricesSet &FeatureRateMatricesSet::operator=( const FeatureRateMatricesSet &iFeatureRateMatricesSet )
{
    FreeMemory();

    m_FeaturesSet = iFeatureRateMatricesSet.m_FeaturesSet;

    m_NumberOfMatrices = iFeatureRateMatricesSet.m_NumberOfMatrices;
    m_RateMatrices = new FeatureRateMatrix[m_NumberOfMatrices];
    if( !m_RateMatrices )
    {
        cerr << "Not enough memory!" << endl;
        assert( false );
        exit( 1 );
    }

    for( unsigned i = 0; i < m_NumberOfMatrices; ++i )
    {
        m_RateMatrices[i] = iFeatureRateMatricesSet.m_RateMatrices[i];
    }

    return *this;
}

void FeatureRateMatricesSet::CopyContent( const FeatureRateMatricesSet &iMatrices )
{
    assert( m_NumberOfMatrices == iMatrices.m_NumberOfMatrices );

    for( unsigned i = 0; i < m_NumberOfMatrices; ++i )
    {
        m_RateMatrices[i].CopyContent( iMatrices.m_RateMatrices[i] );
    }
}



