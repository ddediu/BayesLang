#include "NxsTypologyBlock.h"
#include "Definitions.h"

NxsTypologyBlock::NxsTypologyBlock(NxsCharactersBlock *iCharacters)
:m_CharactersBlock(iCharacters),m_NumberOfCharacters(0),m_CharacterTypes(NULL)
{
    id = "TYPOLOGY";
}

NxsTypologyBlock::~NxsTypologyBlock()
{
    if( m_NumberOfCharacters && m_CharacterTypes )
    {
        delete[] m_CharacterTypes;
    }
}

///*----------------------------------------------------------------------------------------------------------------------
//|	The code here is identical to the base class version (simply returns 0), so the code here should either be modified
//|	or this derived version eliminated altogether. Under what circumstances would you need to modify the default code,
//|	you ask? This function should be modified to something meaningful if this derived class needs to construct and run
//|	a NxsSetReader object to read a set involving characters. The NxsSetReader object may need to use this function to
//|	look up a character label encountered in the set. A class that overrides this method should return the character
//|	index in the range [1..`nchar']; i.e., add one to the 0-offset index.
//*/
//unsigned NxsTypologyBlock::CharLabelToNumber( NxsString s)	/* the character label to be translated to character number */
//{
//	return 0;
//}


/*----------------------------------------------------------------------------------------------------------------------
|	This function provides the ability to read everything following the block name (which is read by the NxsReader
|	object) to the END or ENDBLOCK statement. Characters are read from the input stream `in'. Overrides the pure
|	virtual function in the base class.
*/
void NxsTypologyBlock::Read(NxsToken &token)	/* the token used to read from `in'*/
{
	isEmpty = false;
	NxsString s;
	s = "BEGIN ";
	s += id;
	DemandEndSemicolon(token, s.c_str());

	//Put all characters read so far to UNORDERED:
	m_NumberOfCharacters = m_CharactersBlock->GetNChar();
	m_CharacterTypes = new st_CharacterType[m_NumberOfCharacters];
	if( !m_CharacterTypes )
	{
        errormsg.PrintF("Not enough memory!");
        throw NxsException(errormsg, token.GetFilePosition(), token.GetFileLine(), token.GetFileColumn());
        assert( false );
	}
	unsigned i;
	for( i = 0; i < m_NumberOfCharacters; ++i )
	{
	    m_CharacterTypes[i].m_Number = i+1;
	    m_CharacterTypes[i].m_Name = m_CharactersBlock->GetCharLabel(i);
	    m_CharacterTypes[i].m_Type = TYPOLOGICAL_CHAR_UNORDERED;
	}

	for(;;)
		{
            token.GetNextToken();
            NxsBlock::NxsCommandResult res = HandleBasicBlockCommands(token);
            if (res == NxsBlock::NxsCommandResult(STOP_PARSING_BLOCK))
                return;

            if( token.Equals( "chartypes", false ) )
            {
                //Get the next one:
                token.GetNextToken();
                /*if (res == NxsBlock::NxsCommandResult(STOP_PARSING_BLOCK))
                    return;*/

                while( !token.Equals( ";", false ) )
                {
                    if( token.Equals( "Unordered", false ) || token.Equals( "Ordered", false ) ||
                        token.Equals( "Circular", false ) || token.Equals( "Ranked", false ) ||
                        token.Equals( "Custom", false ) )
                    {
                        //Set the type:
                        eTypologicalCharTyes currentType;
                        if( token.Equals( "Unordered", false ) )
                        {
                            currentType = TYPOLOGICAL_CHAR_UNORDERED;
                        }
                        else if( token.Equals( "Ordered", false ) )
                        {
                            currentType = TYPOLOGICAL_CHAR_ORDERED;
                        }
                        else if( token.Equals( "Circular", false ) )
                        {
                            currentType = TYPOLOGICAL_CHAR_CIRCULAR;
                        }
                        else if( token.Equals( "Ranked", false ) )
                        {
                            currentType = TYPOLOGICAL_CHAR_RANKED;
                        }
                        else
                        {
                            currentType = TYPOLOGICAL_CHAR_CUSTOM;
                        }
                        //Skip the ":":
                        token.GetNextToken();
                        if( !token.Equals( ":", false ) )
                        {
                            errormsg.PrintF("Expecting ':' after UNORDERED, ORDERED, CIRCULAR, RANKED or CUSTOM, but found %s instead.", token.GetTokenAsCStr());
                            throw NxsException(errormsg, token.GetFilePosition(), token.GetFileLine(), token.GetFileColumn());
                            assert( false );
                        }

                        //Get the character names until "," or ";":
                        token.GetNextToken();
                        while( !token.Equals( ",", false ) && !token.Equals( ";", false ) )
                        {
                            unsigned curCharNumber = m_CharactersBlock->CharLabelToNumber( token.GetToken() );
                            if( curCharNumber == 0 )
                            {
                                //This is not a known label: try an index:
                                if( !token.GetToken().IsALong() || (curCharNumber = token.GetToken().ConvertToInt()+1) == 0 )
                                {
                                    //Well, we have a problem!
                                    errormsg.PrintF("Unknown character \"%s\".", token.GetTokenAsCStr());
                                    throw NxsException(errormsg, token.GetFilePosition(), token.GetFileLine(), token.GetFileColumn());
                                    assert( false );
                                }
                            }

                            //cout << "Character \"" << token.GetToken() << "\" has number " << curCharNumber << " and type " << currentType << ". ";

                            //Store the info about this character:
                            m_CharacterTypes[curCharNumber-1].m_Type = currentType;

                            //Get next token:
                            token.GetNextToken();
                        }
                        if( token.Equals( ",", false ) )
                        {
                            token.GetNextToken();
                        }
                    }
                    else
                    {
                        //Unknown token:
                        errormsg.PrintF("Unknown token \"%s\".", token.GetTokenAsCStr());
                        throw NxsException(errormsg, token.GetFilePosition(), token.GetFileLine(), token.GetFileColumn());
                        assert( false );
                    }
                }
            }
            else if( token.Equals( "custommatrix", false ) )
            {
                // Get the feature name:
                token.GetNextToken();
                unsigned curCharNumber = m_CharactersBlock->CharLabelToNumber( token.GetToken() );
                if( curCharNumber == 0 )
                {
                    //This is not a known label: try an index:
                    if( !token.GetToken().IsALong() || (curCharNumber = token.GetToken().ConvertToInt()+1) == 0 )
                    {
                        //Well, we have a problem!
                        errormsg.PrintF("Unknown character \"%s\".", token.GetTokenAsCStr());
                        throw NxsException(errormsg, token.GetFilePosition(), token.GetFileLine(), token.GetFileColumn());
                        assert( false );
                    }
                }

                //Get the number of values:
                token.GetNextToken();
                int nValues=0;
                if( !token.GetToken().IsALong() || (nValues = token.GetToken().ConvertToInt()) == -1 || nValues > MAX_NUMBER_OF_FEATURE_VALUES )
                {
                    //This is not a legal number of values:
                    errormsg.PrintF("Illegal number of values %d.", nValues);
                    throw NxsException(errormsg, token.GetFilePosition(), token.GetFileLine(), token.GetFileColumn());
                    assert( false );
                }

                //Alloc the matrix
                m_CharacterTypes[curCharNumber-1].m_CustomTransitionsMatrix.Init( nValues );

                //Read the matrix here:
                for( unsigned column = 0; (int)column < nValues; ++column )
                {
                    for( unsigned row = 0; (int)row < nValues; ++row )
                    {
                        //Get the current cell:
                        token.GetNextToken();

                        if( token.Equals( "-", false ) )
                        {
                            if( column != row )
                            {
                                errormsg.PrintF("\"-\" allowed only for the main diagonal.");
                                throw NxsException(errormsg, token.GetFilePosition(), token.GetFileLine(), token.GetFileColumn());
                                assert( false );
                            }
                            else
                            {
                                m_CharacterTypes[curCharNumber-1].m_CustomTransitionsMatrix.SetCell( row, column, 1 );
                            }
                        }
                        else if( token.Equals( "0", false ) )
                        {
                            //Store a 0
                            m_CharacterTypes[curCharNumber-1].m_CustomTransitionsMatrix.SetCell( row, column, 0 );
                        }
                        else if( token.Equals( "1", false ) )
                        {
                            //Store a 0
                            m_CharacterTypes[curCharNumber-1].m_CustomTransitionsMatrix.SetCell( row, column, 1 );
                        }
                        else
                        {
                            errormsg.PrintF("Illegal token \"%s\" in custom feature matrix (only -,0 and 1 are allowed).",token.GetTokenAsCStr());
                            throw NxsException(errormsg, token.GetFilePosition(), token.GetFileLine(), token.GetFileColumn());
                            assert( false );
                       }
                    }
                }

                //The last token must be ";":
                token.GetNextToken();
                if( !token.Equals( ";", false ) )
                {
                    errormsg.PrintF("custommatrix must end with \";\".");
                    throw NxsException(errormsg, token.GetFilePosition(), token.GetFileLine(), token.GetFileColumn());
                    assert( false );
                }
            }
            else if( token.Equals( "valuelabels", false ) )
            {
                //Read the value labels for the features:
                //Get the next one:
                token.GetNextToken();
                /*if (res == NxsBlock::NxsCommandResult(STOP_PARSING_BLOCK))
                    return;*/

                while( !token.Equals( ";", false ) )
                {
                    //This should be a feature name:
                    unsigned curCharNumber = m_CharactersBlock->CharLabelToNumber( token.GetToken() );
                    if( curCharNumber == 0 )
                    {
                        //This is not a known label: try an index:
                        if( !token.GetToken().IsALong() || (curCharNumber = token.GetToken().ConvertToInt()+1) == 0 )
                        {
                            //Well, we have a problem!
                            errormsg.PrintF("Unknown character \"%s\".", token.GetTokenAsCStr());
                            throw NxsException(errormsg, token.GetFilePosition(), token.GetFileLine(), token.GetFileColumn());
                            assert( false );
                        }
                    }

                    //Skip the ":":
                    token.GetNextToken();
                    if( !token.Equals( ":", false ) )
                    {
                        errormsg.PrintF("Expecting \":\" after feature name!");
                        throw NxsException(errormsg, token.GetFilePosition(), token.GetFileLine(), token.GetFileColumn());
                        assert( false );
                    }

                    //Get the labels until "," or ";":
                    token.GetNextToken();
                    while( !token.Equals( ",", false ) && !token.Equals( ";", false ) )
                    {
                        //Store it:
                        m_CharacterTypes[curCharNumber-1].m_ValueLabels.push_back( token.GetTokenAsCStr() );

                        //Get next token:
                        token.GetNextToken();
                    }
                    if( token.Equals( ",", false ) )
                    {
                        token.GetNextToken();
                    }
                }
            }

            if (res != NxsBlock::NxsCommandResult(HANDLED_COMMAND))
                SkipCommand(token);
		}
}

