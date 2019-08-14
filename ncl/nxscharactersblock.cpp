//	Copyright (C) 1999-2003 Paul O. Lewis
//
//	This file is part of NCL (Nexus Class Library) version 2.0.
//
//	NCL is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation; either version 2 of the License, or
//	(at your option) any later version.
//
//	NCL is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with NCL; if not, write to the Free Software Foundation, Inc., 
//	59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//
/**
 * This file includes contributions by Brian O'Meara. August 2005.
 * These changes include the ability to parse continuous data types.
 */
#include <iomanip>
#include <climits>

#include "ncl/nxscharactersblock.h"

#include "ncl/nxsreader.h"
#include "ncl/nxsassumptionsblock.h"
#include "ncl/nxssetreader.h"
#include <algorithm>
using namespace std;

	
std::vector<double>  NxsTransformationManager::GetDoubleWeights(const std::string &set_name) const
	{
	std::vector<double> r; 
	const ListOfDblWeights *p = 0L;
	std::map<std::string, ListOfDblWeights>::const_iterator dIt = dblWtSets.begin();
	for (; dIt != dblWtSets.end(); ++dIt)
		{
		if (NxsString::case_insensitive_equals(dIt->first.c_str(), set_name.c_str()))
			{
			p = &(dIt->second);
			break;
			}
		}
	if (p)
		{
		ListOfDblWeights::const_iterator wIt = p->begin();
		const ListOfDblWeights::const_iterator ewIt = p->end();
		for (; wIt != ewIt; ++wIt)
			{
			double w = wIt->first;
			const std::set<unsigned> &s = wIt->second;
			std::set<unsigned>::const_reverse_iterator ip = s.rbegin();
			const std::set<unsigned>::const_reverse_iterator e = s.rend();
			for (; ip != e; ++ip)
				{
				if (*ip >= r.size())
					r.resize(1 + *ip, 1.0);
				r[*ip] = w;
				}
			}
		}
	return r;
	}

std::vector<int> NxsTransformationManager::GetIntWeights(const std::string &set_name) const
	{
	std::vector<int> r; 
	const ListOfIntWeights *p = 0L;
	std::map<std::string, ListOfIntWeights>::const_iterator dIt = intWtSets.begin();
	for (; dIt != intWtSets.end(); ++dIt)
		{
		if (NxsString::case_insensitive_equals(dIt->first.c_str(), set_name.c_str()))
			{
			p = &(dIt->second);
			break;
			}
		}
	if (p)
		{
		ListOfIntWeights::const_iterator wIt = p->begin();
		const ListOfIntWeights::const_iterator ewIt = p->end();
		for (; wIt != ewIt; ++wIt)
			{
			int w = wIt->first;
			const std::set<unsigned> &s = wIt->second;
			std::set<unsigned>::const_reverse_iterator ip = s.rbegin();
			const std::set<unsigned>::const_reverse_iterator e = s.rend();
			for (; ip != e; ++ip)
				{
				if (*ip >= r.size())
					r.resize(1 + *ip, 1);
				r[*ip] = w;
				}
			}
		}
	return r;
	}

NxsDiscreteDatatypeMapper::NxsDiscreteDatatypeMapper(
	NxsCharactersBlock::DataTypesEnum datatypeE, 
	const std::string & symbolsStr,
	char missingChar, 
	char gap, 
	char matchingChar,
	bool respectingCase, 
	const std::map<char, NxsString> & moreEquates)
	:cLookup(NULL),
	stateCodeLookupPtr(NULL),
	symbols(symbolsStr),
	nStates(0),
	matchChar(matchingChar),
	gapChar(gap),
	missing(missingChar),
	respectCase(respectingCase),
	extraEquates(moreEquates),
	datatype(datatypeE),
	restrictionDataype(false),
	userDefinedEquatesBeforeConversion(false)
	{
	if (symbols.empty())
		symbols = NxsCharactersBlock::GetDefaultSymbolsForType(datatype);
	if (datatype == NxsCharactersBlock::mixed)
		throw NxsException("Cannot create a mixed datatype mapper"); // this should be the only empty string-generating datatype
	RefreshMappings(0L);
	}

void NxsDiscreteDatatypeMapper::DebugPrint(std::ostream & out) const
	{
	out << GetNumStatesIncludingGap() << "states (";
	if (gapChar == '\0')
		out << "no gaps";
	else
		out << "including the gap \"state\"";
	const int nsc = (int) stateSetsVec.size();
	out << '\n' << nsc << " state codes.\n";
	out << "NEXUS     State Code      States\n";
	for (int sc = sclOffset; sc < sclOffset + nsc; ++sc)
		{
		std::string nex;
		for (int c = 0; c < 127; ++c)
			{
			if (cLookup[c] == sc)
				nex.append(1, (char) c);
			}
		int buf =  (int) (10 - nex.size());
		nex.append(buf, ' ');
		out << nex << "    " << sc << "     ";
		const std::set<int>	&ss = GetStateSetForCode(sc);
		std::string decoded;
		for (std::set<int>::const_iterator s = ss.begin(); s != ss.end(); ++s)
			decoded.append(StateCodeToNexusString(*s));
		if (decoded.length() < 2)
			out << decoded;
		else if (IsPolymorphic(sc))
			out << '(' << decoded << ')';
		else
			out << '{' << decoded << '}';
		out << '\n';
		}
	}

/*----------------------------------------------------------------------------------------------------------------------
|	Takes the parsed settings that pertain to the datatype and converts them into a set of NxsDiscreteDatatypeMapper 
|	objects to be used to encode the characters.
*/
void NxsCharactersBlock::CreateDatatypeMapperObjects(const NxsPartition & dtParts, const std::vector<DataTypesEnum> & dtcodes)
	{
	if (datatype != mixed)
		{
		NxsDiscreteDatatypeMapper d(datatype, symbols, missing, gap, matchchar, respectingCase, userEquates);
		datatype = d.GetDatatype();	
		DatatypeMapperAndIndexSet das(d, NxsUnsignedSet());
		datatypeMapperVec.clear();
		datatypeMapperVec.push_back(das);
		}
	else
		{
		datatypeMapperVec.clear();
		assert(dtParts.size() == dtcodes.size());
		datatypeMapperVec.reserve(dtParts.size());
		std::vector<DataTypesEnum>::const_iterator cIt = dtcodes.begin();
		for (NxsPartition::const_iterator pIt = dtParts.begin(); pIt != dtParts.end(); ++pIt, ++cIt)
			{
			std::string mt;
			if (*cIt == standard)
				mt.assign("0123456789"); /*mrbayes is the only program to support MIXED and it uses a default (not extendable) symbols list of 0123456789 rather than 01*/
			NxsDiscreteDatatypeMapper d(*cIt, mt, missing, gap, matchchar, respectingCase, userEquates);
			DatatypeMapperAndIndexSet das(d, pIt->second);
			datatypeMapperVec.push_back(das);
			}
		}
	}




/*----------------------------------------------------------------------------------------------------------------------
|	If you say FORMAT DATATYPE=DNA SYMBOLS="01" ; then the valid symbols become "ACGT01"
|	
|	AugmentedSymbolsToMixed tries to split such a matrix into a datatype=mixed(dna:charset_dna,standard:charset_std)
|	by inferring the charpartition (charset_dna,charset_std).  It does this by using GetNamedStateSetOfColumn to
|	detect which states were listed in a column.
|	
|	Returns true if the translation to mixed was performed.  This will only occur if GetOriginalDataType() == GetDataType()
| 	because this is the symptom that there was symbol augmentation of a built in datatype.
|
|	Note that in the GetNamedStateSetOfColumn
|	then ? will not expand the states present in a symbol. Thus when parsing:
|	Matrix 1:                     Matrix 2:
|	s   ACGT10{ACGT01-}           s   ACGT10?
|	t   ACGT100                   t   ACGT100
|		The last character of the first taxon would be parsed as having the potential to have states {ACGT01-}. 
|		But when interperted with GetNamedStateSetOfColumn, Matrix 2 can be "explained" by four DNA columns, and three
|		Standard (01) columns.  Matrix 1, on the other hand would be found to have four DNA columns, and two
|		Standard (01) columns, and one standard ("ACGT01") column.
|
|	Temporary:  Will return false if userDefinedEquatesBeforeConversion is true
*/
bool NxsCharactersBlock::AugmentedSymbolsToMixed()
	{
	DataTypesEnum odt = GetOriginalDataType();
	if (IsMixedType() || (odt == GetDataType()))
		return false;
	const std::string origSymb = GetDefaultSymbolsForType(odt);
	const std::string cutSymb = symbols.substr(0, origSymb.length());
	if (origSymb != cutSymb)
		return false;
	const std::string augmentSymbols = symbols.substr(origSymb.length());
	if (augmentSymbols.empty())
		return false;
	for (std::string::const_iterator a = augmentSymbols.begin(); a != augmentSymbols.end(); ++a)
		{
		if (!isdigit(*a))
			return false;
		}
	
	NxsUnsignedSet stdTypeChars;
	NxsUnsignedSet origTypeChars;
	std::set<int> torigStateInds;
	std::set<int> tstdStateInds;
	torigStateInds.insert(NXS_GAP_STATE_CODE);
	tstdStateInds.insert(NXS_GAP_STATE_CODE);
	for (unsigned j = 0; j < origSymb.length(); ++j)
		torigStateInds.insert(j);
	for (unsigned j =  (unsigned)origSymb.length(); j < (unsigned)symbols.length(); ++j)
		tstdStateInds.insert(j);
	const std::set<int> origStateInds(torigStateInds);
	const unsigned nosi = (unsigned)origStateInds.size();
	const std::set<int> stdStateInds(tstdStateInds);
	const unsigned nssi = (unsigned)stdStateInds.size();

	/*Check each column for patterns that can not be mapped to origSymb or augmentSymbols */
	const unsigned nChars = GetNCharTotal();
	for (unsigned colIndex = 0; colIndex < nChars; ++colIndex)
		{
		const std::set<int> cs = GetNamedStateSetOfColumn(colIndex);
		std::set<int> origUnion;
		set_union(origStateInds.begin(), origStateInds.end(), cs.begin(), cs.end(), inserter(origUnion, origUnion.begin()));
		if (origUnion.size() > nosi)
			{
			std::set<int> stdUnion;
			set_union(stdStateInds.begin(), stdStateInds.end(), cs.begin(), cs.end(), inserter(stdUnion, stdUnion.begin()));
			if (stdUnion.size() > nssi)
				return false;
			stdTypeChars.insert(colIndex);
			}
		else
			origTypeChars.insert(colIndex);
		}

	/* If we get here then the mapping to mixed type will succeed */
	
	/* copy the incoming matrix and mapper */
	VecDatatypeMapperAndIndexSet mdm = datatypeMapperVec;
	const NxsDiscreteDatatypeMapper & oldMapper = mdm[0].first;
	if (oldMapper.GetUserDefinedEquatesBeforeConversion())
		return false; /* dealing with equates correctly is not implemented below, so we'll bale out */

	/* add the new mappers */
	std::map<char, NxsString> noEquates;
	datatypeMapperVec.clear();
	NxsDiscreteDatatypeMapper o(odt, origSymb, missing, gap, matchchar, respectingCase, noEquates);
	datatypeMapperVec.push_back(DatatypeMapperAndIndexSet(o, origTypeChars));
	NxsDiscreteDatatypeMapper s(NxsCharactersBlock::standard, augmentSymbols, missing, gap, matchchar, respectingCase, noEquates);
	datatypeMapperVec.push_back(DatatypeMapperAndIndexSet(s, stdTypeChars));
	
	
	NxsDiscreteDatatypeMapper & newOrigTMapper = datatypeMapperVec[0].first;
	NxsDiscreteDatatypeMapper & newStdTMapper = datatypeMapperVec[1].first;

	/* now we recode discrete matrix with new state codes */
	const int nOrigStates = (int) origSymb.size();
	std::map<int, int> oldToNewStateCode;
	NxsDiscreteStateMatrix::iterator rowIt = discreteMatrix.begin();
	for (unsigned colIndex = 0; rowIt != discreteMatrix.end(); ++colIndex, ++rowIt)
		{
		NxsDiscreteStateRow & row = *rowIt;
		unsigned column = 0;
		for (NxsDiscreteStateRow::iterator cell = row.begin(); cell != row.end(); ++cell, ++column)
			{
			const int initStateCode = *cell;
			if (initStateCode  >= 0 ) //gap and missing codes do not need translation 
				{
				std::map<int, int>::const_iterator otnIt = oldToNewStateCode.find(initStateCode);
				if (otnIt == oldToNewStateCode.end())
					{
					const bool isOrigT = origTypeChars.count(column) > 0;
					const std::set<int> oldSymbols = oldMapper.GetStateSetForCode(initStateCode);
					const std::string oldNexusString = oldMapper.StateCodeToNexusString(initStateCode);
					const char oldNexusChar = (oldNexusString.length() == 1 ? oldNexusString[0] : '\0');
					const bool isPoly =  oldMapper.IsPolymorphic(initStateCode);
					int newStateCode ;
					if (isOrigT)
						{ //old symbol indices will still be the new symbol indices 
						newStateCode = newOrigTMapper.StateCodeForStateSet(oldSymbols, isPoly, true, oldNexusChar);
						newOrigTMapper.StateCodeToNexusString(newStateCode);
						}
					else
						{
						std::set<int> transSymbols;
						for (std::set<int>::const_iterator sIt = oldSymbols.begin(); sIt != oldSymbols.end(); ++sIt)
							{
							if (*sIt >= nOrigStates)
								transSymbols.insert(*sIt - nOrigStates);
							else
								{
								assert(*sIt < 0);
								transSymbols.insert(*sIt);
								}	
							}
						newStateCode = newStdTMapper.StateCodeForStateSet(transSymbols, isPoly, true, oldNexusChar);
						newStdTMapper.StateCodeToNexusString(newStateCode);
						}
					oldToNewStateCode[initStateCode] = newStateCode;
					*cell = newStateCode;
					}
				else
					*cell = otnIt->second;
				}
			}
		}
	datatype = NxsCharactersBlock::mixed;
	return true;
	}
