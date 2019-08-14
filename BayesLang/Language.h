#ifndef LANGUAGE_H
#define LANGUAGE_H

#include "Definitions.h"

/***********************************************************
*
*             The description of a language
*
***********************************************************/

class Language
{
    public:
        Language();
        Language( const char *iName );
        Language( Language &iLanguage );
        virtual ~Language();

        void Init( const char *iName );

        //Print it to file:
        int Print( FILE *iFile );
        unsigned Print( char *iBuffer );

        //Copy operator:
        Language &operator=( Language &iLanguage );

        //Getters:
        const char *GetName( void ){ return m_Name; }
    protected:
        //The language's name:
        char *m_Name;
};

#endif // LANGUAGE_H
