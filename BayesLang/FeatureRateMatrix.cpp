#include "FeatureRateMatrix.h"

//Generating new candidate rates: the sliding window's width:
DblType FeatureRateMatrix::st_RateSlidingWindow = gl_RateProposalSlidingWindowWidth;
//The boundaries:
DblType FeatureRateMatrix::st_LowerBoundary = 0.0;
DblType FeatureRateMatrix::st_UpperBoundary = 2.0;

FeatureRateMatrix::FeatureRateMatrix()
:m_Feature(NULL),m_Model(SINGLE_RATE_MODEL),m_Alpha(0.0),m_HasBeenAltered(false)
{
}

FeatureRateMatrix::~FeatureRateMatrix()
{
}

void FeatureRateMatrix::Print( FILE *iFile )
{
    fprintf( iFile, "The instantaneous rate matrix for feature " );
    m_Feature->Print( iFile );
    fprintf( iFile, " is:\n" );
    m_RateMatrix.Print( iFile );
}

unsigned FeatureRateMatrix::Print( char *iBuffer )
{
    char *curChar = iBuffer;

    curChar += sprintf( curChar, "The instantaneous rate matrix for feature " );
    curChar+= m_Feature->Print( curChar );
    curChar += sprintf( curChar, " is:\n" );
    curChar += m_RateMatrix.Print( curChar );

    return curChar - iBuffer;
}

void FeatureRateMatrix::WriteParamsHeader( FILE *iFile )
{
    assert( m_Model == SINGLE_RATE_MODEL );

    //Export alpha:
    fprintf( iFile, "%s.%s.%d.Alpha\t", m_Feature->GetName(), m_Feature->GetType() == Feature::FEATURE_UNORDERED ? "U" : m_Feature->GetType() == Feature::FEATURE_ORDERED ? "O" : "R", m_Feature->GetNVals() );
}

void FeatureRateMatrix::WriteParamsValues( FILE *iFile )
{
    assert( m_Model == SINGLE_RATE_MODEL );

    //Export alpha and beta:
    fprintf( iFile, "%f\t", m_Alpha );
}

unsigned FeatureRateMatrix::WriteParamsValues( char *iBuffer )
{
    assert( m_Model == SINGLE_RATE_MODEL );

    char *curChar = iBuffer;

    //Export alpha and beta:
    curChar += sprintf( curChar, "%f\t", m_Alpha );

    return curChar - iBuffer;
}

void FeatureRateMatrix::SetFeature( Feature *iFeature )
{
    assert( iFeature );

    m_Feature = iFeature;
    m_RateMatrix.Init( m_Feature->GetNVals() );

    //Set the rate matrix as required: initially all features have the same rate:
    ComputeRateMatrixTemplateForSingleRateModel( 1.0 );
}

