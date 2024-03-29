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
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with NCL; if not, write to the Free Software Foundation, Inc.,
//	59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//
#include <climits>
#include "ncl/nxstreesblock.h"

#include <sstream>
#include <stack>

#include "ncl/nxsreader.h"
using namespace std;
#define REGRESSION_TESTING_GET_TRANS_TREE_DESC 0
#define DEBUGGING_TREES_BLOCK 0
enum PrevTreeTokenDesc
		{
		NXS_TREE_OPEN_PARENS_TOKEN,
		NXS_TREE_CLOSE_PARENS_TOKEN,
		NXS_TREE_COMMA_TOKEN,
		NXS_TREE_CLADE_NAME_TOKEN,
		NXS_TREE_COLON_TOKEN,
		NXS_TREE_BRLEN_TOKEN
		};

void NxsSimpleEdge::WriteAsNewick(std::ostream &out, bool nhx) const
	{
	if (!defaultEdgeLen)
		{
		out << ':';
		if (lenAsString.empty())
			if (hasIntEdgeLens)
				out << iEdgeLen;
			else
				out << dEdgeLen;
		else
			out << lenAsString;
		}
	for (std::vector<NxsComment>::const_iterator uc = unprocessedComments.begin(); uc != unprocessedComments.end(); ++uc)
		out << '[' << uc->GetText() << ']';
	if (nhx && !parsedInfo.empty())
		{
		out << "[&&NHX";
		for (std::map<std::string, std::string>::const_iterator p = parsedInfo.begin(); p != parsedInfo.end(); ++p)
			out << ':' << p->first << '=' << p->second;
		out << ']';
		}
	}

void NxsSimpleNode::WriteAsNewick(std::ostream &out, bool nhx) const
	{
	if (lChild)
		{
		out << '(';
		const std::vector<NxsSimpleNode *> children = GetChildren();
		for (std::vector<NxsSimpleNode *>::const_iterator child = children.begin(); child != children.end(); ++child)
			{
			if (child != children.begin())
				out << ',';
			(*child)->WriteAsNewick(out, nhx);
			}
		out << ')';
		if (!name.empty())
			out << NxsString::GetEscaped(name);
		else if (taxIndex != UINT_MAX)
			out << (1 + taxIndex);
		}
	else
		{
		assert (taxIndex != UINT_MAX);
		out << (1 + taxIndex);
		}
	edgeToPar.WriteAsNewick(out, nhx);
	}

void NxsSimpleNode::AddSelfAndDesToPreorder(std::vector<const NxsSimpleNode *> &p) const
	{
	p.push_back(this);
	NxsSimpleNode * currCh = this->lChild;
	while (currCh)
		{
		currCh->AddSelfAndDesToPreorder(p);
		currCh = currCh->rSib;
		}
	}

std::vector<const NxsSimpleNode *> NxsSimpleTree::GetPreorderTraversal() const
	{
	std::vector<const NxsSimpleNode *> p;
	if (root)
		root->AddSelfAndDesToPreorder(p);
	return p;	
	}

std::vector<std::vector<int> > NxsSimpleTree::GetIntPathDistances(bool toMRCA) const
	{
	if (root == NULL || root->lChild == NULL)
		return std::vector<std::vector<int> >();

	typedef std::map<unsigned, int> TaxonIndToDistMap; 
	typedef std::map<unsigned, TaxonIndToDistMap> PairwiseDistMap;
	typedef PairwiseDistMap::iterator PairwiseDistRow;
	
	std::map<const NxsSimpleNode *,  TaxonIndToDistMap > ndToDist;
	const std::vector<const NxsSimpleNode *> preord = GetPreorderTraversal();
	unsigned maxIndex = 0;
	PairwiseDistMap pairwiseDist;
	for (std::vector<const NxsSimpleNode *>::const_reverse_iterator nIt = preord.rbegin(); nIt != preord.rend(); ++nIt)
		{
		const NxsSimpleNode *nd = *nIt;
		if (nd->lChild)
			{
			TaxonIndToDistMap nm;
			ndToDist[nd] = nm;
			TaxonIndToDistMap & tidm = ndToDist[nd];
			const NxsSimpleNode * currChild = nd->lChild;
			if (nd->taxIndex != UINT_MAX)
				{
				if (maxIndex < nd->taxIndex)
					maxIndex = nd->taxIndex;
				tidm[nd->taxIndex] = 0;
				}
			while (currChild)
				{
				TaxonIndToDistMap currChildEls;
				TaxonIndToDistMap * currChildElsPtr;
				int currEdgeLen = currChild->edgeToPar.GetIntEdgeLen();
				if (currChild->lChild)
					{
					assert(ndToDist.find(currChild) != ndToDist.end());
					currChildElsPtr = &(ndToDist[currChild]);
					}
				else
					{
					if (maxIndex < currChild->taxIndex)
						maxIndex = currChild->taxIndex;
					currChildEls[currChild->taxIndex] = 0;
					currChildElsPtr = &currChildEls;
					}
				for (TaxonIndToDistMap::const_iterator i = tidm.begin(); i != tidm.end(); ++i)
					{
					const unsigned iIndex = i->first;
					const int idist = i->second;
					for (TaxonIndToDistMap::const_iterator j = currChildElsPtr->begin(); j != currChildElsPtr->end(); ++j)
						{
						const unsigned jIndex = j->first;
						const int jdist = j->second;
						const int ndToJDist = jdist + currEdgeLen;
						if (toMRCA)
							{
							PairwiseDistRow  iRow = pairwiseDist.find(iIndex);
							PairwiseDistRow  jRow = pairwiseDist.find(jIndex);
							assert(iRow == pairwiseDist.end() || (iRow->second.find(jIndex) == iRow->second.end()));
							assert(jRow == pairwiseDist.end() || (jRow->second.find(iIndex) == jRow->second.end()));
							pairwiseDist[iIndex][jIndex] = idist;
							pairwiseDist[jIndex][iIndex] = ndToJDist;							
							}
						else
							{						
							const unsigned fIndex = (iIndex < jIndex ? iIndex : jIndex);
							const unsigned sIndex = (iIndex < jIndex ? jIndex : iIndex);
							PairwiseDistRow  r = pairwiseDist.find(fIndex);
							const bool found = (r != pairwiseDist.end() && (r->second.find(sIndex) != r->second.end()));
							if (!found)
								pairwiseDist[fIndex][sIndex] = currEdgeLen + idist + jdist;
							}
						}
					}
				for (TaxonIndToDistMap::const_iterator j = currChildElsPtr->begin(); j != currChildElsPtr->end(); ++j)
					tidm[j->first] = currEdgeLen + j->second;
				currChild = currChild->rSib;
				}
			}
		}
	if (maxIndex == 0)
		return std::vector<std::vector<int> >();
	std::vector<int> toTipDistRow(maxIndex+1, INT_MAX);
	std::vector<std::vector<int> > pathDistMat(maxIndex+1, toTipDistRow);
	for (unsigned diagInd = 0; diagInd <= maxIndex; ++diagInd)
		pathDistMat[diagInd][diagInd] = 0;
		
	for (PairwiseDistMap::const_iterator iit = pairwiseDist.begin(); iit != pairwiseDist.end(); ++iit)
		{
		const unsigned iInd = iit->first;
		const TaxonIndToDistMap & toDistMap = iit->second;
		for (TaxonIndToDistMap::const_iterator jit = toDistMap.begin(); jit != toDistMap.end(); ++jit)
			{
			const unsigned jInd = jit->first;
			if (jInd != iInd)
				{
				const int d = jit->second;
				pathDistMat[iInd][jInd] = d;
				pathDistMat[jInd][iInd] = d;
				}
			}
		}

	return pathDistMat;
	}

