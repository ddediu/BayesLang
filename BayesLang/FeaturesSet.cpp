#include "FeaturesSet.h"

FeaturesSet::FeaturesSet( void )
:m_NumberOfFeatures(0),m_Features(NULL)
{
}

FeaturesSet::FeaturesSet( unsigned iNumberOfFeatures )
:m_NumberOfFeatures(0),m_Features(NULL)
{
    Init( iNumberOfFeatures );
}

FeaturesSet::~FeaturesSet()
{
    if( m_NumberOfFeatures && m_Features )
    {
        delete[] m_Features;
    }
}

void FeaturesSet::Init( unsigned iNumberOfFeatures )
{
    //Preconditions:
    assert( iNumberOfFeatures > 0 );

    if( m_NumberOfFeatures && m_Features )
    {
        delete[] m_Features;
    }

    m_NumberOfFeatures = iNumberOfFeatures;
    m_Features = new Feature[ m_NumberOfFeatures ];
    if( !m_Features )
    {
        cerr << "Not enough memory!" << endl;
        assert( false );
        exit( 1 );
    }
}

//Print it to file:
void FeaturesSet::Print( FILE *iFile )
{
    fprintf( iFile, "There are %d features: ", m_NumberOfFeatures );
    for( unsigned i = 0; i < m_NumberOfFeatures; ++i )
    {
        m_Features[i].Print( iFile );
        if(i < m_NumberOfFeatures-1)
        {
            fprintf( iFile, "," );
        }
    }
    fprintf( iFile, "\n" );
}