/*----------------------------------------------------------------------------------------------------------------------
|	Called when FORMAT command needs to be parsed from within the DIMENSIONS block. Deals with everything after the 
|	token FORMAT up to and including the semicolon that terminates the FORMAT command.
*/
void NxsCharactersBlock::HandleFormat(
  NxsToken &token)	/* the token used to read from `in' */
	{
	errormsg.clear();
	ProcessedNxsCommand tokenVec;
	token.ProcessAsCommand( &tokenVec);

	const ProcessedNxsCommand::const_iterator tvEnd = tokenVec.end();
	NxsPartition dtParts;
	std::vector<DataTypesEnum> dtv;
	std::vector<bool> isR;
	if (!datatypeReadFromFormat)
		{
		bool standardDataTypeAssumed = true;
		bool ignoreCaseAssumed = true;
		datatype = standard;
		originalDatatype = standard;
		ResetSymbols();
		respectingCase = false;
		restrictionDataype = false;
		for (ProcessedNxsCommand::const_iterator wIt = tokenVec.begin(); wIt != tvEnd; ++wIt)
			{
			if (wIt->Equals("DATATYPE"))
				{
				DemandEquals(wIt, tvEnd, " after keyword DATATYPE");
				ProcessedNxsToken::IncrementNotLast(wIt, tvEnd, " after \"DATATYPE =\" in FORMAT command");
				if (wIt->Equals("STANDARD"))
					{
					datatype = standard;
					symbols = "01";
					}
				else if (wIt->Equals("DNA"))
					datatype = dna;
				else if (wIt->Equals("RNA"))
					datatype = rna;
				else if (wIt->Equals("NUCLEOTIDE"))
					datatype = nucleotide;
				else if (wIt->Equals("PROTEIN"))
					datatype = protein;
				else if (wIt->Equals("RESTRICTION"))
					{
					datatype = standard;
					restrictionDataype = true;
					}
				else if (wIt->Equals("CONTINUOUS"))
					{
					datatype = continuous;
					statesFormat = INDIVIDUALS;
					items = std::vector<std::string>(1, std::string("AVERAGE"));
					tokens = true;
					}
				else if (supportMixedDatatype && wIt->Equals("MIXED"))
					{
					datatype = mixed;
					ProcessedNxsToken::IncrementNotLast(wIt, tvEnd, " after \"DATATYPE=MIXED\" in FORMAT command. Expecting (");
					if (!wIt->Equals("("))
						{
						errormsg << "Expecting ( after \"DATATYPE=MIXED\" but found " << wIt->GetToken();
						throw NxsException(errormsg, *wIt);
						}
					ProcessedNxsToken::IncrementNotLast(wIt, tvEnd, " after \"DATATYPE=MIXED(\" in FORMAT command. Expecting a datatype");
					ostringstream fakestream;
					while (!wIt->Equals(")"))
						{
						fakestream << ' ' << NxsString::GetEscaped(wIt->GetToken());
						ProcessedNxsToken::IncrementNotLast(wIt, tvEnd, " in \"DATATYPE=MIXED\" in FORMAT command. Expecting a closing ) to terminate the list.");
						}
					fakestream << ';';
					const std::string accumulated = fakestream.str();
					istringstream fakeinput(accumulated);
					NxsToken subToken(fakeinput);
					try
						{
						std::string mt("mixed datatype definition");
						subToken.GetNextToken();
						this->ReadPartitionDef(dtParts, *this, mt, "Character", "Datatype=Mixed", subToken, false, true);
						}
					catch (NxsException & x)
						{
						errormsg = x.msg;
						throw NxsException(errormsg, *wIt);
						}
					catch (...)
						{
						errormsg << "Error parsing \"DATATYPE=MIXED\" subcommand in FORMAT the command.";
						throw NxsException(errormsg, *wIt);
						}
					for (NxsPartition::const_iterator pIt = dtParts.begin(); pIt != dtParts.end(); ++pIt)
						{
						NxsString name(pIt->first.c_str());
						name.ToUpper();
						if (name == "RESTRICTION")
							{
							dtv.push_back(standard);
							isR.push_back(true);
							}
						else
							{
							isR.push_back(false);
							if (name == "STANDARD")
								dtv.push_back(standard);
							else if (name == "DNA")
								dtv.push_back(dna);
							else if (name == "RNA")
								dtv.push_back(rna);
							else if (name == "NUCLEOTIDE")
								dtv.push_back(nucleotide);
							else if (name == "PROTEIN")
								dtv.push_back(protein);
							else 
								{
								errormsg << pIt->first <<  " is not a valid DATATYPE within a " <<  id << " block";
								throw NxsException(errormsg, *wIt);
								}
							}
						}
					}
				else
					{
					errormsg << wIt->GetToken() <<  " is not a valid DATATYPE within a " <<  id << " block";
					throw NxsException(errormsg, *wIt);
					}
				datatypeReadFromFormat = true;
				originalDatatype = datatype;
				ResetSymbols();
				standardDataTypeAssumed = false;
				if (!ignoreCaseAssumed)
					break;
				}
			else if (wIt->Equals("RESPECTCASE"))
				{
				ignoreCaseAssumed = false;
				respectingCase = true;
				if (!standardDataTypeAssumed)
					break;
				}
			}
		}
	for (ProcessedNxsCommand::const_iterator wIt = tokenVec.begin(); wIt != tvEnd; ++wIt)
		{
		
		if (wIt->Equals("DATATYPE"))// we should have already processed this
			{
			ProcessedNxsToken::IncrementNotLast(wIt, tvEnd, "after DATATYPE in FORMAT command"); // = 
			ProcessedNxsToken::IncrementNotLast(wIt, tvEnd, "after DATATYPE = in FORMAT command"); // datatype
			}
		else if (wIt->Equals("RESPECTCASE"))
			{
			if (!respectingCase)
				{
				errormsg << "Only one FORMAT command should occur per DATA or CHARACTERS block.";
				throw NxsException(errormsg, *wIt);
				}
			}
		else if (wIt->Equals("MISSING"))
			{
			DemandEquals(wIt, tvEnd, "after keyword MISSING");
			ProcessedNxsToken::IncrementNotLast(wIt, tvEnd, "after \"MISSING = \" in FORMAT command");			
			const std::string t = wIt->GetToken();
			if (t.length() != 1)
				{
				errormsg << "MISSING symbol should be a single character, but " << t << " was specified";
				WarnDangerousContent(errormsg, *wIt);
				}
			else if (token.IsPunctuationToken(t) && !token.IsPlusMinusToken(t))
				{
				errormsg << "MISSING symbol specified cannot be a punctuation token (" << t << " was specified)";
				WarnDangerousContent(errormsg, *wIt);
				}
			else if (token.IsWhitespaceToken(t))
				{
				errormsg << "MISSING symbol specified cannot be a whitespace character (" << t << " was specified)";
				WarnDangerousContent(errormsg, *wIt);
				}
			missing = t[0];
			}
		else if (wIt->Equals("GAP"))
			{
			DemandEquals(wIt, tvEnd, "after keyword GAP");
			ProcessedNxsToken::IncrementNotLast(wIt, tvEnd, "after \"GAP = \" in FORMAT command");		
			const std::string t = wIt->GetToken();
			if (t.length() != 1)
				{
				errormsg << "GAP symbol should be a single character, but " << t << " was specified";
				WarnDangerousContent(errormsg, *wIt);
				}
			else if (token.IsPunctuationToken(t) && !token.IsPlusMinusToken(t))
				{
				errormsg << "GAP symbol specified cannot be a punctuation token " << t << " was specified";
				WarnDangerousContent(errormsg, *wIt);
				}
			else if (token.IsWhitespaceToken(t))
				{
				errormsg << "GAP symbol specified cannot be a whitespace character " << t << " was specified";
				WarnDangerousContent(errormsg, *wIt);
				}
			gap = t[0];
			}
		else if (wIt->Equals("MATCHCHAR"))
			{
			DemandEquals(wIt, tvEnd, "after keyword MATCHCHAR");
			ProcessedNxsToken::IncrementNotLast(wIt, tvEnd, "after \"MATCHCHAR = \" in FORMAT command");		
			const std::string t = wIt->GetToken();
			if (t.length() != 1)
				{
				errormsg << "MATCHCHAR symbol should be a single character, but " << t << " was specified";
				WarnDangerousContent(errormsg, *wIt);
				}
			else if (token.IsPunctuationToken(t) && !token.IsPlusMinusToken(t))
				{
				errormsg << "MATCHCHAR symbol specified cannot be a punctuation token (" << t << " was specified)";
				WarnDangerousContent(errormsg, *wIt);
				}
			else if (token.IsWhitespaceToken(t))
				{
				errormsg << "MATCHCHAR symbol specified cannot be a whitespace character (" << t << " was specified)";
				WarnDangerousContent(errormsg, *wIt);
				}
			matchchar = t[0];
			}
		else if (wIt->Equals("SYMBOLS") || wIt->Equals("SYMBOL"))
			{
			if (datatype == NxsCharactersBlock::continuous)
				throw NxsException("SYMBOLS subcommand not allowed for DATATYPE=CONTINUOUS", *wIt);
			if (restrictionDataype)
				throw NxsException("SYMBOLS subcommand not allowed for DATATYPE=RESTRICTION", *wIt);
			int numDefStates;
			unsigned maxNewStates = NCL_MAX_STATES;
			switch(datatype)
				{
				case NxsCharactersBlock::dna:
				case NxsCharactersBlock::rna:
				case NxsCharactersBlock::nucleotide:
					numDefStates = 4;
					maxNewStates = NCL_MAX_STATES-4;
					break;

				case NxsCharactersBlock::protein:
					numDefStates = 21;
					maxNewStates = NCL_MAX_STATES-21;
					break;

				default:
					numDefStates = 0; // replace symbols list for standard datatype
					symbols.clear();
					maxNewStates = NCL_MAX_STATES;
				}
			DemandEquals(wIt, tvEnd, "after keyword SYMBOLS");
			ProcessedNxsToken::IncrementNotLast(wIt, tvEnd, "\" to start the symbols list");
			if (!wIt->Equals("\""))
				{
				errormsg << "Expecting \" after Symbols= but " << wIt->GetToken() << " was found";
				throw NxsException(errormsg, *wIt);
				}
			ProcessedNxsToken::IncrementNotLast(wIt, tvEnd, "closing \" of symbols list");
			NxsString s;
			while (!wIt->Equals("\""))
				{
				s += wIt->GetToken().c_str();
				ProcessedNxsToken::IncrementNotLast(wIt, tvEnd, "closing \" of symbols list");
				}
				
			const std::string to = NxsString::strip_whitespace(s);
			unsigned tlen = (unsigned)to.length();
			if (tlen > maxNewStates)
				{
				errormsg << "SYMBOLS defines " << tlen << " new states but only " << maxNewStates << " new states allowed for this DATATYPE";
				throw NxsException(errormsg, *wIt);
				}
			// Check to make sure user has not used any symbols already in the
			// default symbols list for this data type
			//
			std::string preprocessedS;
			for (unsigned i = 0; i < tlen; i++)
				{
				if (to[i] == '~')
					{
					if (i == 0 || i == tlen -1)
						{
						errormsg << "A ~ in a SYMBOLS list is interpreted as a range of symbols.  The ~ cannot be the first or last character in the symbols list";
						throw NxsException(errormsg, token);
						}
					const char prevChar = to[i-1];
					const char nextChar = to[i+1];
					if ((isdigit(prevChar) && isdigit(nextChar)) || (isalpha(prevChar) && isalpha(nextChar)))
						{
						if (nextChar > prevChar)
							{
							for (char c = (char)((int)prevChar + 1) ; c < nextChar;)
								{
								preprocessedS.append(1, c);
								c = (char) ((int)c + 1);
								}
							}
						else
							{
							errormsg << "Endpoint of SYMBOLS range must be greater than the starting point.  This was not true of " << prevChar << '~' << nextChar;
							throw NxsException(errormsg, token);
							}
						}
					else
						{
						errormsg << prevChar << '~' << nextChar << " is an illegal SYMBOLS range. A range must go from a letter to a letter or from a number to number" ;
						throw NxsException(errormsg, token);
						}
					}
				else
					preprocessedS += to[i];
				}
			NxsString processedS;
			for (std::string::const_iterator pp = preprocessedS.begin(); pp != preprocessedS.end(); ++pp)
				{
				const char c = *pp;
				if (IsInSymbols(c))
					{
					errormsg << "The character " << c << " defined in SYMBOLS is predefined for this DATATYPE and should not occur in a SYMBOLS statement";
					if (nexusReader)
						{
						nexusReader->NexusWarnToken(errormsg, NxsReader::SKIPPING_CONTENT_WARNING, token);
						errormsg.clear();
						}
					}
				else if (   (respectingCase && (userEquates.find(c) != userEquates.end()))
						 || (! respectingCase && (userEquates.find(toupper(c)) != userEquates.end() || userEquates.find(tolower(*pp)) != userEquates.end())))
					{
					errormsg << "The character " << *pp << " defined in SYMBOLS subcommand, has already been introduced as an EQUATE key.  The use of a character as both a state symbol and an equate key is not allowed.";
					throw NxsException(errormsg, token);
					}
				else
					processedS += *pp;
				}

			// If we've made it this far, go ahead and add the user-defined
			// symbols to the end of the list of predefined symbols
			//
			symbols += processedS.c_str();
			}

		else if (wIt->Equals("EQUATE"))
			{
			if (datatype == NxsCharactersBlock::continuous)
				throw NxsException("EQUATE subcommand not allowed for DATATYPE=CONTINUOUS", *wIt);

			DemandEquals(wIt, tvEnd, "after keyword EQUATE");
			ProcessedNxsToken::IncrementNotLast(wIt, tvEnd, "\" to start the Equate definition");
			if (!wIt->Equals("\""))
				{
				errormsg << "Expecting '\"' after keyword EQUATE but found " << wIt->GetToken() << " instead";
				throw NxsException(errormsg, *wIt);
				}
			ProcessedNxsToken::IncrementNotLast(wIt, tvEnd, "\" to end the Equate definition");
			while (!wIt->Equals("\""))
				{
				std::string t = wIt->GetToken();
				if (t.length() != 1)
					{
					errormsg << "Expecting single-character EQUATE symbol but found " << wIt->GetToken() << " instead";
					throw NxsException(errormsg, *wIt);
					}
				const char ch = t[0];
				bool badEquateSymbol = false;

				// Equate symbols cannot be punctuation (except for + and -)
				//
				if (token.IsPunctuationToken(t) && !token.IsPlusMinusToken(t))
					badEquateSymbol = true;
				else if (ch == '^')
					badEquateSymbol = true;
				if (badEquateSymbol)
					{
					errormsg << "EQUATE symbol specified (" << wIt->GetToken() <<  ") is not valid. Equate symbols cannot be any of the following: ()[]{}/\\,;:=*'\"`<>^";
					WarnDangerousContent(errormsg, *wIt);
					}
				if (ch == missing || ch == matchchar || ch == gap || IsInSymbols(ch))
					{
					errormsg << "EQUATE symbol specified (" << wIt->GetToken() <<  ") is not valid; An Equate symbol cannot be a state symbol or identical to the  missing,  gap, or matchchar symbols.";
					throw NxsException(errormsg, *wIt);
					}
				
				DemandEquals(wIt, tvEnd, " in EQUATE definition");
				ProcessedNxsToken::IncrementNotLast(wIt, tvEnd, "State or set of states in Equate definition");
				NxsString s;
				s = wIt->GetToken().c_str();
				if (wIt->Equals("{"))
					{
					while (!wIt->Equals("}"))
						{
						ProcessedNxsToken::IncrementNotLast(wIt, tvEnd, "} to close the state set in an equate definition");
						s += wIt->GetToken().c_str();
						}
					}
				else if (wIt->Equals("("))
					{
					while (!wIt->Equals(")"))
						{
						ProcessedNxsToken::IncrementNotLast(wIt, tvEnd, ") to close the state set in an equate definition");
						s += wIt->GetToken().c_str();
						}
					}
				const std::string nows = NxsString::strip_whitespace(s);
				userEquates[ch] = NxsString(nows.c_str());
				ProcessedNxsToken::IncrementNotLast(wIt, tvEnd, "\" to end the Equate definition");
				}
			}

		else if (wIt->Equals("LABELS"))
			labels = true;
		else if (wIt->Equals("NOLABELS"))
			labels = false;
		else if (wIt->Equals("TRANSPOSE"))
			transposing = true;
		else if (wIt->Equals("INTERLEAVE"))
			interleaving = true;
		else if (wIt->Equals("ITEMS"))
			{
			DemandEquals(wIt, tvEnd, "after keyword ITEMS");
			items.clear();
			// This should be STATES (no other item is supported at this time)
			//
			ProcessedNxsToken::IncrementNotLast(wIt, tvEnd, "after \"ITEMS =\" in FORMAT command");
			if (datatype == NxsCharactersBlock::continuous)
				{
				std::string s;
				if (wIt->Equals("("))
					{
					ProcessedNxsToken::IncrementNotLast(wIt, tvEnd, ") to close Items list in FORMAT command");
					while (!wIt->Equals(")"))
						{
						s = wIt->GetToken();
						NxsString::to_upper(s);
						items.push_back(std::string(s.c_str()));
						ProcessedNxsToken::IncrementNotLast(wIt, tvEnd, ") to close Items list in FORMAT command");
						}
					}
				else
					{
					s = wIt->GetToken();
					NxsString::to_upper(s);
					items.push_back(std::string(s.c_str()));
					}
				}
			else 
				{
				if (!wIt->Equals("STATES"))
					throw NxsException("Sorry, only ITEMS=STATES is supported for discrete datatypes at this time", *wIt);
				items = std::vector<std::string>(1, std::string("STATES"));
				}
			}
		else if (wIt->Equals("STATESFORMAT"))
			{
			DemandEquals(wIt, tvEnd, "after keyword STATESFORMAT");
			ProcessedNxsToken::IncrementNotLast(wIt, tvEnd, "after \"STATESFORMAT =\" in FORMAT command");
			if (wIt->Equals("STATESPRESENT"))
				statesFormat = STATES_PRESENT;
			else
				{
				if (datatype == NxsCharactersBlock::continuous)
					{
					if (wIt->Equals("INDIVIDUALS"))
						statesFormat = INDIVIDUALS;
					else
						throw NxsException("Sorry, only STATESFORMAT=STATESPRESENT or STATESFORMAT=INDIVIDUALS are supported for continuous datatypes at this time", *wIt);
					}
				else
					throw NxsException("Sorry, only STATESFORMAT=STATESPRESENT supported for discrete datatypes at this time", *wIt);
				}
			}
		else if (wIt->Equals("TOKENS"))
			tokens = true;
		else if (wIt->Equals("NOTOKENS"))
			{
			if (datatype == NxsCharactersBlock::continuous)
				throw NxsException("NOTOKENS is not allowed for the CONTINUOUS datatype", *wIt);
			tokens = false;
			}
		}
	if (matchchar != '\0')
		{
		if ((matchchar == gap) || (!respectingCase && toupper(matchchar) == toupper(gap)))
			{
			errormsg << "MatchChar and Gap symbol cannot be identical!  Both were set to " << gap;
			throw NxsException(errormsg, *tokenVec.begin());
			}
		if ((matchchar == missing) || (!respectingCase && toupper(matchchar) == toupper(missing)))
			{
			errormsg << "MatchChar and Missing symbol cannot be identical!  Both were set to " << missing;
			throw NxsException(errormsg, *tokenVec.begin());
			}
		}
	if ((gap != '\0') && ((gap == missing) || (!respectingCase && toupper(gap) == toupper(missing))))
		{
		errormsg << "Gap symbol and Missing symbol cannot be identical!  Both were set to " << missing;
		throw NxsException(errormsg, *tokenVec.begin());
		}
	
	// Perform some last checks before leaving the FORMAT command
	//
	if (!tokens && datatype == continuous)
		GenerateNxsException(token, "TOKENS must be defined for DATATYPE=CONTINUOUS");
	if (tokens && (datatype == dna || datatype == rna || datatype == nucleotide))
		GenerateNxsException(token, "TOKENS not allowed for the DATATYPEs DNA, RNA, or NUCLEOTIDE");
	CreateDatatypeMapperObjects(dtParts, dtv);
	if (IsMixedType() && tokens)
		{
		errormsg = "The combination of DATATYPE=Mixed  and TOKENS are not currently supported.";
		throw NxsException(errormsg, token.GetFilePosition(), token.GetFileLine(), token.GetFileColumn());
		}
	unsigned mapInd = 0;
	for (std::vector<bool>::const_iterator b = isR.begin(); b != isR.end(); ++b, ++mapInd)
		{
		if (*b)
			{
			DatatypeMapperAndIndexSet &mapper = datatypeMapperVec.at(mapInd);
			mapper.first.SetWasRestrictionDataype(true);
			}
		}
	}


NxsDiscreteDatatypeMapper::NxsDiscreteDatatypeMapper()
	:datatype(NxsCharactersBlock::standard),
	restrictionDataype(false),
	userDefinedEquatesBeforeConversion(false)
	{
	symbols.assign("01");
	matchChar = '\0';
	gapChar = '\0';
	missing = '?';
	respectCase = false;
	RefreshMappings(0L);
	}

NxsDiscreteDatatypeMapper & NxsDiscreteDatatypeMapper::operator=(const NxsDiscreteDatatypeMapper& other)
	{
	symbols = other.symbols; 
	lcsymbols = other.lcsymbols;
	nStates = other.nStates;
	matchChar = other.matchChar;
	gapChar = other.gapChar;
	missing = other.missing;
	respectCase = other.respectCase;
	extraEquates = other.extraEquates;
	extraEquates = other.extraEquates;
	datatype = other.datatype;
	sclOffset = other.sclOffset;
	stateSetsVec = other.stateSetsVec;
	stateCodeLookupPtr = &stateSetsVec[-sclOffset];
	charToStateCodeLookup = other.charToStateCodeLookup;
	cLookup = &charToStateCodeLookup[127];
	restrictionDataype = other.restrictionDataype;
	userDefinedEquatesBeforeConversion = other.userDefinedEquatesBeforeConversion;
	return *this;
	}