/* if toMRCA is true the the row i col j element will be the distanc from tip i 
 to the MRCA of (i and j) 
*/
std::vector<std::vector<double> > NxsSimpleTree::GetDblPathDistances(bool toMRCA) const
	{
	if (root == NULL || root->lChild == NULL)
		return std::vector<std::vector<double> >();

	typedef std::map<unsigned, double> TaxonIndToDistMap; 
	typedef std::map<unsigned, TaxonIndToDistMap> PairwiseDistMap; 
	typedef PairwiseDistMap::iterator PairwiseDistRow;

	std::map<const NxsSimpleNode *,  TaxonIndToDistMap > ndToDist;
	const std::vector<const NxsSimpleNode *> preord = GetPreorderTraversal();
	unsigned maxIndex = 0;
	PairwiseDistMap pairwiseDist;
	for (std::vector<const NxsSimpleNode *>::const_reverse_iterator nIt = preord.rbegin(); nIt != preord.rend(); ++nIt)
		{
		const NxsSimpleNode *nd = *nIt;
		if (nd->lChild)
			{
			TaxonIndToDistMap nm;
			ndToDist[nd] = nm;
			TaxonIndToDistMap & tidm = ndToDist[nd];
			if (nd->taxIndex != UINT_MAX)
				{
				if (maxIndex < nd->taxIndex)
					maxIndex = nd->taxIndex;
				tidm[nd->taxIndex] = 0.0;
				}
			// loop over all of the children of nd
			const NxsSimpleNode * currChild = nd->lChild;
			while (currChild)
				{
				TaxonIndToDistMap currChildEls;
				TaxonIndToDistMap * currChildElsPtr;
				double currEdgeLen = currChild->edgeToPar.GetDblEdgeLen();
				if (currChild->lChild)
					{
					assert(ndToDist.find(currChild) != ndToDist.end());
					currChildElsPtr = &(ndToDist[currChild]);
					}
				else
					{
					if (maxIndex < currChild->taxIndex)
						maxIndex = currChild->taxIndex;
					currChildEls[currChild->taxIndex] = 0.0;
					currChildElsPtr = &currChildEls;
					}
				//for each leaf i ( the the previously encountered descendants of nd)...
				for (TaxonIndToDistMap::const_iterator i = tidm.begin(); i != tidm.end(); ++i)
					{
					// compare it to leaf j (descendant of currChild).
					const unsigned iIndex = i->first;
					const double idist = i->second;
					for (TaxonIndToDistMap::const_iterator j = currChildElsPtr->begin(); j != currChildElsPtr->end(); ++j)
						{
						const unsigned jIndex = j->first;
						const double jdist = j->second;
						const double ndToJDist = jdist + currEdgeLen;
						if (toMRCA)
							{
							PairwiseDistRow  iRow = pairwiseDist.find(iIndex);
							PairwiseDistRow  jRow = pairwiseDist.find(jIndex);
							assert(iRow == pairwiseDist.end() || (iRow->second.find(jIndex) == iRow->second.end()));
							assert(jRow == pairwiseDist.end() || (jRow->second.find(iIndex) == jRow->second.end()));
							pairwiseDist[iIndex][jIndex] = idist;
							pairwiseDist[jIndex][iIndex] = ndToJDist;							
							}
						else
							{
							const unsigned fIndex = (iIndex < jIndex ? iIndex : jIndex);
							const unsigned sIndex = (iIndex < jIndex ? jIndex : iIndex);
							PairwiseDistRow  r = pairwiseDist.find(fIndex);
							const bool found = (r != pairwiseDist.end() && (r->second.find(sIndex) != r->second.end()));
							if (!found)
								pairwiseDist[fIndex][sIndex] = idist + ndToJDist;
							}
						}
					}
				for (TaxonIndToDistMap::const_iterator j = currChildElsPtr->begin(); j != currChildElsPtr->end(); ++j)
					tidm[j->first] = currEdgeLen + j->second;
				currChild = currChild->rSib;
				}
			}
		}
	if (maxIndex == 0)
		return std::vector<std::vector<double> >();
	std::vector<double> toTipDistRow(maxIndex+1, DBL_MAX);
	std::vector<std::vector<double> > pathDistMat(maxIndex+1, toTipDistRow);
	for (unsigned diagInd = 0; diagInd <= maxIndex; ++diagInd)
		pathDistMat[diagInd][diagInd] = 0.0;
		

	for (PairwiseDistMap::const_iterator iit = pairwiseDist.begin(); iit != pairwiseDist.end(); ++iit)
		{
		const unsigned iInd = iit->first;
		pathDistMat[iInd][iInd] = 0.0;
		const TaxonIndToDistMap & toDistMap = iit->second;
		for (TaxonIndToDistMap::const_iterator jit = toDistMap.begin(); jit != toDistMap.end(); ++jit)
			{
			const unsigned jInd = jit->first;
			const double d = jit->second;
			pathDistMat[iInd][jInd] = d;
			if (!toMRCA)
				pathDistMat[jInd][iInd] = d;
			}
		}

	return pathDistMat;
	}

/*----------------------------------------------------------------------------------------------------------------------
|	Fills `infoMap` with the key value pairs parsed from a comment that starts with
|		&&NHX
| returns unparsed component
*/
std::string parseNHXComment(const std::string comment, std::map<std::string, std::string> *infoMap)
	{
	if (comment.length() < 6 || comment[0] != '&' || comment[1] != '&' || comment[2] != 'N' ||comment[3] != 'H' || comment[4] != 'X' )
		return comment;
	size_t colonPos = comment.find(':', 5);
	if (colonPos == string::npos)
		return comment.substr(5, string::npos);
	for (;;)
		{
		size_t eqPos = comment.find('=', colonPos);
		if (eqPos == string::npos || (eqPos <= (colonPos + 1)))
			return comment.substr(colonPos, string::npos);
		std::string key = comment.substr(colonPos + 1, eqPos - 1 - colonPos);
		colonPos = comment.find(':', eqPos + 1);
		if (colonPos == eqPos + 1)
			{
			if (infoMap)
				(*infoMap)[key] = string();
			}
		else if (colonPos == string::npos)
			{
			std::string lastVal = comment.substr(eqPos + 1);
			if (infoMap)
				(*infoMap)[key] = lastVal;
			return std::string();
			}
		else 
			{
			std::string value = comment.substr(eqPos + 1, colonPos - eqPos - 1);
			if (infoMap)
				(*infoMap)[key] = value;
			}
		}
	}

