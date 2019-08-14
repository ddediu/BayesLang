#include "LanguageWithFeatures.h"
#include "FeatureValuesMatrix.h"

LanguageWithFeatures::LanguageWithFeatures()
:m_FeatureValuesMatrix(NULL),m_FeaturesSet(NULL),m_FeatureValues(NULL),m_DataType(NULL)
{
}

LanguageWithFeatures::LanguageWithFeatures( FeatureValuesMatrix &iFeatureValuesMatrix, FeaturesSet *iFeaturesSet )
:m_FeatureValuesMatrix(&iFeatureValuesMatrix),m_FeaturesSet(NULL),m_FeatureValues(NULL),m_DataType(NULL)
{
    SetFeaturesSet( iFeaturesSet );
}

LanguageWithFeatures::LanguageWithFeatures( Language &iLanguage, FeatureValuesMatrix &iFeatureValuesMatrix, FeaturesSet *iFeaturesSet )
:Language(iLanguage),m_FeatureValuesMatrix(&iFeatureValuesMatrix),m_FeaturesSet(NULL),m_FeatureValues(NULL),m_DataType(NULL)
{
    SetFeaturesSet( iFeaturesSet );
}

LanguageWithFeatures::LanguageWithFeatures( LanguageWithFeatures &iLanguageWithFeatures )
:Language(iLanguageWithFeatures),m_FeatureValuesMatrix(iLanguageWithFeatures.m_FeatureValuesMatrix),
 m_FeaturesSet(iLanguageWithFeatures.m_FeaturesSet),m_FeatureValues(NULL),m_DataType(NULL)
{
    m_FeatureValues = new int[ m_FeaturesSet->GetNFeatures() ];
    if( !m_FeatureValues )
    {
        cerr << "Not enough memory!" << endl;
        assert( false );
        exit( 1 );
    }
    m_DataType = new eValueDataType[ m_FeaturesSet->GetNFeatures() ];
    if( !m_DataType )
    {
        cerr << "Not enough memory!" << endl;
        assert( false );
        exit( 1 );
    }

    for( unsigned i = 0; i < m_FeaturesSet->GetNFeatures(); ++i )
    {
        m_FeatureValues[i] = iLanguageWithFeatures.m_FeatureValues[i];
        m_DataType[i] = iLanguageWithFeatures.m_DataType[i];
    }
}

LanguageWithFeatures::~LanguageWithFeatures()
{
    if( m_FeatureValues )
    {
        delete[] m_FeatureValues;
    }
    if( m_DataType )
    {
        delete[] m_DataType;
    }
}

void LanguageWithFeatures::Init( const char *iName )
{
    //Init the language name:
    Language::Init( iName );

    //Get the feature values for this language:
    assert( m_FeatureValuesMatrix );
    unsigned i;

    int lgIndex = m_FeatureValuesMatrix->GetLanguageIndexFromName( iName );
    if( lgIndex == (-1) )
    {
        //Language not found!
        for( i = 0; i < m_FeaturesSet->GetNFeatures(); ++i )
        {
            m_FeatureValues[i] = FeatureValuesMatrix::c_MissingData;
            m_DataType[i] = VALUE_ANCESTRAL; //Supposedly, this is an ancestor language
        }
    }
    else
    {
        //Copy the values from the matrix:
        for( i = 0; i < m_FeaturesSet->GetNFeatures(); ++i )
        {
            m_FeatureValues[i] = m_FeatureValuesMatrix->GetValue( lgIndex, i );
            m_DataType[i] = (m_FeatureValues[i] == FeatureValuesMatrix::c_MissingData ? VALUE_MISSING : VALUE_DEFINED);
        }
    }
}

//Set the features set:
void LanguageWithFeatures::SetFeaturesSet( FeaturesSet *iFeaturesSet )
{
    assert( iFeaturesSet );

    if( m_FeatureValues )
    {
        delete[] m_FeatureValues;
    }
    if( m_DataType )
    {
        delete[] m_DataType;
    }

    m_FeaturesSet = iFeaturesSet;
    m_FeatureValues = new int[ m_FeaturesSet->GetNFeatures() ];
    if( !m_FeatureValues )
    {
        cerr << "Not enough memory!" << endl;
        assert( false );
        exit( 1 );
    }
    m_DataType = new eValueDataType[ m_FeaturesSet->GetNFeatures() ];
    if( !m_DataType )
    {
        cerr << "Not enough memory!" << endl;
        assert( false );
        exit( 1 );
    }

    for( unsigned i = 0; i < m_FeaturesSet->GetNFeatures(); ++i )
    {
        m_FeatureValues[i] = FeatureValuesMatrix::c_MissingData;
        m_DataType[i] = VALUE_MISSING;
    }
}

