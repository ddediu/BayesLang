#include "Feature.h"

Feature::Feature( void )
:m_Name(NULL),m_Type(FEATURE_UNORDERED),m_MinVal('0'),m_MaxVal('0'),m_NVals(0),m_LogNVals(0.0),m_NumberOfLabels(0),m_Labels(NULL)
{
}

Feature::Feature( const char *iName, eFeatureTypes iType, unsigned iMinVal, unsigned iMaxVal, vector<string> &iValueLables )
:m_Name(NULL),m_Type(FEATURE_UNORDERED),m_MinVal('0'),m_MaxVal('0'),m_NVals(0),m_LogNVals(0.0),m_NumberOfLabels(0),m_Labels(NULL)
{
    Init( iName, iType, iMinVal, iMaxVal, iValueLables );
}

Feature::~Feature()
{
    if( m_Name )
    {
        delete[] m_Name;
    }
    if( m_NumberOfLabels && m_Labels )
    {
        for( unsigned i = 0; i < m_NumberOfLabels; ++i )
        {
            delete[] m_Labels[i];
        }
        delete[] m_Labels;
    }
}

void Feature::Init( const char *iName, eFeatureTypes iType, unsigned iMinVal, unsigned iMaxVal, vector<string> &iValueLables )
{
    unsigned i;

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

    m_Type = iType;


    m_MinVal = iMinVal;
    m_MaxVal = iMaxVal;

    m_NVals = m_MaxVal - m_MinVal + 1;
    m_LogNVals = log( m_NVals );

    if( m_NVals > MAX_NUMBER_OF_FEATURE_VALUES )
    {
        cerr << "There can be at most " << MAX_NUMBER_OF_FEATURE_VALUES << " feature values, but feature \"" << iName << "\" has " << m_NVals << "!" << endl;
        assert( false );
        exit( 1 );
    }

    if( m_NumberOfLabels > 0 && m_NumberOfLabels != m_NVals )
    {
        cerr << "The number of value lables must be equal to the actual number of values for feature \"" << GetName() << "\"!" << endl;
        assert( false );
        exit( 1 );
    }

    //Copy the value labels (if any)
    if( m_NumberOfLabels && m_Labels )
    {
        for( i = 0; i < m_NumberOfLabels; ++i )
        {
            delete[] m_Labels[i];
        }
        delete[] m_Labels;
    }
    m_NumberOfLabels = 0;
    m_Labels = NULL;

    if( (m_NumberOfLabels = iValueLables.size()) > 0 )
    {
        m_Labels = new char*[ m_NumberOfLabels ];
        if( !m_Labels )
        {
            cerr << "Not enough memory!" << endl;
            assert( false );
            exit( 1 );
        }

        for( i = 0; i < m_NumberOfLabels; ++i )
        {
            m_Labels[i] = new char[ strlen(iValueLables[i].c_str())+1 ];
            if( !m_Labels[i] )
            {
                cerr << "Not enough memory!" << endl;
                assert( false );
                exit( 1 );
            }
            strcpy( m_Labels[i], iValueLables[i].c_str() );
        }
    }
}

//Print it to file:
void Feature::Print( FILE *iFile )
{
    fprintf( iFile, "%s[%s,%c-%c]", m_Name, m_Type==FEATURE_UNORDERED ? "U" : m_Type==FEATURE_CIRCULAR ? "C" : m_Type==FEATURE_RANKED ? "R" : m_Type==FEATURE_CUSTOM ? "K" : "O", (char)m_MinVal, (char)m_MaxVal );
}

//Print it to file:
unsigned Feature::Print( char *iBuffer )
{
    char *curChar = iBuffer;

    curChar += sprintf( curChar, "%s[%s,%c-%c]", m_Name, m_Type==FEATURE_UNORDERED ? "U" : m_Type==FEATURE_CIRCULAR ? "C" : m_Type==FEATURE_RANKED ? "R" : m_Type==FEATURE_CUSTOM ? "K" : "O", (char)m_MinVal, (char)m_MaxVal );

    return curChar - iBuffer;
}

void Feature::SetCustomTransitionsMatrix( SquareMatrix &iMatrix )
{
    //Check the feature type
    if( m_Type != FEATURE_CUSTOM )
    {
        cerr << "Can set a custom transition matrix only for custom features, but feature \"" << m_Name << "\" is not!" << endl;
        assert( false );
        exit( 1 );
    }

    if( iMatrix.GetSize() != m_NVals )
    {
        cerr << "The declared number of values (" << m_NVals << ") for feature \"" << GetName() << "\" is different from the custom transition matrix size (" << iMatrix.GetSize() << ")!" << endl;
        assert( false );
        exit( 1 );
    }

    //Check the matrix itself: any value must be reachable from any other value:
    assert( iMatrix.IsBoolean() );

    //Copy it!
    m_CustomTransitionsMatrix = iMatrix;

    ShortestPath_FloydWarshall( m_CustomTransitionsMatrix, m_CustomShortestPathsMatrix );

    //Check for unreachable values:
    bool foundUnreachableValues= false;
    for( unsigned i = 0; i < m_CustomShortestPathsMatrix.GetSize(); ++i )
    {
        for( unsigned j = 0; j < m_CustomShortestPathsMatrix.GetSize(); ++j )
        {
            if(m_CustomShortestPathsMatrix.GetCell(i,j) == 0 )
            {
                if( foundUnreachableValues == false )
                {
                    foundUnreachableValues = true;
                    cerr << "Any feature value must be reached from any other feature value, but for feature \"" << GetName() << "\", the following are not reachable: ";
                }
                cerr << i+1 << " from " << j+1 << ", ";
            }
        }
    }
    if( foundUnreachableValues == true )
    {
        cerr << endl;
        assert( false );
        exit( 1 );
    }
}



