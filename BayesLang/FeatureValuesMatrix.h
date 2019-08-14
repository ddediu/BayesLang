#ifndef FEATUREVALUESMATRIX_H
#define FEATUREVALUESMATRIX_H

#include "Definitions.h"
#include "LanguagesSet.h"
#include "FeaturesSet.h"

/***********************************************************
*
* The matrix of feature values for each language
*
***********************************************************/

class FeatureValuesMatrix
{
    public:
        //The missing data value:
        static const int c_MissingData;

    public:
        FeatureValuesMatrix();
        FeatureValuesMatrix( unsigned iNumberOfLanguages, unsigned iNumberOfFeatures, LanguagesSet *iLanguagesSet, FeaturesSet *iFeaturesSet );
        virtual ~FeatureValuesMatrix();

        void Init( unsigned iNumberOfLanguages, unsigned iNumberOfFeatures, LanguagesSet *iLanguagesSet, FeaturesSet *iFeaturesSet );

        void SetValue( unsigned iLanguage, unsigned iFeature, int iValue );
        int GetValue( unsigned iLanguage, unsigned iFeature );

        //Print it to file:
        void Print( FILE *iFile );

        //Return the language name if found or (-1) otherwise:
        int GetLanguageIndexFromName( const char *iName );
    protected:
    private:
        //The number of languages:
        unsigned m_NumberOfLanguages;
        //The languages set:
        LanguagesSet *m_LanguagesSet;
        //The language indices in the languages set (the columns):
        unsigned *m_LanguagesIndices;

        //The number of features:
        unsigned m_NumberOfFeatures;
        //The features set:
        FeaturesSet *m_FeaturesSet;
        //The features indices in the features set (the rows):
        unsigned *m_FeaturesIndices;

        //The actual matrix of feature values:
        int **m_Values;

    private:
        //Memory management:
        void AllocMemory( unsigned iNumberOfLanguages, unsigned iNumberOfFeatures );
        void FreeMemory( void );
};

#endif // FEATUREVALUESMATRIX_H