//Copy operator:
LanguageWithFeatures &LanguageWithFeatures::operator=( Language &iLanguage )
{
    Init( iLanguage.GetName() );
    Language::operator=( iLanguage );
    return *this;
}

LanguageWithFeatures &LanguageWithFeatures::operator=( LanguageWithFeatures &iLanguage )
{
    Language::operator=( iLanguage );
    m_FeaturesSet = iLanguage.m_FeaturesSet;

    if( m_FeatureValues )
    {
        delete[] m_FeatureValues;
    }
    if( m_DataType )
    {
        delete[] m_DataType;
    }
    m_FeatureValues = new int[ m_FeaturesSet->GetNFeatures() ];
    if( !m_FeatureValues )
    {
        cerr << "Not enough memory!" << endl;
        assert( false );
        exit( 1 );
    }
    m_DataType = new eValueDataType[ m_FeaturesSet->GetNFeatures() ];
    if( !m_DataType )
    {
        cerr << "Not enough memory!" << endl;
        assert( false );
        exit( 1 );
    }
    for( unsigned i = 0; i < m_FeaturesSet->GetNFeatures(); ++i )
    {
        m_FeatureValues[i] = iLanguage.m_FeatureValues[i];
        m_DataType[i] = iLanguage.m_DataType[i];
    }
    return *this;
}

void LanguageWithFeatures::Print( FILE *iFile, int iWhichFeature, bool iPrettyPrint )
{
    assert( iWhichFeature == (-1) || iWhichFeature < (int)m_FeaturesSet->GetNFeatures() );

    Language::Print( iFile );

    if( !m_FeaturesSet || !m_FeatureValues )
    {
        if( iPrettyPrint )
        {
            fprintf( iFile, "  " );
        }
        fprintf( iFile, "||" );
        return;
    }

    if( iPrettyPrint )
    {
        fprintf( iFile, "  " );
    }
    if( iWhichFeature == (-1) )
    {
        for( unsigned i = 0; i < m_FeaturesSet->GetNFeatures(); ++i )
        {
            fprintf( iFile, "|%c%c", (m_FeatureValues[i] == FeatureValuesMatrix::c_MissingData) ? '.' : (char)m_FeatureValues[i], PrintDataType(m_DataType[i]) );
        }
    }
    else
    {
        fprintf( iFile, "|%c%c", (m_FeatureValues[iWhichFeature] == FeatureValuesMatrix::c_MissingData) ? '.' : (char)m_FeatureValues[iWhichFeature], PrintDataType(m_DataType[iWhichFeature]) );
    }
    fprintf( iFile, "|" );
}

unsigned LanguageWithFeatures::Print( char *iBuffer, int iWhichFeature, bool iPrettyPrint )
{
    char *curChar = iBuffer;

    assert( iWhichFeature == (-1) || iWhichFeature < (int)m_FeaturesSet->GetNFeatures() );

    curChar += Language::Print( curChar );

    if( !m_FeaturesSet || !m_FeatureValues )
    {
        if( iPrettyPrint )
        {
            curChar += sprintf( curChar, "  " );
        }
        curChar += sprintf( curChar, "||" );
        return curChar - iBuffer;
    }

    if( iPrettyPrint )
    {
        curChar += sprintf( curChar, "  " );
    }
    if( iWhichFeature == (-1) )
    {
        for( unsigned i = 0; i < m_FeaturesSet->GetNFeatures(); ++i )
        {
            curChar += sprintf( curChar, "|%c%c", (m_FeatureValues[i] == FeatureValuesMatrix::c_MissingData) ? '.' : (char)m_FeatureValues[i], PrintDataType(m_DataType[i]) );
        }
    }
    else
    {
        curChar += sprintf( curChar, "|%c%c", (m_FeatureValues[iWhichFeature] == FeatureValuesMatrix::c_MissingData) ? '.' : (char)m_FeatureValues[iWhichFeature], PrintDataType(m_DataType[iWhichFeature]) );
    }
    curChar += sprintf( curChar, "|" );

    return curChar - iBuffer;
}

//Print the data type:
char LanguageWithFeatures::PrintDataType( eValueDataType iDataType, bool iAsLetter )
{
    switch( iDataType )
    {
    case VALUE_DEFINED:
        return iAsLetter ? 'd' : '!';
        break;
    case VALUE_MISSING:
        return iAsLetter ? 'm' : '.';
        break;
    case VALUE_ANCESTRAL:
        return iAsLetter ? 'a' : '?';
        break;
    default:
        cerr << "Unknwon data type!" << endl;
        assert( false );
        exit( 1 );
        break;
    }

    return 0;
}