/*----------------------------------------------------------------------------------------------------------------------
|	Must be called when the symbols list changes. 
|	Uses symbols, gap, missing, respectCase,  extraEquates, and datatype fields to establish new mappings.
|	token can be NULL if the call is not triggered by the reading of a NEXUS token.
*/
void NxsDiscreteDatatypeMapper::RefreshMappings(NxsToken *token)
	{
	nStates = (unsigned)symbols.length();
	if (nStates ==  0)
		{
		if (datatype != NxsCharactersBlock::continuous)
			throw NxsException("Cannot create a datatype mapper with no symbols");
		return;
		}
	if (!respectCase)
		{
		NxsString::to_upper(symbols);
		lcsymbols = symbols;
		}
	else
		lcsymbols.clear();

	NxsString::to_lower(lcsymbols);

	if (missing == '\0')
		throw NxsException("Cannot create a datatype mapper with no missing data symbol");
	
	charToStateCodeLookup.assign(384, NXS_INVALID_STATE_CODE); /*256+128 = 384 -- this way we can deal with signed or unsigned chars by pointing cLookup to element 128*/
	cLookup = &charToStateCodeLookup[127];
	stateSetsVec.clear();
	stateCodeLookupPtr = 0L;
	sclOffset = (gapChar == '\0' ? -1 : -2);
	
	std::string bogus;
	std::istringstream bogusStream(bogus);
	NxsToken bogusToken(bogusStream);
	token = (token == NULL ? &bogusToken : token);
	
	/* add the "fundamental" states. */
	std::set<int> stSet;
	std::set<int> missingSet;
	if (gapChar != '\0')
		{
		stSet.insert(NXS_GAP_STATE_CODE);
		/* this is the one of only 2 times that  we don't call AddStateSet to add a state set 
			we do this to avoid illegal indexing of stateSets[1] when there
			is only one element in the vector.
		*/
		stateSetsVec.push_back(NxsDiscreteStateSetInfo(stSet, false, gapChar));
		cLookup[(int) gapChar] = NXS_GAP_STATE_CODE;
		
		missingSet.insert(NXS_GAP_STATE_CODE);
		}
	
	
		/* 
			Add the missing state code
			this is the other time that we don't call AddStateSet (to avoid illegal indexing).
		*/
	assert(missing != '\0');
	assert(nStates > 0);
	for (int s = 0; s < (int) nStates; ++s)
		missingSet.insert(s);
	
	char sym = (respectCase ? missing : (char) toupper(missing));
	stateSetsVec.push_back(NxsDiscreteStateSetInfo(missingSet, false, sym));
	const int stateCode = (const int)stateSetsVec.size() + sclOffset - 1;
	assert(NXS_MISSING_CODE == stateCode);
	if (respectCase)
		cLookup[(int) missing] = stateCode;
	else
		{
		cLookup[(int) tolower(missing)] = stateCode;
		cLookup[(int) toupper(missing)] = stateCode;
		}
	assert(cLookup[(int) missing] == NXS_MISSING_CODE);
	for (int s = 0; s < (int) nStates; ++s)
		{
		stSet.clear();
		stSet.insert(s);
		AddStateSet(stSet, symbols[s], respectCase, false);
		}
	
	/* add the default equates */
	std::map<char, NxsString> defEq = NxsCharactersBlock::GetDefaultEquates(datatype);

	bool convertToStandard = false;
	if (((datatype == NxsCharactersBlock::nucleotide) || (datatype == NxsCharactersBlock::dna)) && symbols != "ACGT")
		convertToStandard = true;
	else if ((datatype == NxsCharactersBlock::rna) && symbols != "ACGU")
		convertToStandard = true;
	else if ((datatype == NxsCharactersBlock::protein) && symbols != "ACDEFGHIKLMNPQRSTVWY*")
		convertToStandard = true;
	if (convertToStandard)
		{
		if (!extraEquates.empty())
			userDefinedEquatesBeforeConversion = true;
		defEq.insert(extraEquates.begin(), extraEquates.end());
		extraEquates.clear();
		defEq.swap(extraEquates);
		/* respectcase is only "applicable" to Standard datatype 
			Any symbol extension will be at the end of the symbols list, 
			so here we add the lower case symbols as equates.
		*/
		if (respectCase)
			{
			std::string lcsym = NxsCharactersBlock::GetDefaultSymbolsForType(datatype);
			NxsString::to_lower(lcsym);
			std::string ucsym = lcsym;
			NxsString::to_upper(ucsym);
			for (unsigned i = 0; i < ucsym.length(); ++i)
				{
				if (ucsym[i] != lcsym[i])
					{
					NxsString u;
					u.append(1, ucsym[i]);
					extraEquates[lcsym[i]] = u;
					}
				}
			}
		datatype =  NxsCharactersBlock::standard;
		}


	std::map<char, NxsString>::const_iterator eqIt = defEq.begin();
	NxsString taxonName;
	for (; eqIt != defEq.end(); ++eqIt)
		{
		const char c = eqIt->first;
		const char u = toupper(c);
		bool addEq = true;
		if (c == missing || c == matchChar || c == gapChar)
			addEq = false;
		if (!respectCase && (u == toupper(missing) || u == toupper(matchChar) || u == toupper(gapChar)))
			addEq = false;
		if (addEq)
			{
			const NxsString & s = eqIt->second;
			int sc = StateCodeForNexusPossibleMultiStateSet(c, s, *token, UINT_MAX, UINT_MAX, 0L, taxonName);
			cLookup[(int) c] = sc;
			}
		}
	

	/* add user-defined equates, and only retain the new ones (those that are not datatype defaults). */
	std::map<char, NxsString> neededExtraEquates;
	for (eqIt = extraEquates.begin(); eqIt != extraEquates.end(); ++eqIt)
		{
		const char c = eqIt->first;
		const char u = toupper(c);
		if (PositionInSymbols(c) == NXS_INVALID_STATE_CODE)
			{
			bool addEq = true;
			if (c == missing || c == matchChar || c == gapChar)
				addEq = false;
			if (!respectCase && (u == toupper(missing) || u == toupper(matchChar) || u == toupper(gapChar)))
				addEq = false;
			if (addEq)
				{
				const int prevCode = cLookup[(int) c];
				const NxsString & s = eqIt->second;
				int sc = StateCodeForNexusPossibleMultiStateSet(c, s, *token, UINT_MAX, UINT_MAX, 0L, taxonName);
				cLookup[(int) c] = sc;
				if (sc != prevCode) /* the equate was new */
					neededExtraEquates[c] = s;
				}
			}
		else
			{
			assert(convertToStandard); // a equate key that is equal to a symbol can happen if the symbols list is augmented (resulting in a conversion to standard datatype)
			}
		}
	extraEquates = neededExtraEquates;
	}   

/*----------------------------------------------------------------------------------------------------------------------
|	Returns the state code of a (possible new state set) `sset`.  This may trigger the reallocation of mapping info.
|	nexusSymbol can be '\0' if there is not a single-character symbol that represents this state set.
|
|	if `addToLookup` is false and the state set is not found then NXS_INVALID_STATE_CODE will be returned.
|	
|	if the stateset is added with a `nexusSymbol` then the new "symbol" will be case-sensitive 
|	(this is an mechanism for entering equates and equates are always case sensitive).
|	
|	New "fundamental" states can NOT be introduced using this function -- if unknown states are encountered, an exception will be generated.
*/
int NxsDiscreteDatatypeMapper::StateCodeForStateSet(const std::set<int> & sset, bool isPolymorphic, bool addToLookup, char nexusSymbol)
	{
	if (sset.size() == 1)
		{
		int c = *sset.begin();
		ValidateStateIndex(c);
		return c;
		}
	assert(stateCodeLookupPtr); 
	NxsDiscreteStateSetInfo *sclStart = stateCodeLookupPtr + nStates;
	const int nCodes = (int)stateSetsVec.size();

	/*we can start at nStates, because < nStates will be handled in the sset.size() == 1 above */
	for (int i = nStates - sclOffset; i < nCodes; ++i) 
		{
		NxsDiscreteStateSetInfo & stateSetInfo = *sclStart++;
		if (sset == stateSetInfo.states && isPolymorphic == stateSetInfo.isPolymorphic)
			return i + sclOffset;
		}
	if (!addToLookup)
		return NXS_INVALID_STATE_CODE;
	return AddStateSet(sset, nexusSymbol, true, isPolymorphic); 
	}

/*----------------------------------------------------------------------------------------------------------------------
|	Adds a new state set and returns its code. 
|	Does NOT check if the state set is present.
|	It is also MANDATORY that this function be called with the fundamental states first (and in order) before
|	 being called with any multi state sets (this is done by RefreshMappings)
*/
int NxsDiscreteDatatypeMapper::AddStateSet(const std::set<int> & states, char nexusSymbol, bool symRespectCase, bool isPolymorphic)
	{
	bool reallyIsPoly = (states.size() > 1 && isPolymorphic);
	char sym = (symRespectCase ? nexusSymbol : (char) toupper(nexusSymbol));
	stateSetsVec.push_back(NxsDiscreteStateSetInfo(states, reallyIsPoly, sym));
	/* if we have gaps, then the sclOffset is -1 and we want to enable 
		stateCodeLookup[-1], so we set stateCodeLookup to &stateSets[1]
		hence the -sclOffset below
	*/
	stateCodeLookupPtr = &stateSetsVec[-sclOffset];

	const int stateCode = (const int)stateSetsVec.size() + sclOffset - 1;
	if (nexusSymbol != '\0')
		{
		if (symRespectCase)
			cLookup[(int) nexusSymbol] = stateCode;
		else
			{
			cLookup[(int) tolower(nexusSymbol)] = stateCode;
			cLookup[(int) toupper(nexusSymbol)] = stateCode;
			}
		}
	return stateCode;
	}



/*----------------------------------------------------------------------------------------------------------------------
|	Throws an NxsNCLAPIException  if `c` is not a valid index of one of the "fundamental" states for the datatype.
*/
void NxsDiscreteDatatypeMapper::ValidateStateIndex(int c) const
	{
	if (c < NXS_MISSING_CODE)
		{
		if (c == NXS_GAP_STATE_CODE)
			{
			if (gapChar == '\0')
				throw NxsNCLAPIException("Illegal usage of NXS_GAP_STATE_CODE in a datatype without gaps");
			return;
			}
		if (c == NXS_INVALID_STATE_CODE)
			throw NxsNCLAPIException("Illegal usage of NXS_INVALID_STATE_CODE as a state index");
		throw NxsNCLAPIException("Illegal usage of unknown negative state index");
		}
	else if (c >= (int) nStates)
		throw NxsNCLAPIException("Illegal usage of state index >= the number of states");
	}	

/*----------------------------------------------------------------------------------------------------------------------
|	Throws an NxsNCLAPIException  if `c` is not a valid state code.
*/
void NxsDiscreteDatatypeMapper::ValidateStateCode(int c) const 
	{
	if (c < sclOffset)
		{
		if (c == NXS_GAP_STATE_CODE)
			throw NxsNCLAPIException("Illegal usage of NXS_GAP_STATE_CODE in a datatype without gaps");
		if (c == NXS_INVALID_STATE_CODE)
			throw NxsNCLAPIException("Illegal usage of NXS_INVALID_STATE_CODE as a state code");
		throw NxsNCLAPIException("Illegal usage of unknown negative state index");
		}
	else if (c >= (((int) stateSetsVec.size()) + sclOffset))
		throw NxsNCLAPIException("Illegal usage of state code > the highest state code");
	}	

	
void NxsDiscreteDatatypeMapper::GenerateNxsExceptionMatrixReading(char const* message, unsigned int taxInd, unsigned int charInd, 
NxsToken& token, const NxsString &nameStr)
	{
	NxsString e = "Error reading character ";
	e << charInd + 1<<" for taxon " << taxInd + 1;
	if (!nameStr.empty())
		{
		NxsString nasn;
		nasn << taxInd + 1;
		if (nasn != nameStr)
			e << " (name \""<< nameStr <<"\")";
		}
	e << ":\n" << message;
	throw NxsException(e, token);
	}
	
/*----------------------------------------------------------------------------------------------------------------------
|	Returns true if the state code maps to a collection of states that were flagged as polymorphic.
|	generates a NxsNCLAPIException if `c` is not a valid state code
*/
bool NxsDiscreteDatatypeMapper::IsPolymorphic(int c) const
	{
	assert(stateCodeLookupPtr);
	ValidateStateCode(c);
	return stateCodeLookupPtr[c].isPolymorphic;
	}

	
/*----------------------------------------------------------------------------------------------------------------------
|	Returns NXS_INVALID_STATE_CODE or the index of `c` in the symbols list.
|	case-sensitivity is controlled by this->respectCase attribute.
|	
|	NOTE: the gap "state" and missing characters are NOT in the symbols list.
*/
int NxsDiscreteDatatypeMapper::PositionInSymbols(char c) const
	{
	int p = (int)symbols.find(c);
	if (p >= 0 && p < (int) nStates)
		return p;
	if (!respectCase)
		{
		p = (int)lcsymbols.find(c);
		if (p >= 0 && p < (int) nStates)
			return p;
		}
	return NXS_INVALID_STATE_CODE;
	}
	

/*----------------------------------------------------------------------------------------------------------------------
|	Returns the NEXUS reperesenation of the state code `scode` which may be a multiple character string such as {DNY}
|   Generates a NxsNCLAPIException if `c` is not a valid state code
*/
void NxsDiscreteDatatypeMapper::WriteStateCodeAsNexusString(std::ostream & out, int scode) const
	{
	ValidateStateCode(scode);
	const NxsDiscreteStateSetInfo * ssi = &(stateSetsVec.at(scode-sclOffset)); 
	const NxsDiscreteStateSetInfo & stateSetInfo =  stateCodeLookupPtr[scode];
	assert (ssi == &stateSetInfo);
	char c = stateSetInfo.nexusSymbol;
	if (c != '\0')
		{
		out << c;
		return;
		}
	out <<	(stateSetInfo.isPolymorphic ? '(' : '{');
	std::set<int>::const_iterator sIt = stateSetInfo.states.begin();
	const std::set<int>::const_iterator endIt = stateSetInfo.states.end();
	for (; sIt != endIt; ++sIt)
		{
		const int state = *sIt;
		const NxsDiscreteStateSetInfo & subStateSetInfo =  stateCodeLookupPtr[state];
		const char subc = subStateSetInfo.nexusSymbol;
		assert (subc != '\0');
		out << subc;
		}
	out <<	(stateSetInfo.isPolymorphic ? ')' : '}');
	}

unsigned NxsDiscreteDatatypeMapper::GetNumStatesInStateCode(int scode) const
	{
	ValidateStateCode(scode);
	const NxsDiscreteStateSetInfo & stateSetInfo =  stateCodeLookupPtr[scode];
	return (unsigned)stateSetInfo.states.size();
	}

void NxsDiscreteDatatypeMapper::WriteStartOfFormatCommand(std::ostream & out) const
	{
	out << "    FORMAT Datatype=" << NxsCharactersBlock::GetNameOfDatatype(datatype);
	if (this->missing != '?')
		{
		out << " Missing=";
		out << this->missing;
		}
	if (this->gapChar != '\0')
		{
		out << "  Gap=";
		out << this->gapChar;
		}
	if (this->datatype != NxsCharactersBlock::continuous)
		{
		unsigned numDefStates = 4;
		if (this->datatype == NxsCharactersBlock::protein)
			numDefStates = 21;
		else if (this->datatype == NxsCharactersBlock::standard)	
			numDefStates = 0;
		unsigned nSym = (unsigned)this->symbols.length();
		if (nSym > numDefStates)
			{
			out << " Symbols=\"";
			for (unsigned i = numDefStates; i < nSym; ++i)
				{
				char c = symbols[i];
				if (c == '\0')
					break;
				out << c;
				}
			out <<"\"";
			}
		}
	const std::map<char, NxsString> defEquates = NxsCharactersBlock::GetDefaultEquates(datatype);
	std::map<char, NxsString> toWrite;
	const std::map<char, NxsString>::const_iterator notFound = defEquates.end();
	std::map<char, NxsString>::const_iterator inDefEquates;
	for (std::map<char, NxsString>::const_iterator i = extraEquates.begin(); i != extraEquates.end(); ++i)
		{
		const char key =  (*i).first;
		const NxsString val =  i->second;
		inDefEquates = defEquates.find(key);
		if (inDefEquates == notFound || inDefEquates->second != val)
			toWrite[key] = val;
		}
	if (toWrite.size() > 0)
		{
		out << " Equate=\"";
		for (std::map<char, NxsString>::const_iterator j = toWrite.begin(); j != toWrite.end(); ++j)
			out << ' ' << j->first << '=' << j->second;
		out <<"\"";
		}
	}

