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
#ifndef NCL_NXSDEFS_H
#define NCL_NXSDEFS_H

#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <list>
#include <utility>

#define NCL_MAJOR_VERSION 2
#define NCL_MINOR_VERSION 1
#define NCL_NAME_AND_VERSION  "NCL version 2.1.00"
#define NCL_COPYRIGHT         "Copyright (c) 1999-2003 by Paul O. Lewis"
#define NCL_HOMEPAGEURL       "http://sourceforge.net/projects/ncl"

// Maximum number of states that can be stored; the only limitation is that this
// number be less than the maximum size of an int (not likely to be a problem).
// A good number for this is 76, which is 96 (the number of distinct symbols
// able to be input from a standard keyboard) less 20 (the number of symbols
// symbols disallowed by the NEXUS standard for use as state symbols)
//
#define NCL_MAX_STATES         76

typedef std::streampos	file_pos;

#define	SUPPORT_OLD_NCL_NAMES

class NxsString;

typedef std::vector<bool> NxsBoolVector;
typedef std::vector<char> NxsCharVector;
typedef std::vector<int> NxsIntVector;
typedef std::vector<unsigned> NxsUnsignedVector;
typedef std::vector<NxsString> NxsStringVector;
typedef std::vector<NxsStringVector> NxsAllelesVector;

typedef std::set<unsigned> NxsUnsignedSet;

typedef std::map< unsigned, NxsStringVector> NxsStringVectorMap;
typedef std::map< NxsString, NxsString> NxsStringMap;
typedef std::map< NxsString, NxsUnsignedSet> NxsUnsignedSetMap;

typedef std::pair<std::string, NxsUnsignedSet> NxsPartitionGroup;
typedef std::list<NxsPartitionGroup> NxsPartition;
typedef std::map<std::string, NxsPartition> NxsPartitionsByName;

// The following typedefs are simply for maintaining compatibility with existing code.
// The names on the right are deprecated and should not be used.
//
typedef NxsBoolVector BoolVect;
typedef NxsUnsignedSet IntSet;
typedef NxsUnsignedSetMap IntSetMap;
typedef NxsAllelesVector AllelesVect;
typedef NxsStringVector LabelList;
typedef NxsStringVector StrVec;
typedef NxsStringVector vecStr;
typedef NxsStringVectorMap LabelListBag;
typedef NxsStringMap AssocList;

class ProcessedNxsToken;
typedef std::vector<ProcessedNxsToken> ProcessedNxsCommand;

#endif