void NxsSimpleEdge::DealWithNexusComments(const std::vector<NxsComment> & ecs, bool NHXComments)
	{
	for (std::vector<NxsComment>::const_iterator ecsIt = ecs.begin(); ecsIt != ecs.end(); ++ecsIt)
		{
		if (NHXComments)
			{
			std::string ns = ecsIt->GetText();
			std::map<std::string, std::string> currCmt;
			std::string unparsed = parseNHXComment(ns, &currCmt);
			for (std::map<std::string, std::string>::const_iterator c = currCmt.begin(); c != currCmt.end(); ++c)
				{
				const std::string & k = c->first;
				const std::string & v = c->second;
				this->parsedInfo[k] = v;
				}
			if (!unparsed.empty())
				{
				if (unparsed.length() == ns.length())
					this->unprocessedComments.push_back(*ecsIt);
				else
					{
					NxsComment nc(unparsed, ecsIt->GetLineNumber(), ecsIt->GetColumnNumber());
					this->unprocessedComments.push_back(nc);
					}
				}
			}
		else
			this->unprocessedComments.push_back(*ecsIt);
		}
	}

void NxsSimpleTree::Initialize(const NxsFullTreeDescription & td)
	{
	if (!td.IsProcessed())
		throw NxsNCLAPIException("A tree description must be processed by ProcessTree before calling NxsSimpleTree::NxsSimpleTree");
	Clear();
	std::string s;
	const std::string & n = td.GetNewick();
	s.reserve(s.length() + 1);
	s.assign(n.c_str());
	s.append(1, ';');
	istringstream newickstream(s);
	NxsToken token(newickstream);
	token.SetEOFAllowed(false);
	realEdgeLens = td.SomeEdgesHaveLengths() && (! td.EdgeLengthsAreAllIntegers());
	const bool NHXComments = td.HasNHXComments();
	NxsString emsg;
	double lastFltEdgeLen;
	long lastIntEdgeLen;
	long currTaxNumber;
	token.GetNextToken();
	assert(token.Equals("("));
	root = AllocNewNode(0L);
	NxsSimpleNode * currNd = root;
	NxsSimpleEdge * currEdge = &(currNd->edgeToPar);
	NxsSimpleNode * tmpNode;
	bool prevInternalOrLength;
	bool currInternalOrLength = false;
	for (;;)
		{
		currEdge->DealWithNexusComments(token.GetEmbeddedComments(), NHXComments);
		if (token.Equals(";"))
			{
			if (currNd != root)
				throw NxsNCLAPIException("Semicolon found before the end of the tree description.  This means that more \"(\" characters  than \")\"  were found.");
			break;
			}
		const NxsString & tstr = token.GetTokenReference();
		const char * t = tstr.c_str();
		bool handled;
		handled = false;
		prevInternalOrLength = currInternalOrLength;
		currInternalOrLength = false;

		if (tstr.length() == 1)
			{
			handled = true;
			if (t[0] == '(')
				{
				tmpNode = AllocNewNode(currNd);
				currNd->AddChild(tmpNode);
				currNd = tmpNode;
				currEdge = &(currNd->edgeToPar);
				}
			else if (t[0] == ')')
				{
				currNd = currNd->GetParent();
				assert(currNd);
				currEdge = &(currNd->edgeToPar);
				currInternalOrLength = true;
				}
			else if (t[0] == ':')
				{
				token.SetLabileFlagBit(NxsToken::hyphenNotPunctuation); // this allows us to deal with sci. not. in branchlengths (and negative branch lengths).
				token.GetNextToken();
				currEdge->DealWithNexusComments(token.GetEmbeddedComments(), NHXComments);
				t = token.GetTokenReference().c_str();
				if (realEdgeLens)
					{
					if (!NxsString::to_double(t, &lastFltEdgeLen))
						{
						emsg << "Expecting a number as a branch length. Found " << tstr;
						throw NxsException(emsg, token);
						}
					currEdge->SetDblEdgeLen(lastFltEdgeLen, t);
					}
				else
					{
					if (!NxsString::to_long(t, &lastIntEdgeLen))
						{
						emsg << "Expecting a number as a branch length. Found " << tstr;
						throw NxsException(emsg, token);
						}
					currEdge->SetIntEdgeLen(lastIntEdgeLen, t);
					}
				currInternalOrLength = true;
				}
			else if (t[0] == ',')
				{
				currNd = currNd->GetParent();
				assert(currNd);
				tmpNode = AllocNewNode(currNd);
				currNd->AddChild(tmpNode);
				currNd = tmpNode;
				currEdge = &(currNd->edgeToPar);
				}
			else 
				handled = false;
			}
		if (!handled)
			{
			bool wasReadAsNumber = NxsString::to_long(t, &currTaxNumber);
			if (wasReadAsNumber)
				{
				if (currTaxNumber < 1)
					{
					if (!prevInternalOrLength)
						{
						emsg << "Expecting a taxon number greater than 1. Found " << tstr;
						throw NxsException(emsg, token);
						}
					wasReadAsNumber = false;
					}
				}
			if (wasReadAsNumber)
				{
				currNd->taxIndex = (unsigned)currTaxNumber - 1;
				if (currNd->lChild == NULL)
					{
					while (currNd->taxIndex >= leaves.size())
						leaves.push_back(0L);
					leaves[currNd->taxIndex] = currNd;
					}
				}
			else
				currNd->name = t;
			}
		token.GetNextToken();
		}
	}
unsigned NxsTreesBlock::TreeLabelToNumber(const std::string & name) const
	{
	NxsString r(name.c_str());
	r.ToUpper();
	std::map<std::string, unsigned>::const_iterator cntiIt = capNameToInd.find(r);
	if (cntiIt == capNameToInd.end())
		return 0;
	return cntiIt->second + 1;
	}
unsigned NxsTreesBlock::GetMaxIndex() const
	{
	if (trees.size() == 0)
		return UINT_MAX;
	return (unsigned)trees.size() - 1;
	}
/*----------------------------------------------------------------------------------------------------------------------
| Returns the number of indices that correspond to the label (and the number
| of items that would be added to *inds if inds points to an empty set).
*/
unsigned NxsTreesBlock::GetIndicesForLabel(const std::string &label, NxsUnsignedSet *inds) const
	{
	NxsString emsg;
	const unsigned numb = TreeLabelToNumber(label);
	if (numb > 0)
		{
		if (inds)
			inds->insert(numb - 1);
		return 1;
		}
	return GetIndicesFromSetOrAsNumber(label, inds, treeSets, GetMaxIndex(), "tree");
	}
bool NxsTreesBlock::AddNewIndexSet(const std::string &label, const NxsUnsignedSet & inds)
	{
	NxsString  nlabel(label.c_str());
	const bool replaced = treeSets.count(nlabel) > 0;
	treeSets[nlabel] = inds;
	return replaced;
	}
/*----------------------------------------------------------------------------------------------------------------------
|	Returns true if this set replaces an older definition.
*/
bool NxsTreesBlock::AddNewPartition(const std::string &label, const NxsPartition & inds)
	{
	NxsString ls(label.c_str());
	bool replaced = treePartitions.count(ls) > 0;
	treePartitions[ls] = inds;
	return replaced;
	}
/*----------------------------------------------------------------------------------------------------------------------
|	Initializes `id' to "TREES", `ntrees' to 0, `defaultTree' to 0, and `taxa' to `tb'. Assumes `tb' is non-NULL.
*/
NxsTreesBlock::NxsTreesBlock(
  NxsTaxaBlockAPI *tb)	/* the NxsTaxaBlockAPI object to be queried for taxon names appearing in tree descriptions */
  :NxsTaxaBlockSurrogate(tb, NULL)
	{
	id			= "TREES";
	defaultTreeInd = UINT_MAX;
	allowImplicitNames = false;
	processAllTreesDuringParse = true;
	writeFromNodeEdgeDataStructure = false;
	}
