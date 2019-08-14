#ifndef INFOHOLDER_H
#define INFOHOLDER_H

#include "Definitions.h"
#include "LanguagesSet.h"
#include "FeaturesSet.h"
#include "FeatureValuesMatrix.h"
#include "ncl/nxstaxablock.h"
#include "NxsTypologyBlock.h"
#include "ncl/nxscharactersblock.h"
#include "ncl/nxstreesblock.h"
#include "TreeTemplatesSet.h"

/***********************************************************
*
*           The holder of model information
*
***********************************************************/

class InfoHolder
{
    public:
        InfoHolder();
        virtual ~InfoHolder();

        //Init the infor holder:
        void Init( NxsTaxaBlock *iNxsTaxaBlock, NxsCharactersBlock *iNxsCharactersBlock, NxsTypologyBlock *iNxsTypologyBlock, NxsTreesBlock *iNxsTreesBlock );

        //Getters:
        Feature *GetFeature( unsigned iIndex ){ return m_FeaturesSet.GetFeature( iIndex ); }

        unsigned GetNTreeTemplates( void ){ return m_TreeTemplatesSet.GetNTreeTemplates(); }
        TreeTemplate *GetTreeTemplate( unsigned iIndex ){ return m_TreeTemplatesSet.GetTreeTemplate( iIndex ); }

        //Print it to file:
        void Print( FILE *iFile );
    protected:
    private:
        //The languages set:
        LanguagesSet m_LanguagesSet;
        //The features set:
        FeaturesSet m_FeaturesSet;
        //The values matrix:
        FeatureValuesMatrix m_ValuesMatrix;
        //The tree templates:
        TreeTemplatesSet m_TreeTemplatesSet;

    private:
        //Set the languages:
        void SetLanguages( NxsTaxaBlock *iNxsTaxaBlock );
        //Set the features:
        void SetFeatures( NxsTypologyBlock *iNxsTypologyBlock );
        //Set the values matrix:
        void SetValuesMatrix( NxsCharactersBlock *iNxsCharactersBlock );
        //Set the tree templates:
        void SetTreeTemplates( NxsTreesBlock *iNxsTreesBlock );

    public:
        //The features info file:
        static const char *st_FeaturesFileName;
};

#endif // INFOHOLDER_H