bool NxsCharactersBlock::HandleNextContinuousState(NxsToken &token, unsigned taxNum, unsigned charNum, ContinuousCharRow & row, const NxsString & )
	{
	if (interleaving)
		token.SetLabileFlagBit(NxsToken::newlineIsToken);
	token.SetLabileFlagBit(NxsToken::hyphenNotPunctuation);
	std::vector<double> v;
	std::vector<int> scored;
	token.GetNextToken();
	NxsString t;
	if (interleaving && token.AtEOL())
		return false;
	if (token.Equals("("))
		{
		token.SetLabileFlagBit(NxsToken::hyphenNotPunctuation);		   
		token.GetNextToken();
		while (!token.Equals(")"))
			{
			t = token.GetToken();
			if (t.length() == 1 && (t[0] == missing || t[0] == gap))
				{
				v.push_back(DBL_MAX);
				scored.push_back(0);
				}
			else if (t.length() == 1 && t[0] == matchchar)
				{
				v.push_back(DBL_MAX);
				scored.push_back(2);
				}
			else if (!t.IsADouble())
				GenerateUnexpectedTokenNxsException(token, "a number");
			else
				{
				v.push_back(t.ConvertToDouble());
				scored.push_back(1);
				}
			token.SetLabileFlagBit(NxsToken::hyphenNotPunctuation);		   
			token.GetNextToken();
			}
		}
	else
		{
		t = token.GetToken();
		if (t.length() == 1 && (t[0] == missing || t[0] == gap))
			{
			v.push_back(DBL_MAX);
			scored.push_back(0);
			}
		else if (t.length() == 1 && t[0] == matchchar)
			{
			v.push_back(DBL_MAX);
			scored.push_back(2);
			}
		else if (!t.IsADouble())
			GenerateUnexpectedTokenNxsException(token, "a number");
		else
			{
			v.push_back(t.ConvertToDouble());
			scored.push_back(1);
			}
		}
	unsigned n_read = (unsigned)v.size();
	if (n_read < items.size())
		{
		errormsg.clear();
		errormsg << "For each cell of the MATRIX a value for each of the " << (unsigned)items.size() <<  " ITEMS listed in the FORMAT command is expected.\nOnly " <<  n_read << " values read.";
		GenerateNxsException(token);
		}
	// We've read in the state now, so if this character has been eliminated, we don't want to go any further with it
	//
	if (charNum == UINT_MAX)
		return true;

	if (charNum > row.size())
		GenerateNxsException(token, "Internal Error: character index out of range in continuousMatrix.");

	ContinuousCharCell & cell = row[charNum];
	cell.clear();

	std::vector<std::string >::const_iterator itemIt = items.begin();
	std::string key;
	unsigned curr_ind_in_v = 0;
	for (; itemIt != items.end(); ++itemIt, ++curr_ind_in_v)
		{
		key = *itemIt;
		if (scored[curr_ind_in_v] == 1)
			cell[key] = vector<double>(1, v[curr_ind_in_v]);
		else if (scored[curr_ind_in_v] == 0)
			cell[key] = vector<double>();
		else
			{
			if (taxNum == 0)
				GenerateNxsException(token, "MATCHCHAR cannot be used in the first taxon");
			const vector<double> & first_taxon_vector = continuousMatrix[0][charNum][key];
			if (first_taxon_vector.empty())
				GenerateNxsException(token, "First taxon does not have a value to copy, but a MATCHCHAR was found.");
			else
				cell[key] = vector<double>(1, first_taxon_vector[0]);
			}
		}
	unsigned curr_ind_mapped = 1;
	if (!key.empty() && curr_ind_in_v < n_read)
		{
		vector<double> & curr_cell_vector = cell[key];
		for (; curr_ind_in_v < n_read; ++curr_ind_in_v, ++curr_ind_mapped)
			{
			if (scored[curr_ind_in_v] == 1)
				curr_cell_vector.push_back(v[curr_ind_in_v]);
			else if (scored[curr_ind_in_v] != 0)
				curr_cell_vector.push_back(DBL_MAX);
			else
				{
				if (taxNum == 0)
					GenerateNxsException(token, "MATCHCHAR cannot be used in the first taxon");
				const vector<double> & first_taxon_vector = continuousMatrix[0][charNum][key];
				if (first_taxon_vector.size() < curr_ind_mapped+1)
					GenerateNxsException(token, "First taxon does not have a value to copy, but a MATCHCHAR was found.");
				else
					curr_cell_vector.push_back(first_taxon_vector[curr_ind_mapped]);
				}
			}
		}
	return true;
	}

int NxsDiscreteDatatypeMapper::StateCodeForNexusChar(
  const char currChar,
  NxsToken &token,
  unsigned taxNum,		/* the taxon index, in range [0..`ntax') */
  unsigned charNum,		/* the character index, in range [0..`nChar') */
  const NxsDiscreteStateRow * firstTaxonRow, 
  const NxsString & nameStr) const
  	{
  	int currState = cLookup[currChar];
	if (currState == NXS_INVALID_STATE_CODE)
		{
		NxsString emsg;
		if (currChar == matchChar)
			{
			if (firstTaxonRow == NULL)
				GenerateNxsExceptionMatrixReading("Unexpected use of MatchChar in first taxon with data.", taxNum, charNum, token, nameStr);
			if (firstTaxonRow->size() <= charNum)
				{
				emsg << "MatchChar found for character number "  << charNum+1 << " but the first taxon does not have a character state stored for this character.";
				GenerateNxsExceptionMatrixReading(emsg, taxNum, charNum, token, nameStr);
				}
			currState = (*firstTaxonRow)[charNum];
			}
		else
			{
			emsg << "Invalid state specified \"" << token.GetToken() << "\"";
			GenerateNxsExceptionMatrixReading(emsg, taxNum, charNum, token, nameStr);
			}
		}
	return currState;
	}

bool NxsCharactersBlock::HandleNextDiscreteState(
  NxsToken &token, 
  unsigned taxNum, 
  unsigned charNum, 
  NxsDiscreteStateRow & row, 
  NxsDiscreteDatatypeMapper &mapper, 
  const NxsDiscreteStateRow * firstTaxonRow, 
  const NxsString & nameStr)
  	{
  	if (interleaving)
		token.SetLabileFlagBit(NxsToken::newlineIsToken);
	assert(!tokens);
	token.SetLabileFlagBit(NxsToken::parentheticalToken);
	token.SetLabileFlagBit(NxsToken::curlyBracketedToken);
	token.SetLabileFlagBit(NxsToken::singleCharacterToken);

	token.GetNextToken();

	if (interleaving && token.AtEOL())
		return false;
	const NxsString &stateAsNexus = token.GetTokenReference();
	int sc =  mapper.EncodeNexusStateString(stateAsNexus, token, taxNum, charNum, firstTaxonRow, nameStr);
	assert(charNum < row.size());
	row[charNum] = sc; 
	return true;
	}

int NxsDiscreteDatatypeMapper::StateCodeForNexusPossibleMultiStateSet(
  const char nexusSymbol,
  const std::string &stateAsNexus,
  NxsToken & token,	/* the token used to read from `in' */
  const unsigned taxNum,		/* the taxon index, in range [0..`ntax') */
  const unsigned charNum,		/* the character index, in range [0..`nChar') */
  const NxsDiscreteStateRow * firstTaxonRow, const NxsString &nameStr)
	{
	assert(stateAsNexus.length() > 0);
	const char firstChar = stateAsNexus[0];
	if (firstChar == '(' || firstChar == '{')
		return StateCodeForNexusMultiStateSet(nexusSymbol, stateAsNexus, token, taxNum, charNum, firstTaxonRow, nameStr);
	if (stateAsNexus.length() > 1)
		{
		NxsString emsg;
		emsg << "Expecting  {} or () around a multiple character state set.  Found " << stateAsNexus << " for taxon " << nameStr;
		GenerateNxsExceptionMatrixReading(emsg, taxNum, charNum, token, nameStr);
		}
		
	int currState = StateCodeForNexusChar(firstChar, token, taxNum, charNum, firstTaxonRow, nameStr);
	cLookup[(int) nexusSymbol] = currState;
	return currState;
	}

int NxsDiscreteDatatypeMapper::StateCodeForNexusMultiStateSet(
  const char nexusSymbol,
  const std::string &stateAsNexus,
  NxsToken & token,	/* the token used to read from `in' */
  const unsigned taxNum,		/* the taxon index, in range [0..`ntax') */
  const unsigned charNum,		/* the character index, in range [0..`nChar') */
  const NxsDiscreteStateRow * firstTaxonRow,
  const NxsString &nameStr)
  	{
	const char firstChar = stateAsNexus[0];
	NxsString emsg;
	const bool poly = (firstChar == '(');
	assert(poly || firstChar == '{');
	bool tildeFound = false;
	int prevState = NXS_INVALID_STATE_CODE;
	char prevChar = firstChar;
	std::string::const_iterator cIt = stateAsNexus.begin();
	std::string::const_iterator endIt = stateAsNexus.end();
	--endIt;
	assert((poly && *endIt == ')') || (!poly && *endIt == '}'));
	std::set<int> sset;
	for (++cIt; cIt != endIt; ++cIt)
		{
		const char currChar = *cIt;
		if ((strchr("\n\r \t", currChar) == NULL) && currChar != ',')
			{
			if (currChar == '~')
				{				
				if (prevState < 0 || prevState >= (int)nStates)
					{
					emsg << "A state range cannot start with " << prevChar;
					GenerateNxsExceptionMatrixReading(emsg, taxNum, charNum, token, nameStr);
					}
				tildeFound = true;
				}
			else
				{
				// Add state symbol and record if it is the first or last one in case we encounter a tilde
				int currState;
				if (tildeFound)
					{
					currState = PositionInSymbols(currChar);
					if (currState == NXS_INVALID_STATE_CODE)
						{
						emsg << "A state range cannot end with " << currChar;
						GenerateNxsExceptionMatrixReading(emsg, taxNum, charNum, token, nameStr);
						}
					if (currState < prevState)
						{
						emsg << prevChar << '~' << currChar << " is not a valid state range (the end state is a lower index than the start)";
						GenerateNxsExceptionMatrixReading(emsg, taxNum, charNum, token, nameStr);
						}
					for (int i = prevState; i <= currState; ++i)
						sset.insert(i);
					tildeFound = false;
					}
				else
					{
					currState = StateCodeForNexusChar(currChar, token, taxNum, charNum, firstTaxonRow, nameStr);
					sset.insert(currState);
					}
				prevState = currState;
				prevChar = currChar;
				} 
			}
		}
	if (prevChar == '~')
		{
		emsg << "State range not terminated -- ending in ~" << *endIt;
		GenerateNxsExceptionMatrixReading(emsg, taxNum, charNum, token, nameStr);
		}
	if (sset.empty())
		{
		emsg << "An illegal (empty) state range was found \"" << stateAsNexus << '\"';
		GenerateNxsExceptionMatrixReading(emsg, taxNum, charNum, token, nameStr);
		}
	return StateCodeForStateSet(sset, poly, true, nexusSymbol);
	}



/*----------------------------------------------------------------------------------------------------------------------
|	Called from HandleNextState to read in the next state when TOKENS was specified. Looks up state in character 
|	states listed for the character to make sure it is a valid state, and returns state's value (0, 1, 2, ...). Note: 
|	does NOT handle adding the state's value to matrix. Save the return value (call it k) and use the following command
|	to add it to matrix: matrix->AddState(i, j, k);
*/
bool NxsCharactersBlock::HandleNextTokenState(
  NxsToken &token, 
  unsigned taxNum, 
  unsigned charNum, 
  NxsDiscreteStateRow & row, 
  NxsDiscreteDatatypeMapper &mapper, 
  const NxsDiscreteStateRow * firstTaxonRow, 
  const NxsString & nameStr)	
  {
	if (interleaving)
		token.SetLabileFlagBit(NxsToken::newlineIsToken);
	token.GetNextToken();
	if (interleaving && token.AtEOL())
		return false;
	if (token.GetTokenLength() ==  0)
		GenerateNxsException(token, "Unexpected empty token encountered");

	int polymorphism = token.Equals("(");
	int uncertainty	 = token.Equals("{");
	if (!uncertainty && !polymorphism)
		{
		row[charNum] = HandleTokenState(token, taxNum, charNum, mapper, firstTaxonRow, nameStr);
		return true;
		}
		
	/*TODO - supporting this requires more bookeeping to allow the mapper to deal with 
	anonymous fundamental states  -- difficult because we don't know the number of symbols in TOKENS mode.*/
	errormsg = "Currently polymorphism and ambiguity are not supported for matrices in TOKENS mode: ";
	errormsg << token.GetToken() << " found while reading character " << charNum + 1 << " of taxon \"" << nameStr << '\"';
	throw NxsException(errormsg, token);
	
	bool tildeFound = false;
	int prevState = NXS_INVALID_STATE_CODE;
	std::string prevToken = token.GetToken();
	std::set<int> sset;
	for (;;)
		{
		// OPEN ISSUE: What about newlines if interleaving? I'm assuming
		// that the newline must come between characters to count.

		token.SetLabileFlagBit(NxsToken::tildeIsPunctuation);
		token.GetNextToken();

		if (token.Equals(","))
			{
			; /*Mesquite exports with , in state sets. We'll ignore the comma*/
			}
		if (polymorphism)
			{
			if (token.Equals(")"))
				{
				if (tildeFound)
					mapper.GenerateNxsExceptionMatrixReading("Range of states still being specified when ')' encountered", taxNum, charNum, token, nameStr);
				break;
				}
			if (token.Equals("{"))
				mapper.GenerateNxsExceptionMatrixReading("Illegal range of states '{' found inside '()'", taxNum, charNum, token, nameStr);

			}
		else if (uncertainty)
			{ 
			if (token.Equals("}"))
				{
				if (tildeFound)
					mapper.GenerateNxsExceptionMatrixReading("Range of states still being specified when '}' encountered", taxNum, charNum, token, nameStr);
				break;
				}
			if (token.Equals("("))
				mapper.GenerateNxsExceptionMatrixReading("Illegal range of states '(' found inside '{}'", taxNum, charNum, token, nameStr);
			}
		else if (token.Equals("~"))
			{
			if (prevState < 0 || prevState >= (int)symbols.length())
				{
				errormsg.clear();
				errormsg << "A state range cannot start with " << prevToken;
				mapper.GenerateNxsExceptionMatrixReading(errormsg, taxNum, charNum, token, nameStr);
				}
			tildeFound = true;
			}
		else 
			{
			int currState;
			if (tildeFound)
				{
				currState = HandleTokenState(token, taxNum, charNum, mapper, firstTaxonRow, nameStr);
				if (currState <= prevState)
					{
					errormsg = "Last state in specified range (";
					errormsg << token.GetToken() << ") must be greater than the first";
					mapper.GenerateNxsExceptionMatrixReading(errormsg, taxNum, charNum, token, nameStr);
					}
				for (int i = prevState; i <= currState; ++i)
					sset.insert(i);
				tildeFound = false;
				}
			else
				{
				// Add current state, then set first to that state's value
				// State's value is its position within the list of states
				// for that character
				//
				currState = HandleTokenState(token, taxNum, charNum, mapper, firstTaxonRow, nameStr);
				sset.insert(currState);
				}
			prevState = currState;
			prevToken = token.GetToken();
			}
		}

	if (prevToken == "~")
		{
		errormsg.clear();
		errormsg << "State range not terminated -- ending in ~" << token.GetToken();
		mapper.GenerateNxsExceptionMatrixReading(errormsg, taxNum, charNum, token, nameStr);
		}
	if (sset.empty())
		{
		errormsg.clear();
		errormsg << "An illegal (empty) state range -- either  {} or ()";
		mapper.GenerateNxsExceptionMatrixReading(errormsg, taxNum, charNum, token, nameStr);
		}
	row[charNum] = mapper.StateCodeForStateSet(sset, (const bool)(polymorphism != 0), true, '\0');
	return true;
	}
	
int NxsCharactersBlock::HandleTokenState(
  NxsToken &token,	/* the token used to read from `in' */
  unsigned taxNum,
  unsigned charNum,
  NxsDiscreteDatatypeMapper &, 
  const NxsDiscreteStateRow * ,
  const NxsString & nameStr)
  	{
	// Token may be one of the character states listed for character charNum in charStates
	const std::string t = token.GetToken(respectingCase);
	NxsStringVectorMap::const_iterator bagIter	= charStates.find(charNum);
	/*
	 if (bagIter == charStates.end())
		return mapper.EncodeNexusStateString(t, token, taxNum, charNum, firstTaxonRow);
	*/
	NxsStringVector::const_iterator ci_begin	= bagIter->second.begin();
	NxsStringVector::const_iterator ci_end		= bagIter->second.end();
	NxsStringVector::const_iterator cit;
	int k = 0;
	for (; ci_begin != ci_end; ++ci_begin, ++k)
		{
		if (respectingCase)
			{
			if (*ci_begin == t)
				return k;
			}
		else
			{
			if (NxsString::case_insensitive_equals(t.c_str(), ci_begin->c_str()))
				return k;
			}
		}
	//return mapper.EncodeNexusStateString(t, token, taxNum, charNum, firstTaxonRow);
	errormsg = "Unrecognized state ";
	errormsg << t << " found while reading character " << charNum + 1 << " of taxon number " << taxNum + 1;
	if (!nameStr.empty())
		errormsg << "(name \"" << nameStr << "\")";
	throw NxsException(errormsg, token);
	}


unsigned NxsCharactersBlock::GetMaxIndex() const
	{
	unsigned nct = GetNCharTotal();
	if (nct == 0)
		return UINT_MAX;
	return nct - 1;
	}

/*----------------------------------------------------------------------------------------------------------------------
| Returns the number of indices that correspond to the label (and the number 
| of items that would be added to *inds if inds points to an empty set).
*/
unsigned NxsCharactersBlock::GetIndicesForLabel(const std::string &label, NxsUnsignedSet *inds) const
	{
	NxsString emsg;
	const unsigned numb = CharLabelToNumber(label);
	if (numb != 0)
		{
		if (inds)
			inds->insert(numb - 1);
		return 1;
		}
	if (!defCodonPosPartitionName.empty())
		{
		std::string t(label);
		NxsString::to_upper(t);
		std::string n;
		if (t == "POS1")
			n.assign("1");
		else if (t == "POS2")
			n.assign("2");
		else if (t == "POS3")
			n.assign("3");
		else if (t == "NONCODING")
			n.assign("N");
		if (!n.empty())
			{
			NxsPartitionsByName::const_iterator pit = codonPosPartitions.find(defCodonPosPartitionName);
			if (pit != codonPosPartitions.end())
				{
				const NxsPartition & p = pit->second;
				for (NxsPartition::const_iterator s = p.begin(); s != p.end(); ++s)
					{
					if (NxsString::case_insensitive_equals(n.c_str(), s->first.c_str()))
						{
						unsigned nel = (unsigned)s->second.size();
						if (inds)
							inds->insert(s->second.begin(), s->second.end());
						return nel;
						}
					}
				}
			}
		}
	if (NxsString::case_insensitive_equals(label.c_str(), "CONSTANT"))
		{
		NxsUnsignedSet c;
		FindConstantCharacters(c);
		if (inds)
			inds->insert(c.begin(), c.end());
		return (unsigned)c.size();
		}
	if (NxsString::case_insensitive_equals(label.c_str(), "GAPPED"))
		{
		NxsUnsignedSet c;
		FindGappedCharacters(c);
		if (inds)
			inds->insert(c.begin(), c.end());
		return (unsigned)c.size();
		}
	return GetIndicesFromSetOrAsNumber(label, inds, charSets, GetMaxIndex(), "character");
	}

/*----------------------------------------------------------------------------------------------------------------------
|	Returns true if this set replaces an older definition.
*/
bool NxsCharactersBlock::AddNewIndexSet(const std::string &label, const NxsUnsignedSet & inds)
	{
	NxsString ls(label.c_str());
	bool replaced = charSets.count(ls) > 0;
	charSets[ls] = inds;
	return replaced;
	}

/*----------------------------------------------------------------------------------------------------------------------
|	Returns true if this set replaces an older definition.
*/
bool NxsCharactersBlock::AddNewPartition(const std::string &label, const NxsPartition & inds)
	{
	NxsString ls(label.c_str());
	ls.ToUpper();
	bool replaced = charPartitions.count(ls) > 0;
	charPartitions[ls] = inds;
	return replaced;
	}