//Compute the likelihood for SINGLE_RATE_MODEL of given change for this feature and a desired alpha:
void FeatureRateMatrix::ComputeRateMatrixTemplateForSingleRateModel( DblType iAlpha )
{
    //Compute the unique rate alpha function of the feature:
    if( m_Model != SINGLE_RATE_MODEL )
    {
        cerr << "This can be applied only to SINGLE_RATE_MODEL!" << endl;
        assert( false );
        exit( 1 );
    }

    unsigned i, j;
    switch( m_Feature->GetType() )
    {
    case Feature::FEATURE_UNORDERED:
        //Compute the rates:
        m_Alpha = iAlpha;
        m_Beta = -m_Alpha * (m_Feature->GetNVals()-1);

        //Fill in the matrix:
        for( i = 0; i < m_RateMatrix.GetSize(); ++i )
        {
            for( j = 0; j < m_RateMatrix.GetSize(); ++j )
            {
                if( i == j )
                {
                    m_RateMatrix.SetCell( i, j, m_Beta );
                }
                else
                {
                    m_RateMatrix.SetCell( i, j, m_Alpha );
                }
            }
        }
        break;
    case Feature::FEATURE_ORDERED:
        //Compute the rates:
        m_Alpha = iAlpha;
        m_Beta = -m_Alpha;

        //Fill in the matrix:
        for( i = 0; i < m_RateMatrix.GetSize(); ++i )
        {
            for( j = 0; j < m_RateMatrix.GetSize(); ++j )
            {
                if( i == j )
                {
                    m_RateMatrix.SetCell( i, j, m_Beta );
                }
                else if( j == i+1 || (i == m_RateMatrix.GetSize()-1 && j == 0) )
                {
                    m_RateMatrix.SetCell( i, j, m_Alpha );
                }
                else
                {
                    m_RateMatrix.SetCell( i, j, 0.0 );
                }
            }
        }
        break;
    case Feature::FEATURE_CIRCULAR:
        //Compute the rates:
        m_Alpha = iAlpha;
        m_Beta = -2*m_Alpha;

        //Fill in the matrix:
        for( i = 0; i < m_RateMatrix.GetSize(); ++i )
        {
            for( j = 0; j < m_RateMatrix.GetSize(); ++j )
            {
                if( i == j )
                {
                    m_RateMatrix.SetCell( i, j, m_Beta );
                }
                else if( (i == 0 && (j == 1 || j == m_RateMatrix.GetSize()-1 )) ||
                         (i == m_RateMatrix.GetSize()-1 && (j == 0 || j == m_RateMatrix.GetSize()-2)) ||
                         (j == i-1 || j == i+1) )
                {
                    m_RateMatrix.SetCell( i, j, m_Alpha );
                }
                else
                {
                    m_RateMatrix.SetCell( i, j, 0.0 );
                }
            }
        }
        break;
    case Feature::FEATURE_RANKED:
        //Compute the rates:
        m_Alpha = iAlpha;
        m_Beta = -2*m_Alpha;

        //Fill in the matrix:
        for( i = 0; i < m_RateMatrix.GetSize(); ++i )
        {
            for( j = 0; j < m_RateMatrix.GetSize(); ++j )
            {
                if( i == j )
                {
                    m_RateMatrix.SetCell( i, j, (i == 0 || i == m_RateMatrix.GetSize()-1) ? -m_Alpha : m_Beta );
                }
                else if( j == i-1 || j == i+1 )
                {
                    m_RateMatrix.SetCell( i, j, m_Alpha );
                }
                else
                {
                    m_RateMatrix.SetCell( i, j, 0.0 );
                }
            }
        }
        break;
    case Feature::FEATURE_CUSTOM:
        //Compute the rates:
        m_Alpha = iAlpha;
        m_Beta = -m_Alpha;

        //Fill in the matrix (each row must add up to 0):
        for( i = 0; i < m_RateMatrix.GetSize(); ++i )
        {
            int sum = 0;
            for( j = 0; j < m_RateMatrix.GetSize(); ++j )
            {
                if( i != j )
                {
                    if( m_Feature->GetCustomTransitionsMatrix().GetCell( i, j ) == 0.0 )
                    {
                        m_RateMatrix.SetCell( i, j, 0.0 );
                    }
                    else if( m_Feature->GetCustomTransitionsMatrix().GetCell( i, j ) == 1.0 )
                    {
                        ++sum;
                        m_RateMatrix.SetCell( i, j, m_Alpha );
                    }
                    else
                    {
                        cerr << "Illegal value in transition matrix!" << endl;
                        assert( false );
                        exit( 1 );
                    }
                }
            }
            m_RateMatrix.SetCell( i, i, -sum*m_Alpha );
        }
        break;
    default:
        cerr << "Unknown feature type!" << endl;
        assert( false );
        break;
    }

    //cout << "The rate matrix:\n";
    //m_RateMatrix.Print( stdout );
}

void FeatureRateMatrix::SetRateMatrix( SquareMatrix &iRateMatrix )
{
    assert( m_Feature && m_Feature->GetNVals() == iRateMatrix.GetSize() );

    m_RateMatrix = iRateMatrix;
}