/*----------------------------------------------------------------------------------------------------------------------
|	Sets `isEmpty' to true in preparation for reading a new TYPOLOGY block. Overrides the pure virtual function in the
|	base class.
*/
void NxsTypologyBlock::Reset()
{
    if( m_NumberOfCharacters && m_CharacterTypes )
    {
        delete[] m_CharacterTypes;
    }

    m_NumberOfCharacters = 0;
    m_CharacterTypes = NULL;

	NxsBlock::Reset();
}

/*----------------------------------------------------------------------------------------------------------------------
|	This function outputs a brief report of the contents of this TYPOLOGY block. Overrides the pure virtual function in
|	the base class.
*/
void NxsTypologyBlock::Report(std::ostream &out)	/* the output stream to which to write the report */
{
	out << endl;
	out << id << " block contains...";
}

/*----------------------------------------------------------------------------------------------------------------------
|	This function is called when an unknown command named `commandName' is about to be skipped. This version of the
|	function (which is identical to the base class version) does nothing (i.e., no warning is issued that a command
|	was unrecognized). Modify this virtual function to provide such warnings to the user (or eliminate it altogether
|	since the base class version already does what this does).
*/
//void NxsTypologyBlock::SkippingCommand(NxsString commandName)	/* the name of the command being skipped */
//{
//}

/*----------------------------------------------------------------------------------------------------------------------
|	The code here is identical to the base class version (simply returns 0), so the code here should either be modified
|	or this derived version eliminated altogether. Under what circumstances would you need to modify the default code,
|	you ask? This function should be modified to something meaningful if this derived class needs to construct and run
|	a NxsSetReader object to read a set involving taxa. The NxsSetReader object may need to use this function to look
|	up a taxon label encountered in the set. A class that overrides this method should return the taxon index in the
|	range [1..ntax]; i.e., add one to the 0-offset index.
*/
//unsigned NxsTypologyBlock::TaxonLabelToNumber(NxsString s) const	/* the taxon label to be translated to a taxon number */
//{
//	return 0;
//}

//void NxsTypologyBlock::HandleEndblock(NxsToken &token)
//{
//}