//Test if the required feature(s) have undefined value(s) [iWhichFeature = (-1) == all features]:
bool LanguageWithFeatures::UndefinedFeatures( int iWhichFeature )
{
    //Preconditions:
    assert( iWhichFeature == (-1) || (iWhichFeature >= 0 && iWhichFeature < (int)m_FeaturesSet->GetNFeatures()) );

    if( iWhichFeature != (-1) )
    {
        return (m_FeatureValues[ iWhichFeature ] == FeatureValuesMatrix::c_MissingData);
    }
    else
    {
        for( unsigned i = 0; i < m_FeaturesSet->GetNFeatures(); ++i )
        {
            if( m_FeatureValues[i] == FeatureValuesMatrix::c_MissingData )
            {
                return true;
            }
        }
        return false;
    }

    //How did I get here?
    assert( false );
    return false;
}

//Generate values for the features with missing values accroding to their description:
void LanguageWithFeatures::GenerateMissingValues( FeatureRateMatrix *iRateMatrix, int iWhichFeature )
{
    if( m_DataType[iWhichFeature] == VALUE_DEFINED )
    {
        //This value is fixed and cannot be changed:
        return;
    }
    else
    {
        //Generate a new feature values coming from its prior distribution (assumed uniform):
        m_FeatureValues[iWhichFeature] = m_FeaturesSet->GetFeature(iWhichFeature)->GetMinVal() + rand() %  m_FeaturesSet->GetFeature(iWhichFeature)->GetNVals();
    }
}

//Generate a new candidate value for a given real feature (return the Hastings ratio or (-1)):
DblType LanguageWithFeatures::NewCandidate( int iWhichFeature )
{
    assert( iWhichFeature >= 0 && iWhichFeature < (int)GetNFeatures() && m_DataType[iWhichFeature] != VALUE_DEFINED );

    int newValue;

    unsigned i;
    int destinationIdices[MAX_NUMBER_OF_FEATURE_VALUES];
    int howManyDestinations = 0;
    SquareMatrix &adjacencyMatrix = m_FeaturesSet->GetFeature(iWhichFeature)->GetCustomTransitionsMatrix();
    int currentValue;

    switch( m_FeaturesSet->GetFeature( iWhichFeature )->GetType() )
    {
    case Feature::FEATURE_ORDERED:
    case Feature::FEATURE_UNORDERED:
        //Any value would do:
        if( m_FeaturesSet->GetFeature( iWhichFeature )->GetNVals() <= 1 )
        {
            cerr << "Features must have at least two values!" << endl;
            assert( false );
            exit( 1 );
        }
        else if( m_FeaturesSet->GetFeature( iWhichFeature )->GetNVals() == 2 )
        {
            if( m_FeatureValues[iWhichFeature] == (int)m_FeaturesSet->GetFeature( iWhichFeature )->GetMinVal() )
            {
                m_FeatureValues[iWhichFeature] = m_FeaturesSet->GetFeature( iWhichFeature )->GetMaxVal();
            }
            else
            {
                m_FeatureValues[iWhichFeature] = m_FeaturesSet->GetFeature( iWhichFeature )->GetMinVal();
            }
        }
        else
        {
            do
            {
                newValue = RandomInt( m_FeaturesSet->GetFeature( iWhichFeature )->GetMinVal(), m_FeaturesSet->GetFeature( iWhichFeature )->GetMaxVal() );
            }
            while( m_FeatureValues[iWhichFeature] == newValue );
            m_FeatureValues[iWhichFeature] = newValue;
        }

        //The proposal distribution is symmetric, so the Hastings ratio is 1.0:
        return 1.0;
        break;
    case Feature::FEATURE_CIRCULAR:
        //Pick the next or the previous value:
        if( RandomProbability() <= 0.5 )
        {
            //Previous:
            m_FeatureValues[iWhichFeature] = (m_FeatureValues[iWhichFeature] == (int)m_FeaturesSet->GetFeature( iWhichFeature )->GetMinVal()) ? m_FeaturesSet->GetFeature( iWhichFeature )->GetMaxVal() : m_FeatureValues[iWhichFeature]-1;
        }
        else
        {
            //Next:
            m_FeatureValues[iWhichFeature] = (m_FeatureValues[iWhichFeature] == (int)m_FeaturesSet->GetFeature( iWhichFeature )->GetMaxVal()) ? m_FeaturesSet->GetFeature( iWhichFeature )->GetMinVal() : m_FeatureValues[iWhichFeature]+1;
        }
        //The proposal distribution is symmetric:
        return 1.0;
        break;
    case Feature::FEATURE_RANKED:
        if( m_FeatureValues[iWhichFeature] == (int)m_FeaturesSet->GetFeature( iWhichFeature )->GetMinVal() )
        {
            m_FeatureValues[iWhichFeature] += RandomProbability() <= 0.5 ? 0 : (+1);
        }
        else if( m_FeatureValues[iWhichFeature] == (int)m_FeaturesSet->GetFeature( iWhichFeature )->GetMaxVal() )
        {
            m_FeatureValues[iWhichFeature] += RandomProbability() <= 0.5 ? (-1) : 0;
        }
        else
        {
            m_FeatureValues[iWhichFeature] += RandomProbability() <= 0.5 ? (-1) : (+1);
        }
        //The proposal distribution is symmetric:
        return 1.0;
        break;
    /*case Feature::FEATURE_ORDERED:
        //Generate the next value in the chain:
        if( m_FeatureValues[iWhichFeature] < m_FeaturesSet->GetFeature( iWhichFeature )->GetMaxVal() )
        {
            ++m_FeatureValues[iWhichFeature];
        }
        else
        {
            m_FeatureValues[iWhichFeature] = m_FeaturesSet->GetFeature( iWhichFeature )->GetMinVal();
        }
        break;*/
    case Feature::FEATURE_CUSTOM:
        //Use the adjancency matrix to generate a new candidate:
        currentValue = m_FeatureValues[iWhichFeature] - m_FeaturesSet->GetFeature( iWhichFeature )->GetMinVal();
        //See where you can go from here:
        for( i = 0; i < adjacencyMatrix.GetSize(); ++i )
        {
            if( (int)i != currentValue && adjacencyMatrix.GetCell( currentValue, i ) == 1.0 )
            {
                destinationIdices[howManyDestinations++] = i;
            }
        }
        assert( howManyDestinations > 0 );
        m_FeatureValues[iWhichFeature] = destinationIdices[ RandomInt( 0, howManyDestinations-1 ) ] + m_FeaturesSet->GetFeature( iWhichFeature )->GetMinVal();
        return 1.0;
        break;
    default:
        //Nothing to be done:
        break;
    }

    //How did I get here?
    assert( false );
    return (-1.0);
}