/*----------------------------------------------------------------------------------------------------------------------
|	Returns true if this set replaces an older definition.
*/
bool NxsCharactersBlock::AddNewCodonPosPartition(const std::string &label, const NxsPartition & inds, bool isDef)
	{
	NxsString ls(label.c_str());
	ls.ToUpper();
	bool replaced = codonPosPartitions.count(ls) > 0;
	codonPosPartitions[ls] = inds;
	if (isDef)
		defCodonPosPartitionName = ls;
	return replaced;
	}

/*----------------------------------------------------------------------------------------------------------------------
|	Returns true if this set replaces an older definition.
*/
bool NxsCharactersBlock::AddNewExSet(const std::string &label, const NxsUnsignedSet & inds)
	{
	NxsString ls(label.c_str());
	bool replaced = exSets.count(ls) > 0;
	exSets[ls] = inds;
	return replaced;
	}

/*----------------------------------------------------------------------------------------------------------------------
|	Initializes `id' to "CHARACTERS", `taxa' to `tb', `assumptionsBlock' to `ab', `ntax', `ntaxTotal', `nChar' and 
|	`nChar' to 0, `newchar' to true, `newtaxa', `interleaving', `transposing', `respectingCase', `tokens' and 
|	`datatype' to `NxsCharactersBlock::standard', `missing' to '?', `gap' and `matchchar'
|	to '\0', and `matrix', `charPos', `taxonPos', `activeTaxon', and `activeChar' to NULL. The ResetSymbols member 
|	function is called to reset the `symbols' data member. Assumes that `tb' and `ab' point to valid NxsTaxaBlockAPI and 
|	NxsAssumptionsBlockAPI objects, respectively.
*/
NxsCharactersBlock::NxsCharactersBlock(
  NxsTaxaBlockAPI *tb,			/* the taxa block object to consult for taxon labels */
  NxsAssumptionsBlockAPI *ab)	/* the assumptions block object to consult for exclusion sets */
  	:NxsTaxaBlockSurrogate(tb, NULL)
	{
	assumptionsBlock = ab;
	id = "CHARACTERS";
	supportMixedDatatype = false;
	convertAugmentedToMixed = false;
	writeInterleaveLen = -1;
	Reset();
	}
/*----------------------------------------------------------------------------------------------------------------------
|	Excludes characters whose indices are contained in the set `exset'. 
|	Returns number of characters actually excluded (some may have already been excluded).
*/
unsigned NxsCharactersBlock::ApplyExset(
  NxsUnsignedSet &exset)	/* set of character indices to exclude in range [0..`nChar') */
	{
	excluded.clear();
	set_union(eliminated.begin(), eliminated.end(), exset.begin(), exset.end(), inserter(excluded, excluded.begin()));
	return (unsigned) excluded.size();
	}

/*----------------------------------------------------------------------------------------------------------------------
|	Includes characters whose indices are contained in the set `inset'.
*/
unsigned NxsCharactersBlock::ApplyIncludeset(
  NxsUnsignedSet &inset)	/* set of character indices to include in range [0..`nChar') */
	{
	NxsUnsignedSet inc(inset);
	inc.erase(eliminated.begin(), eliminated.end());
	excluded.erase(inc.begin(), inc.end());
	return nChar - (unsigned) excluded.size();
	}

/*----------------------------------------------------------------------------------------------------------------------
|	Converts a character label to a 1-offset number corresponding to the character's position based on data from
|	the CharLabels NEXUS command.
|	If `s' is not a valid character label, returns the value 0.
*/
unsigned NxsCharactersBlock::CharLabelToNumber(
  const std::string &inp) const	/* the character label to convert */
	{
	NxsString s(inp.c_str());
	s.ToUpper();
	std::map<std::string, unsigned>::const_iterator ltindIt = ucCharLabelToIndex.find(s);
	if (ltindIt == ucCharLabelToIndex.end())
		return 0;
	return 1 + ltindIt->second;
	}

/*----------------------------------------------------------------------------------------------------------------------
|	Transfers all data from `other' to this object, leaving `other' completely empty. Used to convert a NxsDataBlock 
|	object to a NxsCharactersBlock object in programs where it is desirable to just have a NxsCharactersBlock for 
|	storage but also allow users to enter the information in the form of the deprecated NxsDataBlock. This function 
|	does not make a copy of such things as the data matrix, instead just transferring the pointer to that object from 
|	other to this. This is whay it was named Consume rather than CopyFrom.
*/
void NxsCharactersBlock::Consume(
  NxsCharactersBlock &other)	/* NxsCharactersBlock object from which to copy */
	{
	if (assumptionsBlock)
		assumptionsBlock->SetCallback(NULL);
	assumptionsBlock = other.assumptionsBlock;
	other.assumptionsBlock = NULL;
	if (assumptionsBlock)
		assumptionsBlock->SetCallback(this);
	
	nChar = other.nChar;
	nTaxWithData = other.nTaxWithData;
	matchchar = other.matchchar;
	respectingCase = other.respectingCase;
	transposing = other.transposing;
	interleaving = other.interleaving;
	tokens = other.tokens;
	labels = other.labels;
	missing = other.missing;
	gap = other.gap;
	symbols = other.symbols;
	userEquates = other.userEquates;
	defaultEquates = other.defaultEquates;
	discreteMatrix = other.discreteMatrix;
	continuousMatrix = other.continuousMatrix;
	eliminated = other.eliminated;
	excluded = other.excluded;
	ucCharLabelToIndex = other.ucCharLabelToIndex;
	indToCharLabel = other.indToCharLabel;
	charStates = other.charStates;
	items = other.items;
	charSets = other.charSets;
	charPartitions = other.charPartitions;
	exSets = other.exSets;
	datatype = other.datatype;
	originalDatatype = other.originalDatatype;
	datatypeReadFromFormat = other.datatypeReadFromFormat;
	statesFormat = other.statesFormat;
	datatypeMapperVec = other.datatypeMapperVec;
	isEmpty = false;
	isUserSupplied = other.isUserSupplied;
	supportMixedDatatype = other.supportMixedDatatype;
	convertAugmentedToMixed = other.convertAugmentedToMixed;
	writeInterleaveLen = other.writeInterleaveLen;
	other.Reset();
	transfMgr.Reset();
	}
	

void NxsCharactersBlock::WriteStatesForTaxonAsNexus(
  std::ostream &out,			/* output stream on which to print matrix */
  unsigned taxNum,
  unsigned beginCharInd,
  unsigned endCharInd) const	{
  	assert(endCharInd <= this->nChar);
  	
	if (datatype == continuous)
		{
		const ContinuousCharRow & row = GetContinuousMatrixRow(taxNum);
		if (!row.empty())
			{
			assert(endCharInd <= row.size());
			for (unsigned charInd = beginCharInd; charInd < endCharInd; ++charInd)
				{
				out << ' ';
				ShowStateLabels(out, taxNum, charInd, UINT_MAX);
				}
			}
		}
	else
		{
		const NxsDiscreteStateRow & row = GetDiscreteMatrixRow(taxNum);
		assert(endCharInd <= row.size());
		if (!row.empty())
			{
			const NxsDiscreteDatatypeMapper * dm = GetDatatypeMapperForChar(0);
			if (dm == NULL)
				throw NxsNCLAPIException("No DatatypeMapper in WriteStatesForTaxonAsNexus");
			if (IsMixedType())
				{
				
				for (unsigned charInd = beginCharInd; charInd < endCharInd; ++charInd)
					{
					dm = GetDatatypeMapperForChar(charInd);
					if (dm == NULL)
						{
						errormsg = "No DatatypeMapper for character ";
						errormsg << charInd + 1 << " in WriteStatesForTaxonAsNexus";
						throw NxsNCLAPIException(errormsg);
						}
					const int c = row.at(charInd);
					dm->WriteStateCodeAsNexusString(out, c);
					}
				}
			else
				{
				if (tokens)
					{
					for (unsigned charInd = beginCharInd; charInd < endCharInd; ++charInd)
						{
						int sc = row[charInd];
						out << ' ';
						if (sc == NXS_GAP_STATE_CODE)
							out << gap;
						else 
							{
							NxsString sl = GetStateLabel(charInd, sc);
							if (sl == " ")
								{
								errormsg = "Writing character state ";
								errormsg << 1 + sc << " for character " << 1+charInd << ", but no appropriate chararcter label or symbol was found.";
								throw NxsNCLAPIException(errormsg);
								}
							else
								out  << NxsString::GetEscaped(sl);
							}
						}
					}
				else
					{
					std::vector<int>::const_iterator endIt = row.begin() + beginCharInd;
					std::vector<int>::const_iterator begIt = endIt;
					if (endCharInd == row.size())
						endIt = row.end();
					else
						endIt += endCharInd - beginCharInd;
					dm->WriteStateCodeRowAsNexus(out, begIt, endIt);
					}
				}
			}
		}
	}
	

/*----------------------------------------------------------------------------------------------------------------------
|	Provides a dump of the contents of the `matrix' variable. Useful for testing whether data is being read as 
|	expected. If marginText is NULL, matrix output is placed flush left. If each line of output should be prefaced with 
|	a tab character, specify "\t" for `marginText'.
*/
void NxsCharactersBlock::DebugShowMatrix(
  std::ostream &out,			/* output stream on which to print matrix */
  bool ,	/* deprecated, matchchar no longer used for output */
  char *marginText) const /* for printing first on each line */
	{
	if (!taxa)
		return;
	const unsigned width = taxa->GetMaxTaxonLabelLength();
	const unsigned ntt = GetNTaxTotal();
	for (unsigned i = 0; i < ntt; i++)
		{
		bool skip = true;
		if (datatype == continuous)
			{
			const ContinuousCharRow & row = GetContinuousMatrixRow(i);
			skip = row.empty();
			}
		else
			{
			const NxsDiscreteStateRow & row = GetDiscreteMatrixRow(i);
			skip = row.empty();
			}
		if (!skip)	
			{
			if (marginText != NULL)
				out << marginText;
			const NxsString currTaxonLabel = taxa->GetTaxonLabel(i);
			out << currTaxonLabel;
			unsigned currTaxonLabelLen = (unsigned)currTaxonLabel.size();
			unsigned diff = width - currTaxonLabelLen;
			std::string spacer(diff+5, ' ');
			out << spacer;
			WriteStatesForTaxonAsNexus(out, i, 0, nChar);
			out << endl;
			}
		}
	}

/*----------------------------------------------------------------------------------------------------------------------
|	Returns the maximum observed number of states for any character. Note: this function is rather slow, as it must 
|	walk through each row of each column, adding the states encountered to a set,  then finally returning the size of 
|	the set. Thus, if this function is called often, it would be advisable to initialize an array using this function, 
|	then refer to the array subsequently. 
*/
unsigned NxsCharactersBlock::GetMaxObsNumStates()
	{
	unsigned max = 1;
	for (unsigned j = 0; j < nChar; j++)
		{
		unsigned ns = GetObsNumStates(j);
		if (ns > max)
			ns = max;
		}
	return max;
	}

/*----------------------------------------------------------------------------------------------------------------------
|	Performs a count of the number of active characters.
*/
unsigned NxsCharactersBlock::GetNumActiveChar()
	{
	unsigned num_active_char = 0;
	for (unsigned i = 0; i < nChar; i++)
		{
		if (IsActiveChar(i))
			num_active_char++;
		}
	return num_active_char;
	}



/*----------------------------------------------------------------------------------------------------------------------
|	Returns label for character state `j' at character `i', if a label has been specified. If no label was specified, 
|	returns string containing a single blank (i.e., " ").
*/
NxsString NxsCharactersBlock::GetStateLabelImpl(
  unsigned i,	/* the locus in range [0..`nChar') */
  unsigned j) const	/* the 0-offset index of the state of interest */
	{
	NxsString s = " ";
	NxsStringVectorMap::const_iterator cib = charStates.find(i);
	if (cib != charStates.end() && j < static_cast<unsigned>(cib->second.size()))
		s = cib->second[j];
	return s;
	}


/*----------------------------------------------------------------------------------------------------------------------
|	Returns true if `ch' can be found in the `symbols' array. The value of `respectingCase' is used to determine 
|	whether or not the search should be case sensitive. Assumes `symbols' is non-NULL.
*/
bool NxsCharactersBlock::IsInSymbols(
  char ch)	/* the symbol character to search for */
	{
	char char_in_question = (respectingCase ? ch : (char)toupper(ch));
	for (std::string::const_iterator sIt = symbols.begin(); sIt != symbols.end(); ++sIt)
		{
		const char char_in_symbols = (respectingCase ? *sIt : (char)toupper(*sIt));
		if (char_in_symbols == char_in_question)
			return true;
		}
	return false;
	}

/*----------------------------------------------------------------------------------------------------------------------
|	Called when CHARLABELS command needs to be parsed from within the DIMENSIONS block. Deals with everything after 
|	the token CHARLABELS up to and including the semicolon that terminates the CHARLABELS command. If an ELIMINATE 
|	command has been processed, labels for eliminated characters will not be stored.
*/
void NxsCharactersBlock::HandleCharlabels(
  NxsToken &token)	/* the token used to read from `in' */
	{
	ucCharLabelToIndex.clear();
	indToCharLabel.clear();
	unsigned ind = 0;
	for (;;)
		{
		token.GetNextToken();
		if (token.Equals(";"))
			break;
		else
			{
			if (ind >= nChar)
				GenerateNxsException(token, "Number of character labels exceeds NCHAR specified in DIMENSIONS command");
			NxsString t = token.GetToken();
			if (t != " ")
				{
				indToCharLabel[ind] = t;
				t.ToUpper();
				ucCharLabelToIndex[t] = ind;
				}
			ind++;
			}
		}
	}

/*----------------------------------------------------------------------------------------------------------------------
|	Called when CHARSTATELABELS command needs to be parsed from within the CHARACTERS block. Deals with everything 
|	after the token CHARSTATELABELS up to and including the semicolon that terminates the CHARSTATELABELS command. 
|	CharLabels data structures  will store labels only for characters that have not been eliminated, and likewise for 
|	`charStates'. Specifically, `charStates[0]' refers to the vector of character state labels for the first 
|	non-eliminated character.
*/
void NxsCharactersBlock::HandleCharstatelabels(
  NxsToken &token)	/* the token used to read from `in' */
	{
	unsigned currChar = 0;
	bool semicolonFoundInInnerLoop = false;
	bool tokenAlreadyRead = false;
	bool save = true;

	charStates.clear();
	ucCharLabelToIndex.clear();
	indToCharLabel.clear();
	
	for (;;)
		{
		save = true;

		if (semicolonFoundInInnerLoop)
			break;

		if (tokenAlreadyRead)
			tokenAlreadyRead = false;
		else
			token.GetNextToken();

		if (token.Equals(";"))
			break;

		// Token should be the character number; create a new association
		//
		int sn = atoi(token.GetToken().c_str());
		unsigned n = (unsigned)sn;
		if (sn < 1 || n > nChar || n <= currChar)
			{
			errormsg = "Invalid character number (";
			errormsg += token.GetToken();
			errormsg += ") found in CHARSTATELABELS command (either out of range or not interpretable as an integer)";
			throw NxsException(errormsg, token.GetFilePosition(), token.GetFileLine(), token.GetFileColumn());
			}

		currChar = n; 

		token.GetNextToken();

		// Token should be the character label or / if there is no label 
		//	' ' is a placeholder for no label.
		//
		if (save)
			{
			NxsString t = token.GetToken();
			if (t != " " && !token.Equals("/"))
				{
				indToCharLabel[currChar - 1] = t;
				t.ToUpper();
				ucCharLabelToIndex[t] = currChar - 1;
				}
			}
		if (!token.Equals("/"))
			token.GetNextToken();

		// Token should be a slash character if state labels were provided for this character; otherwise, 
		// token should be one of the following:
		// 1) the comma separating information for different characters, in which case we read in the 
		//	  next token (which should be the next character number)
		// 2) the semicolon indicating the end of the command
		//
		if (!token.Equals("/"))
			{
			if (!token.Equals(",") && !token.Equals(";"))
				{
				errormsg = "Expecting a comma or semicolon here, but found \"";
				errormsg += token.GetToken();
				errormsg += "\" instead";
				throw NxsException(errormsg, token.GetFilePosition(), token.GetFileLine(), token.GetFileColumn());
				}
			if (token.Equals(","))
				token.GetNextToken();
			tokenAlreadyRead = true;
			continue;
			}

		// Now create a new association for the character states list

		for (;;)
			{
			token.GetNextToken();

			if (token.Equals(";"))
				{
				semicolonFoundInInnerLoop = true;
				break;
				}

			if (token.Equals(","))
				break;

			if (save)
				{
				if (datatype == continuous)
					GenerateNxsException(token, "State Labels cannot be specified when the datatype is continuous");

				// Token should be a character state label; add it to the list
				NxsString cslabel = token.GetToken();
				charStates[n - 1].push_back(cslabel);
				}

			} // inner for (;;) loop (grabbing state labels for character n)
		} // outer for (;;) loop
	}