//Compute the loglikelihood of given change for this feature:
DblType FeatureRateMatrix::ComputeLogLikelihood( int iAncestorValue, int iDescendantValue, DblType iBranchLength )
{
    assert( m_Feature &&
            iBranchLength >= 0 &&
            iAncestorValue >= (int)m_Feature->GetMinVal() && iAncestorValue <= (int)m_Feature->GetMaxVal() &&
            iDescendantValue >= (int)m_Feature->GetMinVal() && iDescendantValue <= (int)m_Feature->GetMaxVal() );

    //Exponentiate the instantaneous rate matrix accordingly:
    SquareMatrix m = m_RateMatrix;
    SquareMatrix r( m_RateMatrix.GetSize() );
    Exponential( m, iBranchLength, r );

    //Get the probability p( iDescendantValue | iAncestorValue, iBranchLength ), which is the likelihood:
    return log(r.GetCell( iAncestorValue-m_Feature->GetMinVal(), iDescendantValue-m_Feature->GetMinVal() ));
}

//Generate a new rates matrix (return the Hastings ratio or -1):
DblType FeatureRateMatrix::NewCandidate( void )
{
    //Modify the rate m_Alpha using a sliding window approach, bounded between st_LowerBoundary and st_UpperBoundary, with reflection:
    DblType alpha = m_Alpha + (RandomProbability() - 0.5) * st_RateSlidingWindow;
    if( alpha > st_UpperBoundary )
    {
        alpha = st_UpperBoundary - (alpha - st_UpperBoundary);
    }
    if( alpha < st_LowerBoundary )
    {
        alpha = st_LowerBoundary + (st_LowerBoundary - alpha);
    }

    ComputeRateMatrixTemplateForSingleRateModel( alpha );

    SetAlterationFlag( true );

    //The Hastings ratio in this case is 1.0
    return 1.0;
}

//Copy operator:
FeatureRateMatrix &FeatureRateMatrix::operator=( FeatureRateMatrix &iFeatureRateMatrix )
{
    m_Feature = iFeatureRateMatrix.m_Feature;

    m_RateMatrix = iFeatureRateMatrix.m_RateMatrix;

    m_Model = iFeatureRateMatrix.m_Model;

    m_Alpha = iFeatureRateMatrix.m_Alpha;
    m_Beta = iFeatureRateMatrix.m_Beta;

    m_HasBeenAltered = iFeatureRateMatrix.m_HasBeenAltered;

    return *this;
}

void FeatureRateMatrix::CopyContent( FeatureRateMatrix &iMatrix )
{
    m_RateMatrix.CopyContent( iMatrix.m_RateMatrix );

    m_Alpha = iMatrix.m_Alpha;
    m_Beta = iMatrix.m_Beta;

    m_HasBeenAltered = iMatrix.m_HasBeenAltered;
}

//Count the minimum number of changes requried to get from the ancestral to the descendant state value (parsimony style):
unsigned FeatureRateMatrix::CountChangesParsimony( int iAncestorValue, int iDescendantValue )
{
    //Compute the unique rate alpha function of the feature:
    if( m_Model != SINGLE_RATE_MODEL )
    {
        cerr << "This is only implemented for SINGLE_RATE_MODEL!" << endl;
        assert( false );
        exit( 1 );
    }

    switch( m_Feature->GetType() )
    {
    case Feature::FEATURE_UNORDERED:
        return (iAncestorValue == iDescendantValue) ? 0 : 1;
        break;
    case Feature::FEATURE_ORDERED:
        return (iAncestorValue >= iDescendantValue) ? (iAncestorValue - iDescendantValue) : (m_Feature->GetNVals() - (iDescendantValue - iAncestorValue));
        break;
    case Feature::FEATURE_CIRCULAR:
        return min( abs( iAncestorValue - iDescendantValue ), (int)m_Feature->GetMaxVal()-max(iAncestorValue,iDescendantValue) + min(iAncestorValue,iDescendantValue) - (int)m_Feature->GetMinVal() + 1 );
        break;
    case Feature::FEATURE_RANKED:
        return abs( iAncestorValue - iDescendantValue );
        break;
    case Feature::FEATURE_CUSTOM:
        //Use the shortest path matrix to see how many steps are required:
        return m_Feature->GetShortestPath( iAncestorValue, iDescendantValue );
        break;
    default:
        cerr << "Unknown feature type!" << endl;
        assert( false );
        exit( 1 );
        break;
    }

    return (-1);
}




