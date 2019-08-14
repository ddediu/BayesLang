#ifndef TREETEMPLATESSET_H
#define TREETEMPLATESSET_H

#include "Definitions.h"
#include "TreeTemplate.h"

/***********************************************************
*
*           A set of language tree templates
*
***********************************************************/

class TreeTemplatesSet
{
    public:
        TreeTemplatesSet();
        virtual ~TreeTemplatesSet();

        void Init( unsigned iNumberOfTreeTremplets );

        void Print( FILE *iFile, bool paranthesized = true, int iWhichFeature = (-1) );

        TreeTemplate *GetTreeTemplate( unsigned iIndex ){ assert( iIndex < m_NumberOfTreeTremplets ); return &m_TreeTemplates[ iIndex ]; }
        unsigned GetNTreeTemplates( void ){ return m_NumberOfTreeTremplets; }
    protected:
    private:
        //The number of tree templates:
        unsigned m_NumberOfTreeTremplets;

        //The actual tree templates:
        TreeTemplate *m_TreeTemplates;

    private:
        //Memory management:
        void AllocMemory( unsigned iNumberOfTreeTremplets );
        void FreeMemory( void );
};

#endif // TREETEMPLATESSET_H