/*----------------------------------------------------------------------------------------------------------------------
|	Called when DIMENSIONS command needs to be parsed from within the CHARACTERS block. Deals with everything after 
|	the token DIMENSIONS up to and including the semicolon that terminates the DIMENSIONs command. `newtaxaLabel', 
|	`ntaxLabel' and `ncharLabel' are simply "NEWTAXA", "NTAX" and "NCHAR" for this class, but may be different for 
|	derived classes that use `newtaxa', `ntax' and `nChar' for other things (e.g., ntax is number of populations in 
|	an ALLELES block)
*/
void NxsCharactersBlock::HandleDimensions(
  NxsToken &token,			/* the token used to read from `in' */
  NxsString newtaxaLabel,	/* the label used in data file for `newtaxa' */
  NxsString ntaxLabel,		/* the label used in data file for `ntax' */
  NxsString ncharLabel)		/* the label used in data file for `nChar' */
	{
	nChar = 0;
	unsigned ntaxRead = 0;
	for (;;)
		{
		token.GetNextToken();
		if (token.Equals(newtaxaLabel))
			newtaxa = true;
		else if (token.Equals(ntaxLabel)) 
			{
			DemandEquals(token, "after NTAX in DIMENSIONS command");
			ntaxRead = DemandPositiveInt(token, ntaxLabel.c_str());
			}
		else if (token.Equals(ncharLabel)) 
			{
			DemandEquals(token, "in DIMENSIONS command");
			nChar = DemandPositiveInt(token, ncharLabel.c_str());
			}
		else if (token.Equals(";"))
			break;
		}

	if (nChar == 0)
		{
		errormsg = "DIMENSIONS command must have an NCHAR subcommand .";
		throw NxsException(errormsg, token);
		}
	if (newtaxa)
		{
		if (ntaxRead == 0)
			{
			errormsg = "DIMENSIONS command must have an NTAX subcommand when the NEWTAXA option is in effect.";
			throw NxsException(errormsg, token);
			}
		AssureTaxaBlock(createImpliedBlock, token, "Dimensions");
		if (!createImpliedBlock)
			{
			taxa->Reset();
			if (nexusReader)
				nexusReader->RemoveBlockFromUsedBlockList(taxa);
			}
		taxa->SetNtax(ntaxRead);
		nTaxWithData = ntaxRead;
		}
	else
		{
		AssureTaxaBlock(false, token, "Dimensions");
		const unsigned ntaxinblock = taxa->GetNTax();
		if (ntaxinblock == 0)
			{
			errormsg = "A TAXA block must be read before character data, or the DIMENSIONS command must use the NEWTAXA.";
			throw NxsException(errormsg, token);
			}

		if (ntaxinblock < ntaxRead)
			{
			errormsg = ntaxLabel;
			errormsg += " in ";
			errormsg += id;
			errormsg += " block must be less than or equal to NTAX in TAXA block\nNote: one circumstance that can cause this error is \nforgetting to specify ";
			errormsg += ntaxLabel;
			errormsg += " in DIMENSIONS command when \na TAXA block has not been provided";
			throw NxsException(errormsg, token.GetFilePosition(), token.GetFileLine(), token.GetFileColumn());
			}
		nTaxWithData = (ntaxRead == 0 ? ntaxinblock : ntaxRead);
		}
	}

/*----------------------------------------------------------------------------------------------------------------------
|	Called when ELIMINATE command needs to be parsed from within the CHARACTERS block. Deals with everything after the 
|	token ELIMINATE up to and including the semicolon that terminates the ELIMINATE command. Any character numbers 
|	or ranges of character numbers specified are stored in the NxsUnsignedSet `eliminated', which remains empty until 
|	an ELIMINATE command is processed. Note that like all sets the character ranges are adjusted so that their offset 
|	is 0. For example, given "eliminate 4-7;" in the data file, the eliminate array would contain the values 3, 4, 5 
|	and 6 (not 4, 5, 6 and 7). It is assumed that the ELIMINATE command comes before character labels and/or character 
|	state labels have been specified; an error message is generated if the user attempts to use ELIMINATE after a 
|	CHARLABELS, CHARSTATELABELS, or STATELABELS command.
*/
void NxsCharactersBlock::HandleEliminate(
  NxsToken &token)	/* the token used to read from `in' */
	{
	if (!eliminated.empty() && nexusReader)
		nexusReader->NexusWarnToken("Only one ELIMINATE command should be used in a CHARACTERS or DATA block (it must appear before the MATRIX command).\n   New character eliminations will be added to the previous eliminated characters (the previously eliminated characters will continue to be excluded).", NxsReader::UNCOMMON_SYNTAX_WARNING, token);
	token.GetNextToken();
	NxsSetReader::ReadSetDefinition(token, *this, "Character", "Eliminate", &eliminated);
	assert(eliminated.size() <= nChar);
	}



/*----------------------------------------------------------------------------------------------------------------------
|	Called from HandleMatrix function to read in a standard (i.e., non-transposed) matrix. Interleaving, if 
|	applicable, is dealt with herein.
*/
void NxsCharactersBlock::HandleStdMatrix(
  NxsToken &token)	/* the token used to read from `in' */
	{
	assert(taxa != NULL);
	unsigned indOfTaxInCommand;
	unsigned indOfTaxInMemory;
	unsigned currChar = 0;
	unsigned firstChar = 0;
	unsigned lastChar = nChar;
	unsigned nextFirst = 0;
	unsigned page = 0;
	const bool continuousData =  (datatype == NxsCharactersBlock::continuous);
	const unsigned ntlabels = taxa->GetNumTaxonLabels();
	errormsg.clear();
	bool taxaBlockNeedsLabels = (ntlabels == 0);
	if (!taxaBlockNeedsLabels && ntlabels < nTaxWithData)
		{
		errormsg << "Not enough taxlabels are known to read characters for " << nTaxWithData << " taxa in the Matrix command.";
		throw NxsException(errormsg, token);
		}
	ContinuousCharRow emptyContRow;
	NxsDiscreteStateRow emptyDiscRow;
	ContinuousCharRow *contRowPtr = NULL;
	NxsDiscreteStateRow *discRowPtr = NULL;
	ContinuousCharRow *ftContRowPtr = NULL;
	NxsDiscreteStateRow *ftDiscRowPtr = NULL;
	const bool isContinuous = (datatype == NxsCharactersBlock::continuous);
	if (isContinuous)
		emptyContRow.resize(nChar);
	else 
		emptyDiscRow.assign(nChar, NXS_INVALID_STATE_CODE);
	std::vector<unsigned> toInMem(nTaxWithData, UINT_MAX);
	std::vector<unsigned> nCharsRead(nTaxWithData, 0);

	for (; currChar < nChar; page++)
		{
		for (indOfTaxInCommand = 0; indOfTaxInCommand < nTaxWithData ; indOfTaxInCommand++)
			{
			NxsString nameStr;
			if (labels)
				{
				token.GetNextToken();
				nameStr = token.GetToken();
				if (taxaBlockNeedsLabels)
					{
					if (taxa->IsAlreadyDefined(nameStr))
						{
						errormsg << "Data for this taxon (" << nameStr << ") has already been saved";
						throw NxsException(errormsg, token);
						}
					indOfTaxInMemory = taxa->AddTaxonLabel(nameStr);
					}
				else
					{
					unsigned numOfTaxInMemory = taxa->TaxLabelToNumber(nameStr);
					if (numOfTaxInMemory == 0)
						{
						if (token.Equals(";"))
							errormsg << "Unexpected ; (after only " << currChar << " characters were read)";
						else
							errormsg << "Could not find taxon named \"" << nameStr << "\" among stored taxon labels";
						if (currChar > 0)
							errormsg << "\n   Expecting data for taxon \"" << taxa->GetTaxonLabel(toInMem[indOfTaxInCommand]) << "\"";
						throw NxsException(errormsg, token.GetFilePosition(), token.GetFileLine(), token.GetFileColumn());
						}
					indOfTaxInMemory = numOfTaxInMemory - 1;
					}
				}
			else
				{
				indOfTaxInMemory = indOfTaxInCommand;
				nameStr << (indOfTaxInMemory + 1);
				}
			if (page == 0)
				{
				if (isContinuous)
					{
					assert(indOfTaxInMemory < continuousMatrix.size());
					continuousMatrix[indOfTaxInMemory] = emptyContRow;
					}
				else 
					{
					assert(indOfTaxInMemory < discreteMatrix.size());
					discreteMatrix[indOfTaxInMemory] = emptyDiscRow;
					}
				if (toInMem[indOfTaxInCommand] != UINT_MAX)
					{
					errormsg << "Characters for taxon \"" << nameStr << "\" (number " << indOfTaxInMemory + 1 << "and \"" << taxa->GetTaxonLabel(indOfTaxInMemory) << "\" according to the taxa block) have already been stored";
					throw NxsException(errormsg, token);
					}
				toInMem[indOfTaxInCommand] = indOfTaxInMemory;
				}
			else
				{
				if (toInMem[indOfTaxInCommand] != indOfTaxInMemory)
					{
					errormsg << "Ordering of taxa must be identical to that in first interleave page. Taxon \"" << nameStr << "\" was not expected.";
					throw NxsException(errormsg, token);
					}
				}		

			if (firstChar > 0 && nCharsRead[indOfTaxInCommand] >= firstChar)
				{
				errormsg << "Data for this taxon (" << nameStr << ") have already been saved";
				throw NxsException(errormsg, token);
				}
			if (isContinuous)
				{
				contRowPtr = &continuousMatrix[indOfTaxInMemory];
				if (ftDiscRowPtr == NULL)
					ftContRowPtr = contRowPtr;
				}
			else 
				{
				discRowPtr = &discreteMatrix[indOfTaxInMemory];
				if (ftDiscRowPtr == NULL)
					ftDiscRowPtr = discRowPtr;
				}
			
			//******************************************************
			//******** Beginning of loop through characters ********
			//******************************************************
			bool atEOL = false;
			for (currChar = firstChar; currChar < lastChar; currChar++)
				{
				NxsDiscreteDatatypeMapper * currMapper =  GetMutableDatatypeMapperForChar(currChar);
				// atEOL will be false only if a newline character is encountered before character j processed
				if (continuousData)
					atEOL = HandleNextContinuousState(token, indOfTaxInMemory, currChar, *contRowPtr, nameStr);
				else
					{
					assert(currMapper);
					if (tokens)
						atEOL = HandleNextTokenState(token, indOfTaxInMemory, currChar, *discRowPtr, *currMapper, ftDiscRowPtr, nameStr);
					else
						atEOL = HandleNextDiscreteState(token, indOfTaxInMemory, currChar, *discRowPtr, *currMapper, ftDiscRowPtr, nameStr);
					}
				if (interleaving && !atEOL)
					{
					if (lastChar < nChar && currChar != lastChar)
						{
						errormsg << "Each line within an interleave page must comprise the same number of characters.  Error reading taxon \"" << nameStr << '\"'; 
						throw NxsException(errormsg, token);
						}

					// currChar should be firstChar in next go around
					nextFirst = currChar;

					// Set lastChar to currChar so that we can check to make sure the remaining lines 
					// in this interleave page end at the same place
					lastChar = currChar;
					}
				}
			if (lastChar > 0)
				nCharsRead[indOfTaxInCommand] = lastChar - 1;
			if (lastChar < nChar && indOfTaxInCommand > 0)
				{
				token.SetLabileFlagBit(NxsToken::newlineIsToken);
				token.GetNextToken();
				if (!token.AtEOL())
					{
					errormsg << "Each line within an interleave page must comprise the same number of characters\n. Expecting the end of a line, but found " << token.GetToken() << " when reading data for taxon \"" << nameStr << '\"';
					throw NxsException(errormsg, token);
					}
				}
			else
				{
				const char nextch = token.PeekAtNextChar();
				if (indOfTaxInCommand > 0 && (!atEOL) && (strchr(";[\n\r \t", nextch) == NULL) && nexusReader)
					{
					errormsg << "Expecting a whitespace character at the end of the characters for taxon \""<< nameStr << "\" but found " << nextch;
					nexusReader->NexusWarnToken(errormsg, NxsReader::UNCOMMON_SYNTAX_WARNING, token); 
					errormsg.clear();
					}
				}
			}
		firstChar = nextFirst;
		lastChar = nChar;
		taxaBlockNeedsLabels = false; /* taxaBlockNeedsLabels can only be true on the first page */
		}
	}

/*----------------------------------------------------------------------------------------------------------------------
|	Called from HandleMatrix function to read in a transposed matrix. Interleaving, if applicable, is dealt with herein.
*/
void NxsCharactersBlock::HandleTransposedMatrix(
  NxsToken &token)	/* the token used to read from in */
	{
	assert(taxa);
	unsigned currTaxon = 0;
	unsigned firstTaxon = 0;
	unsigned lastTaxon = nTaxWithData;
	unsigned nextFirst = 0;
	unsigned page = 0;
	const bool continuousData =  (datatype == NxsCharactersBlock::continuous);
	unsigned indOfCharInCommand, indOfCharInMemory;
	const bool isContinuous = (datatype == NxsCharactersBlock::continuous);

	if (isContinuous)
		{
		ContinuousCharRow emptyContRow(nChar);
		for (unsigned i = 0; i < nTaxWithData; ++ i)
			continuousMatrix[i] = emptyContRow;
		}
	else 
		{
		NxsDiscreteStateRow emptyDiscRow(nChar, NXS_INVALID_STATE_CODE);
		for (unsigned i = 0; i < nTaxWithData; ++ i)
			discreteMatrix[i] = emptyDiscRow;
		}
	vector<unsigned> toInMem(nChar, UINT_MAX);
	vector<unsigned> nTaxRead(nChar, 0);
	bool needsCharLabels = indToCharLabel.empty();
	for (;;	page++)
		{
		for (indOfCharInCommand = 0; indOfCharInCommand < nChar; indOfCharInCommand++)
			{
			NxsString rawToken;
			if (labels)
				{
				token.GetNextToken();
				if (needsCharLabels)
					{
					rawToken = token.GetToken();
					NxsString s = rawToken;
					s.ToUpper();
					if (ucCharLabelToIndex.count(s) > 0)
						{
						errormsg << "Data for this character (" << token.GetToken() << ") has already been saved";
						throw NxsException(errormsg, token.GetFilePosition(), token.GetFileLine(), token.GetFileColumn());
						}
					ucCharLabelToIndex[s] = indOfCharInCommand;
					indToCharLabel[indOfCharInCommand] = rawToken;
					indOfCharInMemory = indOfCharInCommand;
					}
				else // either not first interleaved page or character labels not previously defined
					{
					rawToken = token.GetToken();
					NxsString s = rawToken;
					s.ToUpper();
					LabelToIndexMap::const_iterator iter = ucCharLabelToIndex.find(s);
					if (iter == ucCharLabelToIndex.end())
						{
						errormsg << "Could not find character named " << token.GetToken() <<  " among stored character labels";
						throw NxsException(errormsg, token.GetFilePosition(), token.GetFileLine(), token.GetFileColumn());
						}
					indOfCharInMemory = iter->second;
					}
				}
			else 
				indOfCharInMemory = indOfCharInCommand;
			
			if (page == 0)
				{
				if (toInMem[indOfCharInCommand] != UINT_MAX)
					{
					errormsg << "States for character " << indOfCharInCommand;
					if (!rawToken.empty())
						errormsg << " (" << rawToken << ") ";
					errormsg << "have already been stored";
					throw NxsException(errormsg, token);
					}
				toInMem[indOfCharInCommand] = indOfCharInMemory;
				}
			else
				{
				if (toInMem[indOfCharInCommand] != indOfCharInMemory)
					{
					errormsg << "The order of characters must be in the same order in each page of the interleaved matrix. Character " << rawToken << " was unexpected.";
					throw NxsException(errormsg, token);
					}
				}
			if (firstTaxon > 0 && nTaxRead[indOfCharInCommand] >= firstTaxon)
				{
				errormsg << "Data for this character ";
				if (!rawToken.empty())
					errormsg << '(' << rawToken << ") ";
				errormsg << "has already been saved";
				throw NxsException(errormsg, token);
				}

			NxsDiscreteDatatypeMapper * currMapper =  GetMutableDatatypeMapperForChar(indOfCharInMemory);
				
			for (currTaxon = firstTaxon; currTaxon < lastTaxon; currTaxon++)
				{
				bool atEOL = false;
				NxsString nameStr;
				nameStr << 1+currTaxon;
				if (continuousData)
					{
					ContinuousCharRow *contRowPtr = &continuousMatrix[currTaxon];
					atEOL = HandleNextContinuousState(token, currTaxon, indOfCharInMemory, *contRowPtr, nameStr);
					}
				else
					{
					NxsDiscreteStateRow *discRowPtr = &discreteMatrix[currTaxon];
					if (tokens)
						atEOL = HandleNextTokenState(token,  currTaxon, indOfCharInMemory, *discRowPtr, *currMapper, NULL, nameStr);
					else
						atEOL = HandleNextDiscreteState(token, currTaxon, indOfCharInMemory, *discRowPtr, *currMapper, NULL, nameStr);
					}
				if (interleaving && !atEOL)
					{
					if (lastTaxon < nTaxWithData && currTaxon != lastTaxon)
						GenerateNxsException(token, "Each line within an interleave page must comprise the same number of taxa");

					// currTaxon should be firstChar in next go around
					nextFirst = currTaxon;

					// Set lastTaxon to currTaxon so that we can check to make sure the
					// remaining lines in this interleave page end at the same place
					lastTaxon = currTaxon;
					}
				}
			if (currTaxon > 0)
				nTaxRead[indOfCharInCommand] = currTaxon - 1;
			if (lastTaxon < nTaxWithData && indOfCharInCommand > 0)
				{
				token.SetLabileFlagBit(NxsToken::newlineIsToken);
				token.GetNextToken();
				if (!token.AtEOL())
					{
					errormsg = "Each line within an interleave page must comprise the same number of taxa\n.";
					errormsg << "Expecting the end of a line, but found " << token.GetToken();
					throw NxsException(errormsg, token);
					}
				}			
			}
		firstTaxon = nextFirst;
		lastTaxon = nTaxWithData;
		if (currTaxon == nTaxWithData)
			break;
		needsCharLabels = false;
		}
	}