//Copy the volatile content (feature values):
void LanguageWithFeatures::CopyContent( LanguageWithFeatures &iLanguage )
{
    for( unsigned i = 0; i < m_FeaturesSet->GetNFeatures(); ++i )
    {
        if( m_DataType[i] != VALUE_DEFINED )
        {
            m_FeatureValues[i] = iLanguage.m_FeatureValues[i];
        }
    }
}

//Get the log of the prior probability of this language (for ratio computation, i.e. without any multiplicative constants):
DblType LanguageWithFeatures::GetLogPriorProbability( int iWhichFeature )
{
    //Fixed features have a prior of 1.0 (logprior=0) while the others are uniform:
    DblType logprior = 0.0;

    if( iWhichFeature == (-1) )
    {
        for( unsigned i = 0; i < m_FeaturesSet->GetNFeatures(); ++i )
        {
            if( m_DataType[i] == VALUE_DEFINED )
            {
                //logprior += 0.0;
            }
            else
            {
                logprior -= m_FeaturesSet->GetFeature(i)->GetLogNVals();
            }
        }
        return logprior;
    }
    else
    {
        return (m_DataType[iWhichFeature] == VALUE_DEFINED) ? 0.0 : -m_FeaturesSet->GetFeature(iWhichFeature)->GetLogNVals();
    }
}

void LanguageWithFeatures::WriteParamsHeader( FILE *iFile )
{
    for( unsigned i = 0; i < m_FeaturesSet->GetNFeatures(); ++i )
    {
        //if( m_DataType[i] != VALUE_DEFINED )
        //{
            fprintf( iFile, "%s.%s.%c\t", m_Name, m_FeaturesSet->GetFeature(i)->GetName(), PrintDataType( m_DataType[i], true ) );
        //}
    }
}

void LanguageWithFeatures::WriteParamsValues( FILE *iFile )
{
    for( unsigned i = 0; i < m_FeaturesSet->GetNFeatures(); ++i )
    {
        //if( m_DataType[i] != VALUE_DEFINED )
        //{
            fprintf( iFile, "%c\t", m_FeatureValues[i] /*== FeatureValuesMatrix::c_MissingData ? '.' : m_FeatureValues[i]*/ );
        //}
    }
}

unsigned LanguageWithFeatures::WriteParamsValues( char *iBuffer )
{
    char *curChar = iBuffer;

    for( unsigned i = 0; i < m_FeaturesSet->GetNFeatures(); ++i )
    {
        //if( m_DataType[i] != VALUE_DEFINED )
        //{
            curChar += sprintf( curChar, "%c\t", m_FeatureValues[i] /*== FeatureValuesMatrix::c_MissingData ? '.' : m_FeatureValues[i]*/ );
        //}
    }

    return curChar - iBuffer;
}


