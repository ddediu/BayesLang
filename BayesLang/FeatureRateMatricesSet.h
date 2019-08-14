#ifndef FEATURERATEMATRICESSET_H
#define FEATURERATEMATRICESSET_H

#include "Definitions.h"
#include "FeatureRateMatrix.h"
#include "FeaturesSet.h"

/******************************************************************
*
*         Holds the transition matrices for all features
*
*******************************************************************/

class FeatureRateMatricesSet
{
    public:
        FeatureRateMatricesSet();
        FeatureRateMatricesSet( FeaturesSet &iFeaturesSet );
        virtual ~FeatureRateMatricesSet();

        void Init( FeaturesSet &iFeaturesSet );

        FeatureRateMatrix *GetRateMatrix4Feature( int iWhichFeature ){ assert( iWhichFeature >= 0 && iWhichFeature < (int)m_NumberOfMatrices); return &m_RateMatrices[iWhichFeature]; }
        unsigned GetNFeatures( void ){ return m_NumberOfMatrices; }
        Feature *GetFeature( unsigned iIndex ){ return m_FeaturesSet->GetFeature( iIndex ); }

        void Print( FILE *iFile, int iWhichFeature = (-1) );
        unsigned Print( char *iBuffer, int iWhichFeature = (-1) );

        void WriteParamsHeader( FILE *iFile );
        void WriteParamsValues( FILE *iFile );
        unsigned WriteParamsValues( char *iBuffer );

        //Generate a new rates matrix (return the Hastings ratio or -1):
        DblType NewCandidate( int iWhichFeature );

        //Copy operator:
        FeatureRateMatricesSet &operator=( const FeatureRateMatricesSet &iFeatureRateMatricesSet );
        void CopyContent( const FeatureRateMatricesSet &iMatrices );

    protected:
    private:
        //The features set (for referencing purposes):
        FeaturesSet *m_FeaturesSet;

        //Number of features (for caching purposes):
        unsigned m_NumberOfMatrices;

        //The rate matrix for each feature:
        FeatureRateMatrix *m_RateMatrices;

    private:
        //Memory management:
        void AllocMemory( unsigned iNumberOfFeatures );
        void FreeMemory( void );
};

#endif // FEATURERATEMATRICESSET_H