/*----------------------------------------------------------------------------------------------------------------------
|	Called when MATRIX command needs to be parsed from within the CHARACTERS block. Deals with everything after the 
|	token MATRIX up to and including the semicolon that terminates the MATRIX command.
*/
void NxsCharactersBlock::HandleMatrix(
  NxsToken &token)	/* the token used to read from `in' */
	{
	const NxsPartition dtParts;
	const std::vector<DataTypesEnum> dtv;
	if (datatypeMapperVec.empty())
		CreateDatatypeMapperObjects(dtParts, dtv);
	if (taxa == NULL)
		AssureTaxaBlock(false, token, "Matrix");
	
	if (tokens && GetDataType() == standard)
		{
		/* we can run into trouble here because the number of states can be larger than the 
		symbols list in the NxsDiscreteDatatypeMapper object (because CharState labels can be
		used in a matrix, and symbols don't have to be introduced for each character).
		
		We deal with that here, by introducing \0 symbols
		*/
		const unsigned nStatesWSymbols = (const unsigned)symbols.length();
		unsigned nStatesTotal = nStatesWSymbols;
		for (NxsStringVectorMap::const_iterator cib = this->charStates.begin(); cib != this->charStates.end(); ++cib)
			{
			const NxsStringVector & stateLabelsVec = cib->second;
			const unsigned ns = (int)stateLabelsVec.size();
			if (ns > nStatesTotal)
				nStatesTotal = ns;
			}
		if (nStatesTotal > nStatesWSymbols)
			{
			symbols.append(nStatesTotal-nStatesWSymbols, '\0');
			CreateDatatypeMapperObjects(dtParts, dtv);
			}
		}
	const unsigned ntax = taxa->GetNTax();
	if (ntax == 0)
		{
		errormsg = "Must precede ";
		errormsg << id << " block with a TAXA block or specify NEWTAXA and NTAX in the DIMENSIONS command";
		throw NxsException(errormsg, token.GetFilePosition(), token.GetFileLine(), token.GetFileColumn());
		}
	
	discreteMatrix.clear();
	continuousMatrix.clear();

	if (datatype == NxsCharactersBlock::continuous)
		{
		continuousMatrix.clear();
		continuousMatrix.resize(ntax);
		}
	else 
		{
		discreteMatrix.clear();
		discreteMatrix.resize(ntax);
		}
	if (IsMixedType())
		{
		if (transposing)
			throw NxsUnimplementedException("Reading of transposed, mixed datatype matrices will probably never be supported by NCL");
		/*	HandleMixedDatatypeMatrix(token); */
		}
	if (transposing)
		HandleTransposedMatrix(token);
	else
		HandleStdMatrix(token);
	DemandEndSemicolon(token, "MATRIX");
	if (assumptionsBlock)
		assumptionsBlock->SetCallback(this);
	if (convertAugmentedToMixed)
		AugmentedSymbolsToMixed();
	}

/*----------------------------------------------------------------------------------------------------------------------
|	Called when STATELABELS command needs to be parsed from within the DIMENSIONS block. Deals with everything after 
|	the token STATELABELS up to and including the semicolon that terminates the STATELABELS command. Note that the 
|	numbers of states are shifted back one before being stored so that the character numbers in the NxsStringVectorMap 
|	objects are 0-offset rather than being 1-offset as in the NxsReader data file.
*/
void NxsCharactersBlock::HandleStatelabels(
  NxsToken &token)	/* the token used to read from `in' */
	{
	if (datatype == continuous)
		GenerateNxsException(token, "STATELABELS cannot be specified when the datatype is continuous");
	charStates.clear();
	for (;;)
		{
		token.GetNextToken();
		if (token.Equals(";"))
			break;

		int n = atoi(token.GetToken().c_str());
		if (n < 1 || n > (int)nChar)
			{
			errormsg = "Invalid character number (";
			errormsg << token.GetToken() << ") found in STATELABELS command (either out of range or not interpretable as an integer)";
			throw NxsException(errormsg, token.GetFilePosition(), token.GetFileLine(), token.GetFileColumn());
			}
		NxsStringVector & v = charStates[n - 1];
		for (;;)
			{
			token.GetNextToken();
			if (token.Equals(";") || token.Equals(","))
				break;
			v.push_back(token.GetToken());
			} 
		} 
	}

/*----------------------------------------------------------------------------------------------------------------------
|	This function provides the ability to read everything following the block name (which is read by the NxsReader 
|	object) to the END or ENDBLOCK statement. Characters are read from the input stream `in'. Overrides the abstract 
|	virtual function in the base class.
*/
void NxsCharactersBlock::Read(
  NxsToken &token)	/* the token used to read from `in' */
	{
	isEmpty = false;
	isUserSupplied = true;

	NxsString s;
	s = "BEGIN ";
	s += id;
	DemandEndSemicolon(token, s.c_str());
	nTaxWithData = 0;

	for (;;)
		{
		token.GetNextToken();
		NxsBlock::NxsCommandResult res = HandleBasicBlockCommands(token);
		if (res == NxsBlock::NxsCommandResult(STOP_PARSING_BLOCK))
			return;
		if (res != NxsBlock::NxsCommandResult(HANDLED_COMMAND))
			{
			if (token.Equals("DIMENSIONS"))
				HandleDimensions(token, "NEWTAXA", "NTAX", "NCHAR");
			else if (token.Equals("FORMAT"))
				HandleFormat(token);
			else if (token.Equals("ELIMINATE"))
				HandleEliminate(token);
			else if (token.Equals("TAXLABELS"))
				HandleTaxLabels(token);
			else if (token.Equals("CHARSTATELABELS"))
				HandleCharstatelabels(token);
			else if (token.Equals("CHARLABELS"))
				HandleCharlabels(token);
			else if (token.Equals("STATELABELS"))
				HandleStatelabels(token);
			else if (token.Equals("MATRIX"))
				HandleMatrix(token);
			else
				SkipCommand(token);
			}
		}
	}

/*----------------------------------------------------------------------------------------------------------------------
|	This function outputs a brief report of the contents of this CHARACTERS block. Overrides the abstract virtual 
|	function in the base class.
*/
void NxsCharactersBlock::Report(
  std::ostream &out) /* the output stream to which to write the report */
	{
	out << '\n' << id << " block contains ";
	if (nTaxWithData == 0)
		out << "no taxa";
	else if (nTaxWithData == 1)
		out << "one taxon";
	else
		out << nTaxWithData << " taxa";
	out << " and ";
	if (nChar == 0)
		out << "no characters";
	else if (nChar == 1)
		out << "one character";
	else
		out << nChar << " characters";
	out << endl;

	out << "  Data type is \"" << this->GetDatatypeName() << "\"" << endl;
	
	if (respectingCase)
		out << "  Respecting case" << endl;
	else
		out << "  Ignoring case" << endl;

	if (tokens)
		out << "  Multicharacter tokens allowed in data matrix" << endl;
	else
		out << "  Data matrix entries are expected to be single symbols" << endl;

	if (labels && transposing)
		out << "  Character labels are expected on left side of matrix" << endl;
	else if (labels && !transposing)
		out << "  Taxon labels are expected on left side of matrix" << endl;
	else
		out << "  No labels are expected on left side of matrix" << endl;

	if (!indToCharLabel.empty())
		{
		out << "  Character and character state labels:" << endl;
		for (unsigned k = 0; k < nChar; k++) 
			{
			const std::map<unsigned, std::string>::const_iterator toLit = indToCharLabel.find(k);
			const unsigned kNum = 1 + k;
			if (toLit == indToCharLabel.end())
				out << "    " << kNum << "    (no label provided for this character)" << endl;
			else
				out << "    " << kNum << "    " << toLit->second << endl;

			// Output state labels if any are defined for this character
			//
			NxsStringVectorMap::const_iterator cib = charStates.find(k);
			if (cib != charStates.end())
				{
				int ns = (int)cib->second.size();
				for (int m = 0; m < ns; m++)
					out << "        " << cib->second[m] << endl;
				}
			}
		}

	if (transposing && interleaving)
		out << "  Matrix transposed and interleaved" << endl;
	else if (transposing && !interleaving)
		out << "  Matrix transposed but not interleaved" << endl;
	else if (!transposing && interleaving)
		out << "  Matrix interleaved but not transposed" << endl;
	else
		out << "  Matrix neither transposed nor interleaved" << endl;

	out << "  Missing data symbol is '" << missing << '\'' << endl;

	if (matchchar != '\0')
		out << "  Match character is '" << matchchar << '\'' << endl;
	else
		out << "  No match character specified" << endl;

	if (gap != '\0')
		out << "  Gap character specified is '" << gap << '\'' << endl;
	else
		out << "  No gap character specified" << endl;

	out << "  Valid symbols are: " << symbols << endl;

	int numEquateMacros = (int)(userEquates.size() + defaultEquates.size());
	if (numEquateMacros > 0)
		{
		out << "  Equate macros in effect:" << endl;
		std::map<char, NxsString>::const_iterator i = defaultEquates.begin();
		for (; i != defaultEquates.end(); ++i)
			{
			out << "   " << (*i).first << " = " << i->second << endl;
			}
		i = userEquates.begin();
		for (; i != userEquates.end(); ++i)
			{
			out << "   " << (*i).first << " = " << i->second << endl;
			}
		}
	else
		out << "  No equate macros have been defined" << endl;

	if (eliminated.empty())
		out << "  No characters were eliminated" << endl;
	else
		{
		out << "  The following characters were eliminated:" << endl;
		NxsUnsignedSet::const_iterator k;
		for (k = eliminated.begin(); k != eliminated.end(); k++)
			{
			out << "   " << ((*k)+1) << endl;
			}
		}

	
	if (excluded.empty())
		out << "  no characters excluded" << endl;
	else
		{
		out << "  The following characters have been excluded:\n";
		for (NxsUnsignedSet::const_iterator eIt = excluded.begin(); eIt != excluded.end(); ++eIt)
			out << "   " << (*eIt+1) << endl;
		}
	out << "  Data matrix:" << endl;
	DebugShowMatrix(out, false, "    ");
	}

void NxsCharactersBlock::WriteAsNexus(std::ostream &out) const
	{
	out << "BEGIN CHARACTERS;\n";
	WriteBasicBlockCommands(out);
	out << "    DIMENSIONS";
	if (this->taxa)
		{
		const unsigned wod = GetNTaxWithData();
		if (wod > 0)
			{
			const unsigned tnt = taxa->GetNTax();
			if (wod != tnt)
				out << " NTax=" << wod;
			}
		}
	out << " NChar=" << this->nChar<< ";\n";
	this->WriteEliminateCommand(out);
	this->WriteFormatCommand(out);
	this->WriteCharStateLabelsCommand(out);
	this->WriteMatrixCommand(out);
	WriteSkippedCommands(out);
	out << "END;\n";
	}


void NxsCharactersBlock::WriteEliminateCommand(
  std::ostream &out) const /* output stream on which to print matrix */
	{
	if (eliminated.empty())
		return;
	out << "    ELIMINATE";
	for (NxsUnsignedSet::const_iterator u = this->eliminated.begin(); u != this->eliminated.end(); ++u)
		out << ' ' << (1 + *u);
	out << ";\n";
	}


void NxsCharactersBlock::WriteMatrixCommand(
  std::ostream &out) const /* output stream on which to print matrix */
	{
	if (taxa == NULL)
		return;
	unsigned width = taxa->GetMaxTaxonLabelLength();
	const unsigned ntaxTotal = taxa->GetNTax();
	out << "Matrix\n";
	int prec = 6;
	if (datatype == continuous)
		prec = out.precision(10);
	unsigned stride = (this->writeInterleaveLen < 1 ? this->nChar : this->writeInterleaveLen);
	unsigned begChar = 0;
	while (begChar < this->nChar)
		{
		if (begChar > 0)
			out << '\n';
		unsigned endChar  = std::min(begChar + stride, this->nChar);
		for (unsigned i = 0; i < ntaxTotal; i++)
			{
			if (this->TaxonIndHasData(i))
				{
				const std::string currTaxonLabel = NxsString::GetEscaped(taxa->GetTaxonLabel(i));
				out << currTaxonLabel;
				unsigned currTaxonLabelLen = (unsigned)currTaxonLabel.size();
				unsigned diff = width - currTaxonLabelLen;
				for (unsigned k = 0; k < diff+5; k++)
					out << ' ';
	
				WriteStatesForMatrixRow(out, i, UINT_MAX, begChar, endChar);
				out << '\n';
				}
			}
		begChar = endChar;
		}
	out << ";\n";
	if (datatype == continuous)
		out.precision(prec);
	}

void NxsCharactersBlock::WriteStatesForMatrixRow(
  std::ostream &out,				/* the output stream on which to write */
  unsigned currTaxonIndex,	/* the taxon, in range [0..`ntax') */
  unsigned ,
  unsigned beginChar,
  unsigned endChar) const		/* the index of the first taxon (if UINT_MAX, don't use matchchar) */
	{
	WriteStatesForTaxonAsNexus(out, currTaxonIndex, beginChar, endChar);
	}
	

void NxsCharactersBlock::WriteCharLabelsCommand(std::ostream &out) const
	{
	if (indToCharLabel.empty())
		return;
	out << "    CHARLABELS";
	std::map<unsigned, std::string>::const_iterator resultSearchIt;
	const std::map<unsigned, std::string>::const_iterator endIt = indToCharLabel.end();
	unsigned emptyLabelsToWrite = 0;
	for (unsigned oit = 0; oit < nChar; ++oit)
		{
		resultSearchIt = indToCharLabel.find(oit);
		if (resultSearchIt == endIt)
			emptyLabelsToWrite++;
		else
			{
			for (unsigned j = 0; j < emptyLabelsToWrite; ++j)
				out << " _";
			emptyLabelsToWrite = 0;
			out << ' ' << NxsString::GetEscaped(resultSearchIt->second);
			}
		}
	out << ";\n";
	}

void NxsCharactersBlock::WriteCharStateLabelsCommand(std::ostream &out) const
	{
	if (charStates.empty())
		{
		this->WriteCharLabelsCommand(out);
		return;
		}
	const NxsString mtString;
	bool isFirst = true;
	std::map<unsigned, std::string>::const_iterator resultSearchIt;
	const std::map<unsigned, std::string>::const_iterator endIt = indToCharLabel.end();
	const NxsStringVectorMap::const_iterator endCSIt = this->charStates.end();
	for (unsigned oit = 0; oit < nChar; ++oit)
		{
		resultSearchIt = indToCharLabel.find(oit);
		NxsString escapedCLabel;
		if (resultSearchIt != endIt)
			escapedCLabel = NxsString::GetEscaped(resultSearchIt->second).c_str();
		const NxsStringVectorMap::const_iterator cib = this->charStates.find(oit);
		if (isFirst)
			{
			out << "    CharStateLabels \n      ";
			isFirst = false;
			}
		else
			out << ",\n      ";
		out << 1 + oit << ' ';
		if (cib != endCSIt)
			{
			const NxsStringVector & stateLabelsVec = cib->second;
			unsigned ns = (int)stateLabelsVec.size();
			if (!escapedCLabel.empty())
				out << escapedCLabel;				
			out << " / ";
			for (unsigned m = 0; m < ns; m++)
				out << " " << NxsString::GetEscaped(stateLabelsVec[m]);
			}
		else if (!escapedCLabel.empty())
			out << escapedCLabel;
		else out << '/';
		}
	out << ";\n";
	}

void NxsCharactersBlock::WriteFormatCommand(std::ostream &out) const
	{
	const NxsDiscreteDatatypeMapper * mapper =  GetDatatypeMapperForChar(0);
	if (IsMixedType())
		{
		out << "    FORMAT Datatype=MIXED(";
		bool first = true;
		for (std::vector<DatatypeMapperAndIndexSet>::const_iterator mIt = datatypeMapperVec.begin(); mIt != datatypeMapperVec.end(); ++mIt)
			{
			if (first)
				first = false;
			else
				out << ", ";
			out << GetNameOfDatatype(mIt->first.GetDatatype()) << ':';
			NxsSetReader::WriteSetAsNexusValue(mIt->second, out);
			}
		out << ')';
		if (this->missing != '?')
			out << " Missing=" << this->missing;
		if (this->gap != '\0')
			out << "  Gap=" << this->gap;
		}
	else
		mapper->WriteStartOfFormatCommand(out);
		
	if (this->respectingCase)
		out << " RespectCase"; 

	if (this->matchchar != '\0')
		out << "  MatchChar=" << this->matchchar;
	if (this->datatype == continuous)
		{
		out << " Items = (";
		for (vector<std::string>::const_iterator iIt = items.begin(); iIt != items.end(); ++iIt)
			out << *iIt << ' '; 
		out << ")";
		if (this->statesFormat == STATES_PRESENT)
			out << " StatesFormat=StatesPresent";
		}
	else if (this->statesFormat == INDIVIDUALS)
			out << " StatesFormat=Individuals";
	
	if (this->tokens && this->datatype != NxsCharactersBlock::continuous) /*TOKENS is the only choice for continuous data*/
		out << " Tokens";
	if (this->writeInterleaveLen > 1 && (this->nChar > (unsigned)this->writeInterleaveLen ))
		{
		out << " Interleave";
		}
	out << ";\n";
	}

std::map<char, NxsString> NxsCharactersBlock::GetDefaultEquates(DataTypesEnum dt)
	{
	std::map<char, NxsString> defEquates;
	if (dt == NxsCharactersBlock::dna || dt == NxsCharactersBlock::rna || dt == NxsCharactersBlock::nucleotide)
		{
		defEquates['R'] = NxsString("{AG}");
		defEquates['M'] = NxsString("{AC}");
		defEquates['S'] = NxsString("{CG}");
		defEquates['V'] = NxsString("{ACG}");
		if (dt == NxsCharactersBlock::dna || dt == NxsCharactersBlock::nucleotide)
			{
			defEquates['Y'] = NxsString("{CT}");
			defEquates['K'] = NxsString("{GT}");
			defEquates['W'] = NxsString("{AT}");
			defEquates['H'] = NxsString("{ACT}");
			defEquates['B'] = NxsString("{CGT}");
			defEquates['D'] = NxsString("{AGT}");
			defEquates['N'] = NxsString("{ACGT}");
			defEquates['X'] = NxsString("{ACGT}");
			if (dt == NxsCharactersBlock::nucleotide)
				defEquates['U'] ='T';			
			}
		else
			{
			defEquates['Y'] = NxsString("{CU}");
			defEquates['K'] = NxsString("{GU}");
			defEquates['W'] = NxsString("{AU}");
			defEquates['H'] = NxsString("{ACU}");
			defEquates['B'] = NxsString("{CGU}");
			defEquates['D'] = NxsString("{AGU}");
			defEquates['N'] = NxsString("{ACGU}");
			defEquates['X'] = NxsString("{ACGU}");
			}
		}
	else if (dt == NxsCharactersBlock::protein)
		{
		defEquates['B'] = NxsString("{DN}");
		defEquates['Z'] = NxsString("{EQ}");
		defEquates['X'] = NxsString("{ACDEFGHIKLMNPQRSTVWY*}");
		}
	/* molecular datatypes are the only datatypes with default equates and 
		keys of either case are equivalent.
	*/
	NxsString upperKeys;
	for (std::map<char, NxsString>::const_iterator k = defEquates.begin(); k != defEquates.end(); ++k)
		{
		upperKeys += k->first;
		}
	for (std::string::const_iterator k = upperKeys.begin(); k != upperKeys.end(); ++k)
		{
		const char c = *k;
		const char lc = (char)tolower(c);
		defEquates[lc] = defEquates[c];
		}

	return defEquates;
	}

