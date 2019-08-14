#ifndef LANGUAGESSET_H
#define LANGUAGESSET_H

#include "Language.h"

/***********************************************************
*
*       Holds info about the languages involved
*
***********************************************************/

class LanguagesSet
{
    public:
        LanguagesSet( void );
        LanguagesSet( unsigned iNumberOfLanguages );
        virtual ~LanguagesSet();

        void Init( unsigned iNumberOfLanguages );

        //Language manipulation:
        Language *GetLanguage( unsigned iIndex ){ return (iIndex < m_NumberOfLanguages) ? &m_Languages[iIndex] : NULL; }
        unsigned GetNLanguages( void ){ return m_NumberOfLanguages; }

        //Print it to file:
        void Print( FILE *iFile );
    protected:
    private:
        //The number of languages held:
        unsigned m_NumberOfLanguages;

        //The actual languages:
        Language *m_Languages;
};

#endif // LANGUAGESSET_H
