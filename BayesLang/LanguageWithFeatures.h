#ifndef LANGUAGEWITHFEATURES_H
#define LANGUAGEWITHFEATURES_H

#include "Language.h"
#include "FeaturesSet.h"
#include "FeatureValuesMatrix.h"
#include "FeatureRateMatrix.h"

/*************************************************************************
*
*               Contains a language _with_ feature values
*
*************************************************************************/

class LanguageWithFeatures : public Language
{
    public:
        //The type of value data:
        enum eValueDataType {VALUE_DEFINED, VALUE_MISSING, VALUE_ANCESTRAL };

    public:
        LanguageWithFeatures();
        LanguageWithFeatures( FeatureValuesMatrix &iFeatureValuesMatrix, FeaturesSet *iFeaturesSet );
        LanguageWithFeatures( Language &iLanguage, FeatureValuesMatrix &iFeatureValuesMatrix, FeaturesSet *iFeaturesSet );
        LanguageWithFeatures( LanguageWithFeatures &iLanguageWithFeatures );
        virtual ~LanguageWithFeatures();

        void Init( const char *iName );

        //Copy operator:
        LanguageWithFeatures &operator=( Language &iLanguage );
        LanguageWithFeatures &operator=( LanguageWithFeatures &iLanguage );

        void Print( FILE *iFile, int iWhichFeature = (-1), bool iPrettyPrint = false );
        unsigned Print( char *iBuffer, int iWhichFeature = (-1), bool iPrettyPrint = false );
        void WriteParamsHeader( FILE *iFile );
        void WriteParamsValues( FILE *iFile );
        unsigned WriteParamsValues( char *iBuffer );

        //Getters:
        int GetFeatureValue( int iWhichFeature ){ assert( iWhichFeature >= 0 && iWhichFeature < (int)m_FeaturesSet->GetNFeatures() ); return m_FeatureValues[iWhichFeature]; }
        unsigned GetNFeatures( void ){ return m_FeaturesSet->GetNFeatures(); }
        eValueDataType GetFeatureType( int iWhichFeature ){ assert( iWhichFeature >= 0 && iWhichFeature < (int)m_FeaturesSet->GetNFeatures() ); return m_DataType[iWhichFeature]; }
        bool IsMissing( int iWhichFeature ){ return GetFeatureType( iWhichFeature ) == VALUE_MISSING; }

        //Test if the required feature(s) have undefined value(s) [iWhichFeature = (-1) == all features]:
        bool UndefinedFeatures( int iWhichFeature = (-1) );

        //Generate values for the features with missing values accroding to their description:
        void GenerateMissingValues( FeatureRateMatrix *iRateMatrix, int iWhichFeature = (-1) );

        //Generate a new candidate value for a given real feature (return the Hastings ratio or (-1)):
        DblType NewCandidate( int iWhichFeature );

        //Copy the volatile content (feature values):
        void CopyContent( LanguageWithFeatures &iLanguage );

        //Get the log of the prior probability of this language (for ratio computation, i.e. without any multiplicative constants):
        DblType GetLogPriorProbability( int iWhichFeature = (-1) );

    protected:
    private:
        //The feature values matrix:
        FeatureValuesMatrix *m_FeatureValuesMatrix;

        //The features:
        FeaturesSet *m_FeaturesSet;

        //The actual values:
        int *m_FeatureValues;
        //Do they replace missing or ancestral (undefined) data?
        eValueDataType *m_DataType;

    private:
        //Set the features set:
        void SetFeaturesSet( FeaturesSet *iFeaturesSet );
        //Print the data type:
        char PrintDataType( eValueDataType iDataType, bool iAsLetter = false );

};

#endif // LANGUAGEWITHFEATURES_H