/*----------------------------------------------------------------------------------------------------------------------
|	Clears `translateList', `rooted', `treeName' and `treeDescription'.
*/
NxsTreesBlock::~NxsTreesBlock()
	{
	}
/*----------------------------------------------------------------------------------------------------------------------
|	Makes data member `taxa' point to `tb' rather than the NxsTaxaBlockAPI object it was previously pointing to. Assumes
|	`tb' is non-NULL.
*/
void NxsTreesBlock::ReplaceTaxaBlockPtr(
  NxsTaxaBlockAPI *tb)		/* pointer to new NxsTaxaBlockAPI object (does not attempt to delete the object previously pointed to) */
	{
	assert(tb != NULL);
	taxa = tb;
	}
/*----------------------------------------------------------------------------------------------------------------------
|	Returns the description of the tree stored at position `i' in `treeDescription'. Assumes that `i' will be in the
|	range [0..`ntrees').
*/
NxsString NxsTreesBlock::GetTreeDescription(
  unsigned i)	/* the index of the tree for which the description is to be returned */
	{
	return NxsString(GetFullTreeDescription(i).GetNewick().c_str());
	}
/*----------------------------------------------------------------------------------------------------------------------
|	Returns true if the `i'th tree (0-offset) is rooted, false otherwise. Assumes that `i' will be in the
|	range [0..ntrees).
*/
bool NxsTreesBlock::IsRootedTree(
  unsigned i)	/* the index of the tree in question */
  	{
	return GetFullTreeDescription(i).IsRooted();
	}
/*----------------------------------------------------------------------------------------------------------------------
|	Returns the name of the tree stored at position `i' in `treeName'. Assumes that `i' will be in the range
|	[0..`ntrees').
*/
NxsString NxsTreesBlock::GetTreeName(
  unsigned i)	/* the index of the tree for which the name is to be returned */
	{
	return NxsString(GetFullTreeDescription(i).GetName().c_str());
	}
/*----------------------------------------------------------------------------------------------------------------------
|	Returns true if the `i'th tree (0-offset) is the default tree, false otherwise. Assumes that `i' will be in the
|	range [0..ntrees).
*/
bool NxsTreesBlock::IsDefaultTree(
  unsigned i)	/* the index of the tree in question */
	{
	return (i == GetNumDefaultTree());
	}
const NxsFullTreeDescription & NxsTreesBlock::GetFullTreeDescription(unsigned i) const
	{
	assert(i < trees.size());
	return trees.at(i);
	}
/*----------------------------------------------------------------------------------------------------------------------
|	This function outputs a brief report of the contents of this block. Overrides the abstract virtual function in the
|	base class.
*/
void NxsTreesBlock::Report(
  std::ostream &out)	/* the output stream to which to write the report */
	{
	const unsigned ntrees = GetNumTrees();
	out << '\n' <<  id << " block contains ";
	if (ntrees == 0)
		{
		out << "no trees" << endl;
		return;
		}
	if (ntrees == 1)
		out << "one tree" << endl;
	else
		out << ntrees << " trees" << endl;
	for (unsigned k = 0; k < ntrees; k++)
		{
		const NxsFullTreeDescription & tree = GetFullTreeDescription(k);
		out << "    " << (k+1) << "    " << tree.GetName();
		out << "    (";
		if (tree.IsRooted())
			out << "rooted";
		else
			out << "unrooted";
		if (defaultTreeInd == k)
			out << ",default tree)" << endl;
		else
			out << ')' << endl;
		}
	}
/*----------------------------------------------------------------------------------------------------------------------
|	Outputs a brief description of this block's contents to the referenced NxsString. An example of the output of this
|	command is shown below:
|>
|	TREES block contains 102 trees
|>
*/
void NxsTreesBlock::BriefReport(
  NxsString &s)	/* reference to the string in which to store the contents of the brief report */
	{
	const unsigned ntrees = GetNumTrees();
	s << "\n\n" << id << " block contains ";
	if (ntrees == 0)
		s += "no trees\n";
	else if (ntrees == 1)
		s += "one tree\n";
	else
		s << ntrees << " trees\n";
	}
/*----------------------------------------------------------------------------------------------------------------------
|	Flushes `treeName', `treeDescription', `translateList' and `rooted', and sets `ntrees' and `defaultTree' both to 0
|	in preparation for reading a new TREES block.
*/
void NxsTreesBlock::Reset()
	{
	NxsBlock::Reset();
	ResetSurrogate();
	defaultTreeInd = UINT_MAX;
	trees.clear();
	capNameToInd.clear();
	treeSets.clear();
	treePartitions.clear();
	constructingTaxaBlock = false;
	newtaxa = false;
	}
/*----------------------------------------------------------------------------------------------------------------------
|	Returns the 0-offset index of the default tree, which will be 0 if there is only one tree stored or no trees
|	stored. If more than one tree is stored, the default tree will be the one specifically indicated by the user (using
|	an asterisk in the data file), or 0 if the user failed to specify.
*/
unsigned NxsTreesBlock::GetNumDefaultTree()
	{
	return (defaultTreeInd == UINT_MAX ? 0 : defaultTreeInd);
	}
/*----------------------------------------------------------------------------------------------------------------------
|	Returns the number of trees stored in this NxsTreesBlock object.
*/
unsigned NxsTreesBlock::GetNumTrees() const
	{
	return (unsigned)trees.size();
	}
/*----------------------------------------------------------------------------------------------------------------------
|	Returns the number of trees stored in this NxsTreesBlock object.
*/
unsigned NxsTreesBlock::GetNumTrees()
	{
	return (unsigned)trees.size();
	}
void NxsTreesBlock::WriteTranslateCommand(std::ostream & out) const
	{
	assert(taxa);
	out << "    TRANSLATE" << "\n";
	const unsigned nt = taxa->GetNTaxTotal();
	for (unsigned i = 0; i < nt; ++i)
		{
		if (i > 0)
				out << ",\n";
		out << "        " << i + 1 << ' ' << NxsString::GetEscaped(taxa->GetTaxonLabel(i));
		}
	out << ";\n";
	}

void NxsTreesBlock::WriteTreesCommand(std::ostream & out) const
	{
	if (constructingTaxaBlock)
		{
		// this check is intended to make sure that ProcessTree really behaves
		//	as a const function.
		// If we are constructingTaxaBlock, then the it can modify the contained taxa block
		throw NxsNCLAPIException("WriteTreesCommand block cannot be called while the Trees Block is still being constructed");
		}
	NxsTreesBlock *ncthis = const_cast<NxsTreesBlock *>(this);
	NxsSimpleTree nst(0, 0.0);
	for (unsigned k = 0; k < trees.size(); k++)
		{
#		if defined REGRESSION_TESTING_GET_TRANS_TREE_DESC
			NxsTreesBlock *nc = const_cast<NxsTreesBlock *>(this);
			NxsString transTreeDesc = nc->GetTranslatedTreeDescription(k);
#		endif
		NxsFullTreeDescription & treeDesc = trees.at(k);
		ncthis->ProcessTree(treeDesc);
		const std::string & name = treeDesc.GetName();
		out << "    TREE ";
		if (k == defaultTreeInd)
			out << "* ";
		out << NxsString::GetEscaped(name) << " = [&";
		out << (treeDesc.IsRooted() ? 'R' : 'U');
		out << ']';
		if (writeFromNodeEdgeDataStructure)
			{
			nst.Initialize(treeDesc);
			nst.WriteAsNewick(out, true);
			}
		else
			out << treeDesc.GetNewick();
		out << ";\n";
		}
	}