const char * NxsCharactersBlock::GetNameOfDatatype(DataTypesEnum datatype)
	{
	switch(datatype)
		{
		case NxsCharactersBlock::dna:
			return "DNA";
		case NxsCharactersBlock::rna:
			return "RNA";
		case NxsCharactersBlock::nucleotide:
			return "Nucleotide";
		case NxsCharactersBlock::protein:
			return "Protein";
		case NxsCharactersBlock::continuous:
			return "Continuous";
		default:
			return "Standard";
		}
	}

/*----------------------------------------------------------------------------------------------------------------------
|	Returns NxsCharactersBlock object to the state it was in when first created.
*/
void NxsCharactersBlock::Reset()
	{
	ResetSurrogate();
	NxsBlock::Reset();
	nTaxWithData = 0;
	nChar = 0;
	newtaxa				= false;
	interleaving		= false;
	transposing			= false;
	respectingCase		= false;
	labels				= true;
	tokens				= false;
	datatype			= NxsCharactersBlock::standard;
	originalDatatype	= NxsCharactersBlock::standard;
	datatypeReadFromFormat = false;
	missing				= '?';
	gap					= '\0';
	matchchar			= '\0';
	symbols.clear();
	ResetSymbols();
	
	ucCharLabelToIndex.clear();
	indToCharLabel.clear();
	charSets.clear();
	charPartitions.clear();
	codonPosPartitions.clear();
	defCodonPosPartitionName.clear();
	exSets.clear();
	charStates.clear();
	userEquates.clear();
	defaultEquates.clear();
	eliminated.clear();
	datatypeMapperVec.clear();
	discreteMatrix.clear();
	continuousMatrix.clear();
	items = std::vector<std::string>(1, std::string("STATES"));
	statesFormat = STATES_PRESENT;
	restrictionDataype = false;
	}

std::string NxsCharactersBlock::GetDefaultSymbolsForType(NxsCharactersBlock::DataTypesEnum dt)
	{
	switch(dt)
		{
		case NxsCharactersBlock::nucleotide:
		case NxsCharactersBlock::dna:
			return std::string("ACGT");
		case NxsCharactersBlock::rna:
			return std::string("ACGU");
		case NxsCharactersBlock::protein:
			return std::string("ACDEFGHIKLMNPQRSTVWY*");
		case NxsCharactersBlock::standard:
			return std::string("01");
		default:
			return std::string();

		}
	return std::string();
	}
/*----------------------------------------------------------------------------------------------------------------------
|	Resets standard symbol set after a change in `datatype' is made. Also flushes equates list and installs standard 
|	equate macros for the current `datatype'.
*/
void NxsCharactersBlock::ResetSymbols()
	{
	symbols = GetDefaultSymbolsForType(datatype);
	userEquates.clear();
	defaultEquates = GetDefaultEquates(datatype);
	datatypeMapperVec.clear();
	}

/*----------------------------------------------------------------------------------------------------------------------
|	Looks up the state(s) at row `i', column `charNum' of matrix and writes it (or them) to out. If there is uncertainty or 
|	polymorphism, the list of states is surrounded by the appropriate set of symbols (i.e., parentheses for 
|	polymorphism, curly brackets for uncertainty). If TOKENS was specified, the output takes the form of the defined 
|	state labels; otherwise, the correct symbol is looked up in `symbols' and output.
*/
void NxsCharactersBlock::ShowStateLabels(
  std::ostream &out,				/* the output stream on which to write */
  unsigned taxInd,				/* the taxon, in range [0..`ntax') */
  unsigned charInd,				/* the character, in range [0..`nChar') */
  unsigned ) const		/* the index of the first taxon (if UINT_MAX, don't use matchchar) */
	{
	if (datatype == continuous)
		{
		const ContinuousCharCell & cell = continuousMatrix.at(taxInd).at(charInd);
		std::vector<std::string>::const_iterator itemIt = items.begin();
		bool parensNeeded = items.size() > 1;
		if (items.size() == 1)
			{
			ContinuousCharCell::const_iterator oit = cell.find(*itemIt);
			if (oit != cell.end() && oit->second.size() > 1)
				parensNeeded = true;
			}
		if (parensNeeded)
			out	 << '(';
		for (; itemIt != items.end(); ++itemIt)
			{
			ContinuousCharCell::const_iterator cit = cell.find(*itemIt);
			if (cit == cell.end())
				out << missing << ' ';
			else
				{
				if (cit->second.empty())
					out << missing << ' ';
				else
					{
					vector<double>::const_iterator vIt = cit->second.begin();
					for(; vIt != cit->second.end(); ++vIt)
						{
						if (*vIt == DBL_MAX)
							out << missing << ' ';
						else
							out << *vIt << ' ';
						}
					}
				}
			}
		if (parensNeeded)
			out	 << ") ";
		else
			out << ' ';
		return;
		}
	const NxsDiscreteDatatypeMapper * mapper = GetDatatypeMapperForChar(charInd);
	assert(mapper != NULL);
	const int currStateCode = discreteMatrix.at(taxInd).at(charInd);
	if (tokens)
		{
		out << ' ';
		if (currStateCode >= 0 && currStateCode < (int) mapper->GetNumStates())
			{
			NxsStringVectorMap::const_iterator ci = charStates.find(charInd);
			if (ci != charStates.end() && ((int) ci->second.size()) > currStateCode)
				out << ci->second[currStateCode];
			return;
			}
		}
	mapper->WriteStateCodeAsNexusString(out, currStateCode);
	}

/*----------------------------------------------------------------------------------------------------------------------
|	Writes out the state (or states) stored in this NxsDiscreteDatum object to the buffer `s' using the symbols array 
|	to do the necessary translation of the numeric state values to state symbols. In the case of polymorphism or 
|	uncertainty, the list of states will be surrounded by brackets or parentheses (respectively). Assumes `s' is 
|	non-NULL and long enough to hold everything printed.
*/
void NxsCharactersBlock::WriteStates(
  NxsDiscreteDatum &d,	/* the datum to be queried */
  char *s,				/* the buffer to which to print */
  unsigned slen)		/* the length of the buffer `s' */
	{
	std::ostringstream outs;
	ShowStates(outs, d.taxInd, d.charInd);
	std::string sfo = outs.str();
	if (s == NULL || sfo.length() > slen)
		throw NxsNCLAPIException("Char buffer too small in NxsCharactersBlock::WriteStates");
	strcpy(s, sfo.c_str());
	}

/*----------------------------------------------------------------------------------------------------------------------
|	This function is no longer the most efficient way to access parsed data (see notes on NxsCharacterBlock and 
|	GetMatrix() and GetMatrixDecoder() methods.
|
|	Returns the number of states for taxon `i', character `j'.
*/
unsigned NxsCharactersBlock::GetNumStates(
  unsigned taxInd,	/* the taxon in range [0..`ntax') */
  unsigned charInd)	/* the character in range [0..`nChar') */
	{
	const NxsDiscreteDatatypeMapper * mapper = GetDatatypeMapperForChar(charInd);
	assert(mapper != NULL);
	const int currStateCode = discreteMatrix.at(taxInd).at(charInd);
	return mapper->GetNumStatesInStateCode(currStateCode);
	}

/*----------------------------------------------------------------------------------------------------------------------
|	Returns the number of states for character `j' over all taxa. Note: this function is rather slow, as it must walk 
|	through each row, adding the states encountered to a set, then finally returning the size of the set. Thus, if this 
|	function is called often, it would be advisable to initialize an array using this function, then refer to the array 
|	subsequently.
*/
unsigned NxsCharactersBlock::GetObsNumStates(
  unsigned charInd)	/* the character in range [0..`nChar') */
	{
	const NxsDiscreteDatatypeMapper * mapper = GetDatatypeMapperForChar(charInd);
	assert(mapper != NULL);
	unsigned max = 0;
	for (NxsDiscreteStateMatrix::const_iterator rowIt = discreteMatrix.begin(); rowIt != discreteMatrix.end(); ++rowIt)
		{
		const NxsDiscreteStateRow & row = *rowIt;
		if (row.size() > charInd)
			{
			const int currStateCode = row[charInd];
			if (currStateCode > (int) max)
				max = (unsigned) currStateCode;
			}
		}
	return max;
	}

/*----------------------------------------------------------------------------------------------------------------------
|	Excludes character whose 0-offset current index is `i'. If character has already been excluded, this function has 
|	no effect.
*/
void NxsCharactersBlock::ExcludeCharacter(
  unsigned i)	/* index of character to exclude in range [0..`nChar') */
	{
	if (i >= nChar)
		{
		errormsg  = "Character index is ExcludeCharacter out-of-range.   Must be < ";
		errormsg << nChar;
		throw NxsNCLAPIException(errormsg);
		}
	excluded.insert(i);
	}
/*----------------------------------------------------------------------------------------------------------------------
|	Includes character whose 0-offset current index is `i'. If character is already active, this function has no effect.
*/
void NxsCharactersBlock::IncludeCharacter(
  unsigned i)	/* index of character to include in range [0..`nChar') */
	{
	if (i >= nChar)
		{
		errormsg  = "Character index is ExcludeCharacter out-of-range.   Must be < ";
		errormsg << nChar;
		throw NxsNCLAPIException(errormsg);
		}
	excluded.erase(i);
	}

/*----------------------------------------------------------------------------------------------------------------------
|	Returns true iff taxon `taxInd` has a gap for character `charInd`
*/
bool NxsCharactersBlock::IsGapState(
  unsigned taxInd,	/* the taxon, in range [0..`ntax') */
  unsigned charInd)	/* the character, in range [0..`nChar') */
	{
	const NxsDiscreteStateRow & row = discreteMatrix.at(taxInd);
	return (row.size() > charInd && row[charInd] == NXS_GAP_STATE_CODE);
	}
/*----------------------------------------------------------------------------------------------------------------------
|	Returns true iff taxon `taxInd` has is missing for character `charInd`
*/
bool NxsCharactersBlock::IsMissingState(
  unsigned taxInd,	/* the taxon, in range [0..`ntax') */
  unsigned charInd)	/* the character, in range [0..`nChar') */
	{
	const NxsDiscreteStateRow & row = discreteMatrix.at(taxInd);
	return (row.size() <= charInd || (row[charInd] == NXS_MISSING_CODE));
	}


void NxsCharactersBlock::FindConstantCharacters(NxsUnsignedSet &c) const
	{
	vector<int> iv;
	for (unsigned colIndex = 0; colIndex < nChar; ++colIndex)
		{
		const NxsDiscreteDatatypeMapper * mapper = GetDatatypeMapperForChar(colIndex);
		if (mapper == NULL)
			throw NxsNCLAPIException("No DatatypeMapper in FindConstantCharacters");
		
		std::set<int> intersectionSet = mapper->GetStateSetForCode(NXS_MISSING_CODE);
		for (NxsDiscreteStateMatrix::const_iterator rowIt = discreteMatrix.begin(); rowIt != discreteMatrix.end(); ++rowIt)
			{
			const NxsDiscreteStateRow & row = *rowIt;
			if (row.size() > colIndex)
				{
				const int sc = row[colIndex];
				std::set<int> currSet = mapper->GetStateSetForCode(sc);
				iv.clear();
				set_intersection(currSet.begin(), currSet.end(), intersectionSet.begin(), intersectionSet.end(), back_inserter(iv));
				intersectionSet.clear();
				if (iv.empty())
					break;
				intersectionSet.insert(iv.begin(), iv.end());
				}
			}
		if (!intersectionSet.empty())
			c.insert(colIndex);
		}
	}

void NxsCharactersBlock::FindGappedCharacters(NxsUnsignedSet &c) const
	{
	vector<int> iv;
	for (unsigned colIndex = 0; colIndex < nChar; ++colIndex)
		{
		for (NxsDiscreteStateMatrix::const_iterator rowIt = discreteMatrix.begin(); rowIt != discreteMatrix.end(); ++rowIt)
			{
			const NxsDiscreteStateRow & row = *rowIt;
			if (row.size() > colIndex && row[colIndex] == NXS_GAP_STATE_CODE)
				{
				c.insert(colIndex); 
				break;
				}
			}
		}
	}

/* Behaves like GetMaximalStateSetOfColumn except that missing data columns do not increase
	size of the returned state set.
*/
std::set<int> NxsCharactersBlock::GetNamedStateSetOfColumn(const unsigned colIndex) const
	{
	const NxsDiscreteDatatypeMapper * mapper = GetDatatypeMapperForChar(colIndex);
	if (mapper == NULL)
		throw NxsNCLAPIException("No DatatypeMapper in GetNamedStateSetOfColumn");

	std::set<int> sset;
	std::set<int> scodes;
	const unsigned maxnstates = mapper->GetNumStatesIncludingGap();
	for (NxsDiscreteStateMatrix::const_iterator rowIt = discreteMatrix.begin(); rowIt != discreteMatrix.end(); ++rowIt)
		{
		const NxsDiscreteStateRow & row = *rowIt;
		if (row.size() > colIndex)
			{
			const int sc = row[colIndex];
			if (sc != NXS_MISSING_CODE && scodes.count(sc) == 0)
				{
				scodes.insert(sc);
				const std::set<int>	& ts = mapper->GetStateSetForCode(sc);
				sset.insert(ts.begin(), ts.end());
				if (sset.size() == maxnstates)
					break;
				}
			}
		}
	return sset;
	}
/* Returns the union of all states that are consistent with a column */
std::set<int> NxsCharactersBlock::GetMaximalStateSetOfColumn(const unsigned colIndex) const
	{
	const NxsDiscreteDatatypeMapper * mapper = GetDatatypeMapperForChar(colIndex);
	if (mapper == NULL)
		throw NxsNCLAPIException("No DatatypeMapper in GetNamedStateSetOfColumn");

	std::set<int> sset;
	std::set<int> scodes;
	const unsigned maxnstates = mapper->GetNumStatesIncludingGap();
	for (NxsDiscreteStateMatrix::const_iterator rowIt = discreteMatrix.begin(); rowIt != discreteMatrix.end(); ++rowIt)
		{
		const NxsDiscreteStateRow & row = *rowIt;
		if (row.size() > colIndex)
			{
			const int sc = row[colIndex];
			if (scodes.count(sc) == 0)
				{
				scodes.insert(sc);
				const std::set<int>	& ts = mapper->GetStateSetForCode(sc);
				sset.insert(ts.begin(), ts.end());
				if (sset.size() == maxnstates)
					break;
				}
			}
		}
	return sset;
	}

/*----------------------------------------------------------------------------------------------------------------------
|	Returns true if taxon `i' is polymorphic for character `j', false otherwise. Assumes `matrix' is non-NULL. Note 
|	that return value will be false if there is only one state (i.e., one cannot tell whether there is uncertainty 
|	using this function).
*/
 bool NxsCharactersBlock::IsPolymorphic(
  unsigned taxInd,	/* the taxon in range [0..`ntax') */
  unsigned charInd)	/* the character in range [0..`nChar') */
	{
	const NxsDiscreteDatatypeMapper * mapper = GetDatatypeMapperForChar(charInd);
	assert(mapper);
	if (taxInd >= discreteMatrix.size())
		throw NxsNCLAPIException("Taxon index out of range of NxsCharactersBlock::IsPolymorphic");
	const NxsDiscreteStateRow & row = discreteMatrix[taxInd];
	if (row.size() <= charInd)
		throw NxsNCLAPIException("Character index out of range of NxsCharactersBlock::IsPolymorphic");
	return mapper->IsPolymorphic(row[charInd]);
	}


/*----------------------------------------------------------------------------------------------------------------------
|	Shows the states for taxon `i', character `j', on the stream `out'. Uses `symbols' array to translate the states 
|	from the way they are stored (as integers) to the symbol used in the original data matrix. Assumes `i' is in the 
|	range [0..`ntax') and `j' is in the range [0..`nChar'). Also assumes `matrix' is non-NULL.
*/
void NxsCharactersBlock::ShowStates(
  std::ostream &out, /* the stream on which to show the state(s) */
  unsigned taxInd,	/* the (0-offset) index of the taxon in question */
  unsigned charInd)	/* the (0-offset) index of the character in question */
	{
	bool ft = tokens;
	tokens = false;
	ShowStateLabels(out, taxInd, charInd, UINT_MAX);
	tokens = ft;
	}

/*---------------------------------------------------------------------------------------
| Results in aliasing of the taxa, assumptionsBlock blocks!
*/
void NxsCharactersBlock::CopyCharactersContents(const NxsCharactersBlock &other)
	{
	assumptionsBlock = other.assumptionsBlock;
	nChar = other.nChar;
	nTaxWithData = other.nTaxWithData;
	matchchar = other.matchchar;
	respectingCase = other.respectingCase;
	transposing = other.transposing;
	interleaving = other.interleaving;
	tokens = other.tokens;
	labels = other.labels;
	missing = other.missing;
	gap = other.gap;
	symbols = other.symbols;
	userEquates = other.userEquates;
	datatypeMapperVec = other.datatypeMapperVec;
	discreteMatrix = other.discreteMatrix;
	continuousMatrix = other.continuousMatrix;
	eliminated = other.eliminated;
	excluded = other.excluded;
	ucCharLabelToIndex = other.ucCharLabelToIndex;
	indToCharLabel = other.indToCharLabel;
	charStates = other.charStates;
	items = other.items;
	charSets = other.charSets;
	exSets = other.exSets;
	charPartitions = other.charPartitions;
	codonPosPartitions = other.codonPosPartitions;
	defCodonPosPartitionName = other.defCodonPosPartitionName;
	transfMgr = other.transfMgr;
	datatype = other.datatype;
	statesFormat = other.statesFormat;
	supportMixedDatatype = other.supportMixedDatatype;
	convertAugmentedToMixed = other.convertAugmentedToMixed;
	restrictionDataype = other.restrictionDataype;
	writeInterleaveLen = other.writeInterleaveLen;
	}


NxsCharactersBlock *NxsCharactersBlockFactory::GetBlockReaderForID(const std::string & idneeded, NxsReader *reader, NxsToken *)
	{
	if (reader == NULL || idneeded != "CHARACTERS")
		return NULL;
	NxsCharactersBlock * nb  = new NxsCharactersBlock(NULL, NULL);
	nb->SetCreateImpliedBlock(true);
	nb->SetImplementsLinkAPI(true);
	return nb;
	}
