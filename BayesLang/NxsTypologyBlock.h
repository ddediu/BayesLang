#ifndef NXSTYPOLOGYBLOCK_H
#define NXSTYPOLOGYBLOCK_H

#include <ncl/nxsblock.h>
#include <ncl/nxscharactersblock.h>
#include "SquareMatrix.h"

/***************************************************************
*
* This class implmenets the application-specific TYPOLOGY block
*
****************************************************************/

class NxsTypologyBlock : public NxsBlock
{
    public:
        //The types of typological character:
        enum eTypologicalCharTyes {TYPOLOGICAL_CHAR_UNORDERED, TYPOLOGICAL_CHAR_ORDERED, TYPOLOGICAL_CHAR_CIRCULAR,
                                   TYPOLOGICAL_CHAR_RANKED, TYPOLOGICAL_CHAR_CUSTOM };

    public:
        NxsTypologyBlock(NxsCharactersBlock *iCharacters);
        virtual ~NxsTypologyBlock();

        virtual void	Report(std::ostream &out);

        //Getters:
        unsigned GetNFeatures( void ){ return m_NumberOfCharacters; }
        const char *GetFeatureLabel( unsigned i ){ return (i < m_NumberOfCharacters) ? m_CharacterTypes[i].m_Name.c_str() : NULL; }
        eTypologicalCharTyes GetFeatureType( unsigned i ){ return (i < m_NumberOfCharacters) ? m_CharacterTypes[i].m_Type : TYPOLOGICAL_CHAR_UNORDERED; }
        unsigned GetFeatureNumber( unsigned i ){ return (i < m_NumberOfCharacters) ? m_CharacterTypes[i].m_Number : 0; }
        SquareMatrix &GetFeatureCustomTransitionsMatrix( unsigned i ){ assert(i < m_NumberOfCharacters); return m_CharacterTypes[i].m_CustomTransitionsMatrix; }
        vector<string> &GetFeatureValueLabels( unsigned i){ assert(i < m_NumberOfCharacters); return m_CharacterTypes[i].m_ValueLabels; }

    protected:
		//void			SkippingCommand(NxsString commandName);
		//unsigned		TaxonLabelToNumber(NxsString s) const;
		//unsigned		CharLabelToNumber(NxsString s);
		//void			HandleEndblock(NxsToken &token);
		virtual void	Read(NxsToken &token);
		virtual void	Reset();

    private:
        //The CHARACTERS block defining the linguistic features (characters):
        NxsCharactersBlock *m_CharactersBlock;

        //Store for each character (identified by its name) its type:
        struct st_CharacterType
        {
            st_CharacterType(void):m_Name(""),m_Number(0),m_Type(TYPOLOGICAL_CHAR_UNORDERED){}

            NxsString m_Name;
            unsigned m_Number;
            eTypologicalCharTyes m_Type;

            SquareMatrix m_CustomTransitionsMatrix;

            vector<string> m_ValueLabels;
        };
        unsigned m_NumberOfCharacters;
        st_CharacterType *m_CharacterTypes;

};

#endif // NXSTYPOLOGYBLOCK_H