/*----------------------------------------------------------------------------------------------------------------------
|	Writes contents of this block in NEXUS format to `out'.
*/
void NxsTreesBlock::WriteAsNexus(std::ostream &out) const
	{
	if (GetNumTrees() == 0)
		return;
	out << "BEGIN TREES;\n";
	WriteBasicBlockCommands(out);
	WriteTranslateCommand(out);
	WriteTreesCommand(out);
	WriteSkippedCommands(out);
	out << "END;\n";
	}
NxsTreesBlock *NxsTreesBlockFactory::GetBlockReaderForID(const std::string & idneeded, NxsReader *reader, NxsToken *)
	{
	if (reader == NULL || idneeded != "TREES")
		return NULL;
	NxsTreesBlock * nb = new NxsTreesBlock(NULL);
	nb->SetCreateImpliedBlock(true);
	nb->SetImplementsLinkAPI(true);
	return nb;
	}
void NxsTreesBlock::ConstructDefaultTranslateTable(NxsToken &token, const char * cmd)
	{
	if (taxa == NULL)
		{
		if (nxsReader == NULL)
			GenerateNxsException(token, "A taxa block must be read before the Trees block can be read.");
		unsigned nTb;
		nxsReader->GetTaxaBlockByTitle(NULL, &nTb);
		AssureTaxaBlock(nTb == 0 && allowImplicitNames && createImpliedBlock, token, cmd);
		}
	const unsigned nt = taxa->GetNTaxTotal();
	if (nt == 0)
		{
		if (allowImplicitNames)
			{
			if (nexusReader)
				nexusReader->NexusWarnToken("A TAXA block should be read before the TREES block (but no TAXA block was found).  Taxa will be inferred from their usage in the TREES block.", NxsReader::AMBIGUOUS_CONTENT_WARNING , token);
			constructingTaxaBlock = true;
			newtaxa = true;
			}
		else
			GenerateNxsException(token, "Taxa block must be read before the Trees block can be read.");
		}
	if (!constructingTaxaBlock)
		{
		for (unsigned i = 0; i < nt; ++i)
			{
			NxsString s;
			s += (i + 1);
			capNameToInd[s] = i;
			NxsString t = taxa->GetTaxonLabel(i);
			t.ToUpper();
			capNameToInd[t] = i;
			}
		}
	}
void NxsTreesBlock::HandleTranslateCommand(NxsToken &token)
	{
	for (unsigned n = 0;; ++n)
		{
		token.GetNextToken();
		if (token.Equals(";"))
			break;
		NxsString key(token.GetTokenReference().c_str());
		unsigned keyInd = taxa->TaxLabelToNumber(key);
		token.GetNextToken();
		NxsString value(token.GetTokenReference().c_str());
		unsigned valueInd = taxa->TaxLabelToNumber(value);
		if (valueInd == 0)
			{
			if (constructingTaxaBlock)
				{
				taxa->SetNtax(n+1);
				valueInd = 1 + taxa->AddTaxonLabel(value);
				NxsString numV;
				numV += (valueInd);
				if (capNameToInd.find(numV) == capNameToInd.end())
					capNameToInd[numV] = valueInd;
				}
			else if (nexusReader)
				{
				errormsg << "Unknown taxon " << value << " in TRANSLATE command.\nThe translate key "<< key << " has NOT been added to the translation table!";
				nexusReader->NexusWarnToken(errormsg, NxsReader::PROBABLY_INCORRECT_CONTENT_WARNING, token);
				errormsg.clear();
				}
			}
		if (valueInd > 0)
			{
			if (keyInd != 0 && keyInd != valueInd && nexusReader)
				{
				errormsg << "TRANSLATE command overwriting the taxon " << key << " with a redirection to " << value;
				nexusReader->NexusWarnToken(errormsg, NxsReader::OVERWRITING_CONTENT_WARNING, token);
				errormsg.clear();
				}
			key.ToUpper();
			capNameToInd[key] = valueInd - 1;
			}
		token.GetNextToken();
		if (token.Equals(";"))
			break;
		if (!token.Equals(","))
			{
			errormsg << "Expecting a , or ; after a translate key-value pair. Found " << token.GetTokenReference();
			throw NxsException(errormsg, token);
			}
		}
	constructingTaxaBlock = false;
	}

/*
| Converts to a Nexus token (and thus loses some of the file position information).
*/
void NxsTreesBlock::ProcessTokenVecIntoTree(
  const ProcessedNxsCommand & tokenVec,
  NxsFullTreeDescription & td,
  NxsLabelToIndicesMapper *taxa,
  std::map<std::string, unsigned> &capNameToInd,
  bool allowNewTaxa,
  NxsReader * nexusReader,
  const bool respectCase)
	{
	ProcessedNxsCommand::const_iterator tvIt = tokenVec.begin();
	ostringstream tokenStream;
	long line = 0;
	long col = 0;
	file_pos pos = 0;
	if (!tokenVec.empty())
		{
		line = tvIt->GetLineNumber();
		col = tvIt->GetColumnNumber();
		pos = tvIt->GetFilePosition();
		for (;tvIt != tokenVec.end(); ++tvIt)
			tokenStream << NxsString::GetEscaped(tvIt->GetToken());
		tokenStream << ';';
		}
	std::string s = tokenStream.str();
	istringstream newickstream(s);
	NxsToken token(newickstream);
	try
		{
		ProcessTokenStreamIntoTree(token, td, taxa, capNameToInd, allowNewTaxa, nexusReader, respectCase);
		}
	catch (NxsException & x)
		{
		x.pos += pos;
		x.line += line;
		x.col += col;
		throw x;
		}
	}

