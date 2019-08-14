#ifndef FEATURE_H
#define FEATURE_H

#include "Definitions.h"
#include "SquareMatrix.h"

/***********************************************************
*
* A linguistic feature, which can be ordered or unordered
*
***********************************************************/

class Feature
{
    public:
        //The types:
        enum eFeatureTypes { FEATURE_UNORDERED, FEATURE_ORDERED, FEATURE_CIRCULAR, FEATURE_RANKED, FEATURE_CUSTOM };

    public:
        Feature( void );
        Feature( const char *iName, eFeatureTypes iType, unsigned iMinVal, unsigned iMaxVal, vector<string> &iValueLables );
        virtual ~Feature();

        void Init( const char *iName, eFeatureTypes iType, unsigned iMinVal, unsigned iMaxVal, vector<string> &iValueLables );

        void SetMinMaxVals( unsigned iMinVal, unsigned iMaxVal )
        {
            m_MinVal = iMinVal; m_MaxVal = iMaxVal; m_NVals = m_MaxVal - m_MinVal + 1; m_LogNVals = log( m_NVals );
            if( m_Type == FEATURE_CUSTOM && m_CustomTransitionsMatrix.GetSize() != m_NVals )
            {
                cerr << "The declared number of values (" << m_NVals << ") for feature \"" << GetName() << "\" is different from the custom transition matrix size (" << m_CustomTransitionsMatrix.GetSize() << ")!" << endl;
                assert( false );
                exit( 1 );
            }
            if( m_NumberOfLabels > 0 && m_NumberOfLabels != m_NVals )
            {
                cerr << "The number of value lables must be equal to the actual number of values for feature \"" << GetName() << "\"!" << endl;
                assert( false );
                exit( 1 );
            }
        }
        void SetCustomTransitionsMatrix( SquareMatrix &iMatrix );
        SquareMatrix &GetCustomTransitionsMatrix( void ){ return m_CustomTransitionsMatrix; }

        unsigned GetMaxVal( void ){ return m_MaxVal; }
        unsigned GetMinVal( void ){ return m_MinVal; }
        unsigned GetNVals( void ){ return m_NVals; }
        DblType GetLogNVals( void ){ return m_LogNVals; }

        unsigned GetNValueLabels( void ){ return m_NumberOfLabels; }
        const char *GetValueLabel( unsigned i ){ assert( i < m_NumberOfLabels ); return m_Labels[i]; }

        eFeatureTypes GetType( void ){ return m_Type; }

        char *GetName( void ){ return m_Name; }

        int GetShortestPath( int iStart, int iEnd )
        {
            assert( iStart >= (int)m_MinVal && iStart <= (int)m_MaxVal && iEnd >= (int)m_MinVal && iEnd <= (int)m_MaxVal );
            return (iStart == iEnd) ? 0 : m_CustomShortestPathsMatrix.GetCell( iStart - m_MinVal, iEnd - m_MinVal );
        }

        //Print it to file:
        void Print( FILE *iFile );
        unsigned Print( char *iBuffer );
    protected:
    private:
        //The feature name:
        char *m_Name;

        //The type:
        eFeatureTypes m_Type;

        //For custom features, the custom transition matrix as defined by the user:
        SquareMatrix m_CustomTransitionsMatrix;
        //...and the shortest path between any two nodes:
        SquareMatrix m_CustomShortestPathsMatrix;

        //The allowed values:
        unsigned m_MinVal;
        unsigned m_MaxVal;

        //The range of values:
        unsigned m_NVals;
        //The log range of values (used for log prior computation and cached here for speed):
        DblType m_LogNVals;

        //Any special labels associated with it:
        unsigned m_NumberOfLabels;
        char **m_Labels;

};

#endif // FEATURE_H
