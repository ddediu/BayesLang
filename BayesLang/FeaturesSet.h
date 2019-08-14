#ifndef FEATURESSET_H
#define FEATURESSET_H

#include "Feature.h"

/***********************************************************
*
*       Holds info about the features involved
*
***********************************************************/

class FeaturesSet
{
    public:
        FeaturesSet();
        FeaturesSet( unsigned iNumberOfFeatures );
        virtual ~FeaturesSet();

        void Init( unsigned iNumberOfFeatures );

        //Feature manipulation:
        Feature *GetFeature( unsigned iIndex ){ return (iIndex < m_NumberOfFeatures) ? &m_Features[iIndex] : NULL; }
        unsigned GetNFeatures( void ){ return m_NumberOfFeatures; }

        //Print it to file:
        void Print( FILE *iFile );
    protected:
    private:
        //The number of features held:
        unsigned m_NumberOfFeatures;

        //The actual features:
        Feature *m_Features;
};

#endif // FEATURESSET_H