void NxsTreesBlock::ProcessTokenStreamIntoTree(
  NxsToken & token,
  NxsFullTreeDescription & td,
  NxsLabelToIndicesMapper *taxa,
  std::map<std::string, unsigned> &capNameToInd,
  bool allowNewTaxa,
  NxsReader * nexusReader,
  const bool respectCase)
	{
	NxsString errormsg;
	int & flags = td.flags;
	bool NHXComments = false;
	bool someMissingEdgeLens = false;
	bool someHaveEdgeLens = false;
	bool someRealEdgeLens = false;
	bool hasPolytomies = false;
	bool hasDegTwoNodes = false;
	bool hasInternalLabels = false;
	bool hasInternalLabelsInTaxa = false;
	bool hasInternalLabelsNotInTaxa = false;
	const bool rooted = (flags & NxsFullTreeDescription::NXS_IS_ROOTED_BIT);
	std::stack<unsigned> nchildren;
	std::set<unsigned> taxaEncountered;
	double minDblEdgeLen = DBL_MAX;
	int minIntEdgeLen = INT_MAX;
	double lastFltEdgeLen;
	long lastIntEdgeLen;
	bool taxsetRead = false;
	token.GetNextToken();
	ostringstream newickStream;
	if (!token.Equals("("))
		{
		errormsg << "Expecting a ( to start the tree description, but found " << token.GetTokenReference();
		throw NxsException(errormsg, token);
		}
	nchildren.push(0);
	newickStream << '(';
	int prevToken = NXS_TREE_OPEN_PARENS_TOKEN;
	token.GetNextToken();
	for (;;)
		{
		const std::vector<NxsComment> & ecs = token.GetEmbeddedComments();
		for (std::vector<NxsComment>::const_iterator ecsIt = ecs.begin(); ecsIt != ecs.end(); ++ecsIt)
			{
			if (!NHXComments)
				{
				const std::string & ns = ecsIt->GetText();
				if (ns.length() > 5 && ns[0] == '&' && ns[1] == '&' && ns[2] == 'N' &&ns[3] == 'H' && ns[4] == 'X')
					NHXComments = true;
				}
			ecsIt->WriteAsNexus(newickStream);
			}
		if (token.Equals(";"))
			{
			if (!nchildren.empty())
				throw NxsException("Semicolon found before the end of the tree description.  This means that more \"(\" characters  than \")\"  were found.", token);
			break;
			}
		const NxsString & tstr = token.GetTokenReference();
		const char * t = tstr.c_str();
		bool handled;
		handled = false;
		if (tstr.length() == 1)
			{
			if (t[0] == '(')
				{
				if (nchildren.empty())
					throw NxsException("End of tree description.  Expected ; but found (", token);
				if (prevToken == NXS_TREE_CLOSE_PARENS_TOKEN || prevToken == NXS_TREE_CLADE_NAME_TOKEN || prevToken == NXS_TREE_BRLEN_TOKEN)
					{
					errormsg << "Expecting a , before a new subtree definition:\n \")(\"\n \"name(\" and\n \"branch-length(\"\n are prohibited.";
					if (nexusReader)
						nexusReader->NexusWarnToken(errormsg, NxsReader::PROBABLY_INCORRECT_CONTENT_WARNING, token);
					else
						throw NxsException(errormsg, token);
					/* if we did not throw an excection, then we are in relaxed parsing mode.  
						We'll add the implied ,
					*/
					if (!someMissingEdgeLens && (prevToken == NXS_TREE_CLOSE_PARENS_TOKEN || prevToken == NXS_TREE_CLADE_NAME_TOKEN))
						someMissingEdgeLens = true;
					nchildren.top() += 1;
					newickStream << ',';
					prevToken = NXS_TREE_COMMA_TOKEN;
					}
				else if (prevToken == NXS_TREE_COLON_TOKEN)
					throw NxsException("Expecting a branch length after a : but found (", token);
				nchildren.top() += 1;
				nchildren.push(0);
				newickStream << '(';
				prevToken = NXS_TREE_OPEN_PARENS_TOKEN;
				handled = true;
				}
			else if (t[0] == ')')
				{
				if (nchildren.empty())
					throw NxsException("End of tree description.  Expected ; but found )", token);
				if (prevToken == NXS_TREE_OPEN_PARENS_TOKEN || prevToken == NXS_TREE_COMMA_TOKEN)
					throw NxsException("Expecting a clade description before the subtree's closing )\n \"()\" and \",)\" are prohibited.", token);
				if (prevToken == NXS_TREE_COLON_TOKEN)
					throw NxsException("Expecting a branch length after a : but found (", token);
				if (!someMissingEdgeLens && (prevToken == NXS_TREE_CLOSE_PARENS_TOKEN || prevToken == NXS_TREE_CLADE_NAME_TOKEN))
					someMissingEdgeLens = true;
				if (nchildren.top() == 1)
					hasDegTwoNodes = true;
				else if ((!hasPolytomies) && (nchildren.top() > 2))
					{
					if (rooted)
						{
						if (nchildren.top() > 2)
							hasPolytomies = true;
						}
					else if (nchildren.top() > 3 || nchildren.size() > 1) /* three children are allowed not considered a polytomy */
						hasPolytomies = true;
					}
				nchildren.pop();
				newickStream << ')';
				prevToken = NXS_TREE_CLOSE_PARENS_TOKEN;
				handled = true;
				}
			else if (t[0] == ':')
				{
				if (prevToken != NXS_TREE_CLOSE_PARENS_TOKEN && prevToken != NXS_TREE_CLADE_NAME_TOKEN)
					throw NxsException("Found a : separator for a subtree at an inappropriate location. A colon is only permitted after a clade name or )-symbol.", token);
				if (taxsetRead && prevToken == NXS_TREE_CLADE_NAME_TOKEN)
					throw NxsException("Found a : separator after a taxset name. Branch lengths cannot be assigned to multi-taxon taxsets.", token);
				newickStream << ':';
				prevToken = NXS_TREE_COLON_TOKEN;
				handled = true;
				token.SetLabileFlagBit(NxsToken::hyphenNotPunctuation); // this allows us to deal with sci. not. in branchlengths (and negative branch lengths).
				}
			else if (t[0] == ',')
				{
				if (prevToken == NXS_TREE_OPEN_PARENS_TOKEN)
					throw NxsException("Found a empty subclade found. The combination \"(,\" is prohibited.", token);
				if (prevToken == NXS_TREE_COMMA_TOKEN)
					throw NxsException("Found a empty subclade found. The combination \",,\" is prohibited.", token);
				if (prevToken == NXS_TREE_COLON_TOKEN)
					throw NxsException("Found a , when a branch length was expected found. The combination \":,\" is prohibited.", token);
				if (!someMissingEdgeLens && (prevToken == NXS_TREE_CLOSE_PARENS_TOKEN || prevToken == NXS_TREE_CLADE_NAME_TOKEN))
					someMissingEdgeLens = true;
				nchildren.top() += 1;
				newickStream << ',';
				prevToken = NXS_TREE_COMMA_TOKEN;
				handled = true;
				}
			}
		if (!handled)
			{
			if (prevToken == NXS_TREE_COLON_TOKEN)
				{
				bool handledLength = false;
				if (!someRealEdgeLens)
					{
					if (NxsString::to_long(t, &lastIntEdgeLen))
						{
						handledLength = true;
						if (lastIntEdgeLen < minIntEdgeLen)
							minIntEdgeLen = lastIntEdgeLen;
						}
					}
				if (!handledLength)
					{
					if (!NxsString::to_double(t, &lastFltEdgeLen))
						{
						errormsg << "Expecting a number as a branch length. Found " << tstr;
						throw NxsException(errormsg, token);
						}
					someRealEdgeLens = true;
					if (lastFltEdgeLen < minDblEdgeLen)
						minDblEdgeLen = lastFltEdgeLen;
					}
				newickStream << tstr;
				someHaveEdgeLens = true;
				prevToken = NXS_TREE_BRLEN_TOKEN;
				}
			else
				{
				if (prevToken == NXS_TREE_BRLEN_TOKEN || prevToken == NXS_TREE_CLADE_NAME_TOKEN)
					{
					errormsg << "Found a name " << tstr << " which should be preceded by a ( or a ,";
					throw NxsException(errormsg, token);
					}
				taxsetRead = false;
				NxsString ucl(t);
				if (!respectCase)
					ucl.ToUpper();
				NxsString toAppend;
				std::map<std::string, unsigned>::const_iterator tt = capNameToInd.find(ucl);
				unsigned ind = (tt == capNameToInd.end() ? UINT_MAX : tt->second);
				if (taxaEncountered.find(ind) != taxaEncountered.end())
					{
					errormsg << "Taxon number " << ind + 1 << " (coded by the token " << tstr << ") has already been encountered in this tree. Duplication of taxa in a tree is prohibited.";
					throw NxsException(errormsg, token);
					}
				if (prevToken == NXS_TREE_CLOSE_PARENS_TOKEN)
					{
					hasInternalLabels = true;
					if (ind == UINT_MAX)
						{
						hasInternalLabelsNotInTaxa = true;
						toAppend += tstr;
						}
					else
						{
						hasInternalLabelsInTaxa = true;
						taxaEncountered.insert(ind);
						toAppend += (1 + ind);
						}
					}
				else
					{
					if (ind == UINT_MAX)
						{
						std::set<unsigned> csinds;
						unsigned nadded = taxa->GetIndexSet(tstr, &csinds);
						if (nadded == 0)
							{
							if (!allowNewTaxa)
								{
								errormsg << "Expecting a Taxon label after a \"" << (prevToken == NXS_TREE_OPEN_PARENS_TOKEN ? '(' : ',') << "\" character. Found \"" << tstr << "\" but this is not a recognized taxon label.";
								throw NxsException(errormsg, token);
								}
							std::string tasstring(tstr.c_str());
							unsigned valueInd = taxa->AppendNewLabel(tasstring);
							NxsString numV;
							numV += (valueInd+1);
							if (capNameToInd.find(numV) == capNameToInd.end())
								capNameToInd[numV] = valueInd;
							if (!respectCase)
								NxsString::to_upper(tasstring);
							capNameToInd[tasstring] = valueInd;
							toAppend += (1 + valueInd);
							}
						else
							{
							bool firstTaxonAdded = true;
							for (std::set<unsigned>::const_iterator cit = csinds.begin(); cit != csinds.end(); ++cit)
								{
								if (taxaEncountered.find(*cit) != taxaEncountered.end())
									{
									errormsg << "Taxon number " << *cit + 1 << " (one of the members of the taxset " << tstr << ") has already been encountered in this tree. Duplication of taxa in a tree is prohibited.";
									throw NxsException(errormsg, token);
									}
								taxaEncountered.insert(*cit);
								if (!firstTaxonAdded)
									toAppend.append(1, ',');
								toAppend += (1 + *cit);
								firstTaxonAdded = false;
								}
							if (nadded > 1)
								{
								taxsetRead = true;
								someMissingEdgeLens = true;
								}
							}
						}
					else
						{
						taxaEncountered.insert(ind);
						toAppend += (1 + ind);
						}
					}
				newickStream << toAppend;
				prevToken = NXS_TREE_CLADE_NAME_TOKEN;
				}
			}
		token.GetNextToken();
		}
	td.flags |= NxsFullTreeDescription::NXS_TREE_PROCESSED;
	if (someHaveEdgeLens)
		{
		flags |= NxsFullTreeDescription::NXS_HAS_SOME_EDGE_LENGTHS_BIT;
		if (someRealEdgeLens)
			{
			flags &= ~(NxsFullTreeDescription::NXS_INT_EDGE_LENGTHS_BIT);
			td.minDblEdgeLen = minDblEdgeLen;
			}
		else
			{
			flags |= NxsFullTreeDescription::NXS_INT_EDGE_LENGTHS_BIT;
			td.minIntEdgeLen = minIntEdgeLen;
			}
		}
	td.newick = newickStream.str();
	if (someMissingEdgeLens)
		flags |= NxsFullTreeDescription::NXS_MISSING_SOME_EDGE_LENGTHS_BIT;
	if (hasPolytomies)
		flags |= NxsFullTreeDescription::NXS_HAS_POLYTOMY_BIT;
	if (hasDegTwoNodes)
		flags |= NxsFullTreeDescription::NXS_HAS_DEG_TWO_NODES_BIT;
	if (hasInternalLabels)
		{
		flags |= NxsFullTreeDescription::NXS_HAS_INTERNAL_NAMES_BIT;
		if (hasInternalLabelsNotInTaxa)
			flags |= NxsFullTreeDescription::NXS_HAS_NEW_INTERNAL_NAMES_BIT;
		if (hasInternalLabelsInTaxa)
			flags |= NxsFullTreeDescription::NXS_KNOWN_INTERNAL_NAMES_BIT;
		}
	if (NHXComments)
		flags |= NxsFullTreeDescription::NXS_HAS_NHX_BIT;
	if (taxaEncountered.size() == taxa->GetMaxIndex())
		flags |= NxsFullTreeDescription::NXS_HAS_ALL_TAXA_BIT;
	}

