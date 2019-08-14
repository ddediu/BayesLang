#ifndef FEATURERATEMATRIX_H
#define FEATURERATEMATRIX_H

#include "Definitions.h"
#include "SquareMatrix.h"
#include "Feature.h"

/******************************************************************
*
*      This holds the trnasition matrix for a given feature
*
*******************************************************************/

class FeatureRateMatrix
{
    public:
        //The possible rate models:
        enum eRateModels { SINGLE_RATE_MODEL /*The simplest model (and with the fewest parameters): all (non-null) rates are equal to alpha and uniformaly distributed*/ };
    public:
        FeatureRateMatrix();
        virtual ~FeatureRateMatrix();

        void SetFeature( Feature *iFeature );
        void SetRateMatrix( SquareMatrix &iRateMatrix );

        void Print( FILE *iFile );
        unsigned Print( char *iBuffer );

        void WriteParamsHeader( FILE *iFile );
        void WriteParamsValues( FILE *iFile );
        unsigned WriteParamsValues( char *iBuffer );

        //Compute the loglikelihood of given change for this feature:
        DblType ComputeLogLikelihood( int iAncestorValue, int iDescendantValue, DblType iBranchLength );

        //Compute the rate matrix for SINGLE_RATE_MODEL for this feature and a desired alpha:
        void ComputeRateMatrixTemplateForSingleRateModel( DblType iAlpha = (-1) );

        //Generate a new rates matrix (return the Hastings ratio or -1):
        DblType NewCandidate( void );

        //Copy operator:
        FeatureRateMatrix &operator=( FeatureRateMatrix &iFeatureRateMatrix );
        void CopyContent( FeatureRateMatrix &iMatrix );

        //Optimizations:
        bool HasBeenAltered( void ){ return m_HasBeenAltered; }
        void ResetAlterationFlag( void ){ m_HasBeenAltered = false; }
        void SetAlterationFlag( bool iFlag ){ m_HasBeenAltered = iFlag; }

        //Count the minimum number of changes requried to get from the ancestral to the descendant state value (parsimony style):
        unsigned CountChangesParsimony( int iAncestorValue, int iDescendantValue );

    protected:
    private:
        //The feature:
        Feature *m_Feature;

        //The current instantaneous rate matrix:
        SquareMatrix m_RateMatrix;

        //The constraints on the instantaneous rates:
        eRateModels m_Model;

        //Generate the rate matrix given the model:
        void ComputeRateMatrixTemplate( Feature *iFeature );

        /***********************************************************************************************************************************************************
        * For SINGLE_RATE_MODEL, the rate matrix looks like:
        * Unordered feature: | b a a ... a |       Ordered feature: | b a 0 ... 0 |    Circular feature: | b a 0 ... a |   Ranked feature: | b a 0 ... 0 |
        *                    | a b a ... a |                        | 0 b a ... 0 |                      | a b a ... 0 |                   | a b a ... 0 |
        *                    | . . . ... . |                        | . . . ... . |                      | . . . ... . |                   | . . . ... . |
        *                    | a a a ... b |                        | a 0 0 ... b |                      | a 0 0 ... b |                   | 0 0 0 ... b |
        * where b = -a*(n-1)                                        where b = -a                         where b = -2a                     where b = -2a or -a
        * Therefore, we need a (m_Alpha), b (m_Beta)
        ***********************************************************************************************************************************************************/
        DblType m_Alpha, m_Beta;

        //Optimizations: has this rates natrix been altered:
        bool m_HasBeenAltered;

    public:
        //Generating new candidate rates: the sliding window's width:
        static DblType st_RateSlidingWindow;
        //The boundaries:
        static DblType st_LowerBoundary;
        static DblType st_UpperBoundary;
};

#endif // FEATURERATEMATRIX_H