void NxsTreesBlock::ProcessTree(NxsFullTreeDescription & ftd) const
	{
	if (ftd.flags & NxsFullTreeDescription::NXS_TREE_PROCESSED)
		return;
	ftd.newick.append(1, ';');
	const std::string incomingNewick = ftd.newick;
	ftd.newick.clear();
	istringstream newickstream(incomingNewick);
	NxsToken token(newickstream);
	ProcessTokenStreamIntoTree(token, ftd, taxa, capNameToInd, constructingTaxaBlock, nexusReader);
	}

void NxsTreesBlock::HandleTreeCommand(NxsToken &token, bool rooted)
	{
	assert(taxa);
	token.GetNextToken();
	if (token.Equals("*"))
		{
		defaultTreeInd = (unsigned)trees.size();
		token.GetNextToken();
		}
	NxsString treeName = token.GetToken();
#	if defined(DEBUGGING_TREES_BLOCK) && DEBUGGING_TREES_BLOCK
		for (std::map<std::string, unsigned>::const_iterator tt =  capNameToInd.begin(); tt != capNameToInd.end(); ++tt)
			std::cerr << tt->first << " ==> " << 1+tt->second << '\n';
		std::cerr <<   '\n';
#	endif
	DemandEquals(token, "after tree name in TREE command");
	file_pos fp = 0;
	int fline = token.GetFileLine();
	int fcol = token.GetFileColumn();
	fp = token.GetFilePosition();
	try {
		// This should be either a tree description or a command comment specifying
		// whether this tree is to be rooted ([&R]) or unrooted ([&U]).
		//
		token.SetLabileFlagBit(NxsToken::saveCommandComments);
		token.SetLabileFlagBit(NxsToken::parentheticalToken);
		token.GetNextToken();
		NxsString s = token.GetToken();
		if (!s.empty() && s[0] == '&')
			{
			if (s[1] == 'R' || s[1] == 'r')
				rooted = true;
			else if (s[1] == 'U' || s[1] == 'u')
				rooted = false;
			else
				{
				errormsg << "[" << token.GetToken() << "] is not a valid command comment in a TREE command";
				throw NxsException(errormsg, token.GetFilePosition(), token.GetFileLine(), token.GetFileColumn());
				}
			// now grab the tree description
			token.SetLabileFlagBit(NxsToken::parentheticalToken);
			token.GetNextToken();
			s = token.GetToken();
			}
		if (!s.empty() && s[0] != '(')
			{
			errormsg << "Expecting command comment or tree description in TREE (or UTREE) command, but found " << token.GetToken() << " instead";
			throw NxsException(errormsg);
			}
		}
	catch (NxsX_UnexpectedEOF &)
		{
		errormsg << "Unexpected end of file in tree description.\n";
		errormsg << "This probably indicates that the parentheses in the newick description are not balanced, and one or more closing parentheses are needed.";
		throw NxsException(errormsg, fp, fline, fcol);
		}
	std::string mt;
	int f = (rooted ? NxsFullTreeDescription::NXS_IS_ROOTED_BIT : 0);
	trees.push_back(NxsFullTreeDescription(mt, treeName, f));
	NxsFullTreeDescription & td = trees[trees.size() -1];
	ostringstream newickStream;
	newickStream << token.GetTokenReference();
	token.GetNextToken();
	const std::vector<NxsComment> & ecs = token.GetEmbeddedComments();
	for (std::vector<NxsComment>::const_iterator ecsIt = ecs.begin(); ecsIt != ecs.end(); ++ecsIt)
		ecsIt->WriteAsNexus(newickStream);
	while (!token.Equals(";"))
		{
		if (token.Equals("(") || token.Equals(")") || token.Equals(","))
			GenerateUnexpectedTokenNxsException(token, "root taxon information");
		newickStream << token.GetTokenReference();
		token.GetNextToken();
		const std::vector<NxsComment> & iecs = token.GetEmbeddedComments();
		for (std::vector<NxsComment>::const_iterator iecsIt = iecs.begin(); iecsIt != iecs.end(); ++iecsIt)
			iecsIt->WriteAsNexus(newickStream);
		}
	td.newick = newickStream.str();
	if (processAllTreesDuringParse)
		{
		try
			{
			ProcessTree(td);
			}
		catch (NxsException &x)
			{
			x.pos += fp;
			x.line += fline - 1; /*both tokenizers start at 1 instead of zero, so we need to decrement the line */
			x.col += fcol;
			throw x;
			}
		}
	}
/*----------------------------------------------------------------------------------------------------------------------
|	This function provides the ability to read everything following the block name (which is read by the NxsReader
|	object) to the END or ENDBLOCK command. Characters are read from the input stream `in'. Overrides the abstract
|	virtual function in the base class.
*/
void NxsTreesBlock::Read(
  NxsToken &token)	/* the token used to read from `in' */
	{
	isEmpty = false;
	isUserSupplied = true;
	DemandEndSemicolon(token, "BEGIN TREES");
	//AssureTaxaBlock(createImpliedBlock, token, "BEGIN TREES");
	bool readTranslate = false;
	bool readTree = false;
	errormsg.clear();
	constructingTaxaBlock = false;
	newtaxa = false;
	capNameToInd.clear();
	for (;;)
		{
		token.GetNextToken();
		NxsBlock::NxsCommandResult res = HandleBasicBlockCommands(token);
		if (res == NxsBlock::NxsCommandResult(STOP_PARSING_BLOCK))
			{
			if (constructingTaxaBlock)
				{
				if (taxa && taxa->GetNTax() > 0)
					newtaxa = true;
				constructingTaxaBlock = false; /* we don't allow the construction of taxa blocks over repeated readings or after the block has been read */
				}
			return;
			}
		if (res != NxsBlock::NxsCommandResult(HANDLED_COMMAND))
			{
			if (token.Equals("TRANSLATE"))
				{
				if (readTree)
					WarnDangerousContent("TRANSLATE command must precede any TREE commands in a TREES block", token);
				if (readTranslate)
					{
					WarnDangerousContent("Only one TRANSLATE command may be read in a TREES block", token);
					capNameToInd.clear();
					}
				readTranslate = true;
				ConstructDefaultTranslateTable(token, "TRANSLATE");
				HandleTranslateCommand(token);
				}
			else
				{
				bool utreeCmd = token.Equals("UTREE");
				bool treeCmd = token.Equals("TREE");
				if (utreeCmd || treeCmd)
					{
					if (!readTranslate && ! readTree)
						ConstructDefaultTranslateTable(token, token.GetTokenReference().c_str());
					readTree = true;
					HandleTreeCommand(token, treeCmd);
					}
				else
					SkipCommand(token);
				}
			}
		}
	}
/*----------------------------------------------------------------------------------------------------------------------
|	Returns the description of the tree with index `i' where i is in [0..ntrees).
|	Node numbers will be translated to names in the resulting tree description.
|	Use GetTreeDescription if translation is not desired.
*/
NxsString NxsTreesBlock::GetTranslatedTreeDescription(
  unsigned i)	/* the index of the tree for which the description is to be returned */
	{
	assert(i < trees.size());
	assert(taxa);
	NxsFullTreeDescription & ftd = trees.at(i);
	ProcessTree(ftd);
	std::string incomingNewick = ftd.newick;
	incomingNewick.append(1, ';');
	istringstream newickstream(incomingNewick);
	NxsToken token(newickstream);
	token.GetNextToken();
	if (!token.Equals("("))
		{
		errormsg << "Expecting a ( to start the tree description, but found " << token.GetTokenReference();
		throw NxsException(errormsg, token);
		}
	int prevToken = NXS_TREE_OPEN_PARENS_TOKEN;
	long taxIndLong;
	const unsigned ntax = taxa->GetNTaxTotal();
	ostringstream translated;
	for (;;)
		{
		const std::vector<NxsComment> & ecs = token.GetEmbeddedComments();
		for (std::vector<NxsComment>::const_iterator ecsIt = ecs.begin(); ecsIt != ecs.end(); ++ecsIt)
			ecsIt->WriteAsNexus(translated);
		if (token.Equals(";"))
			break;
		const NxsString & t = token.GetTokenReference();
		bool handled;
		handled = false;
		if (t.length() == 1)
			{
			if (t[0] == '(')
				{
				translated <<  '(';
				prevToken = NXS_TREE_OPEN_PARENS_TOKEN;
				handled = true;
				}
			else if (t[0] == ')')
				{
				translated << ')';
				prevToken = NXS_TREE_CLOSE_PARENS_TOKEN;
				handled = true;
				}
			else if (t[0] == ':')
				{
				translated << ':';
				prevToken = NXS_TREE_COLON_TOKEN;
				handled = true;
				token.SetLabileFlagBit(NxsToken::hyphenNotPunctuation); // this allows us to deal with sci. not. in branchlengths (and negative branch lengths).
				}
			else if (t[0] == ',')
				{
				translated << ',';
				prevToken = NXS_TREE_COMMA_TOKEN;
				handled = true;
				}
			}
		if (!handled)
			{
			if (prevToken == NXS_TREE_COLON_TOKEN)
				{
				translated << t;
				prevToken = NXS_TREE_BRLEN_TOKEN;
				}
			else
				{
				if (NxsString::to_long(t.c_str(), &taxIndLong) && taxIndLong <= (long) ntax && taxIndLong > 0)
					translated << NxsString::GetEscaped(taxa->GetTaxonLabel((unsigned) taxIndLong - 1));
				else if (prevToken == NXS_TREE_CLOSE_PARENS_TOKEN)
					translated << t;
				else
					{
					errormsg << "Expecting a taxon index in a tree description, but found " << t;
					throw NxsException(errormsg, token);
					}
				}
			}
		token.GetNextToken();
		}
	return NxsString(translated.str().c_str());
	}

