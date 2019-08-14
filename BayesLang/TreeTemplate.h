#ifndef TREETEMPLATE_H
#define TREETEMPLATE_H

#include "Definitions.h"
#include "LanguageWithFeatures.h"
#include "ncl/nxstreesblock.h"
#include "LanguagesSet.h"
#include "FeatureValuesMatrix.h"
#include "FeatureRateMatricesSet.h"
#include "Permutation.h"

/***********************************************************
*
* A languages tree template (the tree is doubly linked)
*
***********************************************************/

#define NUMBER_OF_ALTERED_ANCESTRAL_STATES     1
#define NUMBER_OF_ALTERED_MISSING_VALUE_STATES 1
#define NUMBER_OF_ALTERED_BRANCHES             1
#define NUMBER_OF_ALTERED_RATE_MATRIX          1
#define TOTAL_NUMBER_OF_ALTERATIONS_IN_ONE_MOVE (NUMBER_OF_ALTERED_ANCESTRAL_STATES + NUMBER_OF_ALTERED_MISSING_VALUE_STATES + NUMBER_OF_ALTERED_BRANCHES + NUMBER_OF_ALTERED_RATE_MATRIX)


class TreeTemplate
{
    //Forward declaration of the branch class:
    public:
        class Branch;
    private:
        struct st_AncestralOrMissingValueStates;

    //The actual classes:
    public:
        //The node class:
        class Node
        {
            friend class TreeTemplate;
            public:
                Node( void );
                Node( unsigned iNumberOfDescendants );
                Node( const NxsSimpleNode *iRoot, LanguagesSet &iLanguagesSet, FeatureValuesMatrix &iFeatureValuesMatrix, FeaturesSet *iFeaturesSet, FeatureRateMatricesSet *iRateMatrices, TreeTemplate *iTreeTemplate );
                Node( Node &iNode, TreeTemplate *iTreeTemplate );
                virtual ~Node( void );

                void Init( unsigned iNumberOfDescendants );
                void SetAncestor( Branch *iAncestor );

                void Print( FILE *iFile, bool paranthesized, int iWhichFeature, int iMaxTreeDepth, int *iRemaningSibs, int iCurLevel, bool iPrintLikelihoods = false );
                unsigned Print( char *iBuffer, bool paranthesized, int iWhichFeature, int iMaxTreeDepth, int *iRemaningSibs, int iCurLevel, bool iPrintLikelihoods = false );
                void WriteParamsHeader( FILE *iFile );
                void WriteParamsValues( FILE *iFile );
                unsigned WriteParamsValues( char *iBuffer );

                int GetMaxTreeDepth( void );
                unsigned GetNumberOfNodes( void );
                void BuildNodesList( Node **ioNodesList, unsigned &ioCurrentNode );
                void MarkEdges( Node **ioNodesList, unsigned iNumberOfNodes, int **ioAdjacencymatrix );

                unsigned GetNDescendants( void ){ return m_NumberOfDescendants; }
                int GetFeatureValue( int iWhichFeature ){ return m_Language.GetFeatureValue( iWhichFeature ); }

                //Compute the loglikelihood for this tree template:
                DblType ComputeLogLikelihood( int iWhichFeature );
                void UpdateFullLogLikelihood( void );
                DblType GetLogLikelihood( int iWhichFeature ){ return iWhichFeature == -1 ? m_LogLikelihoods[m_RateMatrices->GetNFeatures()] : m_LogLikelihoods[iWhichFeature]; }

                //Generate values for the features with missing values (iWhichFeature = -1 == all features):
                void GenerateMissingValues( int iWhichFeature = (-1) );
                bool IsMissing( int iWhichFeature ){ return m_Language.IsMissing( iWhichFeature ); }

                //Generate a new candidate value for a given real feature:
                DblType NewCandidate( int iWhichFeature );

                //Get the language's name:
                const char *GetLanguageName( void ){ return m_Language.GetName(); }

                //Copy the node content:
                void CopyContent( Node &iNode );

                //Get the log of the prior probability of this node (for ratio computation, i.e. without any multiplicative constants):
                DblType GetLogPriorProbability( int iWhichFeature = (-1) );

                //Optimizations:
                bool HasBeenAltered( void ){ return m_HasBeenAltered; }
                void ResetAlterationFlag( void ){ m_HasBeenAltered = false; }
                void SetAlterationFlag( bool iFlag ){ m_HasBeenAltered = iFlag; }

                //Count the minimim number of changes taking place on this tree (parsimony-style):
                void CountChangesParsimony( unsigned *iCounter, bool iIgnoreMissingDataNodes );

                //Get the the minimim number of changes taking place on the hole path leading to this node (parsimony-style):
                unsigned GetNumberOfChangesParsimony( int iWhichFeature ){ return (iWhichFeature == (-1)) ? m_CountChanges[m_TreeTemplate->m_RateMatrices.GetNFeatures()] : m_CountChanges[iWhichFeature]; }

           private:
                //It contains a language:
                LanguageWithFeatures m_Language;

                //A number of descendant nodes (0 for terminals):
                unsigned m_NumberOfDescendants;
                Branch **m_Descendants;

                //And the (possible) ancestor:
                Branch *m_Ancestor;

                //The instantaneous rates matrix for the features:
                FeatureRateMatricesSet *m_RateMatrices;

                //The containing TreeTemplate:
                TreeTemplate *m_TreeTemplate;

                //This subtree's loglikelihood for each feature separately as well as for all features together (the last one); -1 == undefined:
                DblType *m_LogLikelihoods;

                //The minimum number of changes (parsimony-style) for each feature separately as well as for all features together (the last one); -1 == undefined:
                unsigned *m_CountChanges;

                //Optimizations: has this node been altered:
                bool m_HasBeenAltered;

                //The node index (used for serialization of the tree):
                unsigned m_NodeIndex;

            private:
                //Memory management:
                void AllocMemory( unsigned iNumberOfDescendants );
                void FreeMemory( void );

                //Pretty print: draw the lines:
                void DrawLines( int iMaxTreeDepth = 0, int *iRemaningSibs = NULL, int iCurLevel = 0, FILE *iFile = stdout, bool iSpacingLine = false );

                //Build the cached lists of paramteres (iActuallyBuildLists== true) or just count them (iActuallyBuildLists == false):
                void BuildCachedLists( st_AncestralOrMissingValueStates *ioAncestralStates, st_AncestralOrMissingValueStates *ioMisingValuesStates,
                                       unsigned &iNumberOfBranches, Branch **iBranches, bool iActuallyBuildLists );
        };

        //The branch class:
        class Branch
        {
            public:
                Branch( void );
                Branch( DblType iLength, Node *iAncestor, Node *iDescendant, FeatureRateMatricesSet *iRateMatrices, TreeTemplate *iTreeTemplate );
                virtual ~Branch( void );

                void Init( DblType iLength, Node *iAncestor, Node *iDescendant, FeatureRateMatricesSet *iRateMatrices,TreeTemplate *iTreeTemplate );

                DblType GetLength( void ){ return m_Length; }
                Node *GetDescendant( void ){ return m_Descendant; }
                Node *GetAncestor( void ){ return m_Ancestor; }

                //Compute the loglikelihood for this tree template:
                DblType ComputeLogLikelihood( int iWhichFeature );
                void UpdateFullLogLikelihood( void );
                DblType GetLogLikelihood( int iWhichFeature ){ return iWhichFeature == -1 ? m_LogLikelihoods[m_RateMatrices->GetNFeatures()] : m_LogLikelihoods[iWhichFeature]; }

                //Generate values for the features with missing values (iWhichFeature = -1 == all features):
                void GenerateMissingValues( int iWhichFeature = (-1) );

                //Generate a new candidate value:
                DblType NewCandidate( void );

                //Copy the vloatile info (branch length and likelihoods):
                void CopyContent( Branch &iBranch );

                //Get the log of the prior probability of this branch (for ratio computation, i.e. without any multiplicative constants):
                DblType GetLogPriorProbability( int iWhichFeature = (-1) );

                //Optimizations:
                bool HasBeenAltered( void ){ return m_HasBeenAltered; }
                void ResetAlterationFlag( void ){ m_HasBeenAltered = false; }
                void SetAlterationFlag( bool iFlag ){ m_HasBeenAltered = iFlag; }

                //Get the the minimim number of changes taking place on this branch (parsimony-style):
                unsigned GetNumberOfChangesParsimony( int iWhichFeature ){ return (iWhichFeature == (-1)) ? m_CountChanges[m_TreeTemplate->m_RateMatrices.GetNFeatures()] : m_CountChanges[iWhichFeature]; }
                //Count the minimim number of changes taking place on this tree (parsimony-style):
                void CountChangesParsimony( unsigned *iCounter, bool iIgnoreMissingDataNodes );

            private:
                //The branch length:
                DblType m_Length;

                //The origin node (NULL for the root):
                Node *m_Ancestor;

                //and the destination node:
                Node *m_Descendant;

                //The instantaneous rates matrix for the features:
                FeatureRateMatricesSet *m_RateMatrices;

                //This branch's loglikeliood for each feature separately as well as for all features together (the last one); -1 == undefined:
                DblType *m_LogLikelihoods;

                //The branch's minimum number of changes (parsimony-style) for each feature separately as well as for all features together (the last one); -1 == undefined:
                unsigned *m_CountChanges;

                //The containing TreeTemplate:
                TreeTemplate *m_TreeTemplate;

                //Optimizations: has this branch been altered:
                bool m_HasBeenAltered;
        };

    public:
        TreeTemplate();
        virtual ~TreeTemplate();

        void Print( FILE *iFile, bool paranthesized = true, int iWhichFeature = (-1), bool iPrintLikelihoods = false, bool iPrintRateMatrices = false ); //iWhichFeature = (-1) == all features
        unsigned Print( char *iBuffer, bool paranthesized = true, int iWhichFeature = (-1), bool iPrintLikelihoods = false, bool iPrintRateMatrices = false ); //iWhichFeature = (-1) == all features
        void WriteParamsHeader( FILE *iFile );
        void WriteParamsValues( FILE *iFile );
        unsigned WriteParamsValues( char *iBuffer );
        void WriteStructure( FILE *iFile );

        void SetName( const char *iName );
        const char *GetName( void ){ return m_Name; }

        //Recursively add a tree:
        void AddTree( const NxsSimpleNode *iRoot, LanguagesSet &iLanguagesSet, FeatureValuesMatrix &iFeatureValuesMatrix, FeaturesSet *iFeaturesSet );

        int GetMaxTreeDepth( void );
        unsigned GetNumberOfNodes( void ){ return !m_Root ? 0 : m_Root->GetNumberOfNodes(); }

        FeatureRateMatricesSet *GetFeatureRateMatricesSet( void ){ return &m_RateMatrices; }

        //Compute the likelihood for this tree template (iWhichFeature = -1 == all features):
        DblType ComputeLikelihood( int iWhichFeature = (-1) );
        DblType GetLogLikelihood( int iWhichFeature ){ return iWhichFeature == -1 ? m_LogLikelihoods[m_RateMatrices.GetNFeatures()] : m_LogLikelihoods[iWhichFeature]; }

        //Generate values for the features with missing values (iWhichFeature = -1 == all features):
        void GenerateMissingValues( int iWhichFeature = (-1) );

        //Generate a new candidate tree (for the given feature(s)) from the current one,
        //given the probabilities of altering the ancestral states, missing values, branch lengths and the rate matrix;
        //return the Hasting Ratio for this move or (-1) if the move was unseccessful:
        DblType NewCandidate( int iWhichFeature = (-1), DblType iAlterAncestralProb = 1.0,
                           DblType iAlterMissingProb = 1.0, DblType iAlterBranchProb = 1.0,
                           DblType iAlterRatesProb = 1.0 );

        //Copy operators:
        TreeTemplate &operator=( const TreeTemplate &iTree );
        TreeTemplate &CopyContent( const TreeTemplate &iTree );

        //Get the log of the prior probability of this tree (for ratio computation, i.e. without any multiplicative constants):
        DblType GetLogPriorProbability( int iWhichFeature = (-1) );

        //Get the the minimim number of changes taking place on this tree (parsimony-style):
        unsigned GetNumberOfChangesParsimony( int iWhichFeature ){ return (iWhichFeature == (-1)) ? m_CountChanges[m_RateMatrices.GetNFeatures()] : m_CountChanges[iWhichFeature]; }
        //Given a tree, count the minimim number of changes taking place on this tree (parsimony-style):
        void CountChangesParsimony( bool iIgnoreMissingDataNodes = true );

    protected:
    private:
        //The tree's root:
        Node *m_Root;

        //The tree's name (if any):
        char *m_Name;

        //The instantaneous rates matrix for the features:
        FeatureRateMatricesSet m_RateMatrices;

        //This tree's loglikeliood for each feature separately as well as for all features together (the last one); -1 == undefined:
        DblType *m_LogLikelihoods;

        //This tree's minimum number of changes (parsimony-style) for each feature separately as well as for all features together (the last one); -1 == undefined:
        unsigned *m_CountChanges;

        //Candidate tree generation by components (return the Hastings ratio or -1):
        DblType AlterAncestralStates( int iWhichFeature = (-1), unsigned iHowManyStates2Alter = 1 );
        DblType AlterMissingValueStates( int iWhichFeature = (-1), unsigned iHowManyStates2Alter = 1 );
        DblType AlterBeanchLengths( unsigned iHowManyBranches2Alter = 1 );
        DblType AlterRatesMatrix( int iWhichFeature );

        //The list of ancestral and missing value states for each feature:
        struct st_AncestralOrMissingValueStates
        {
            st_AncestralOrMissingValueStates(void):m_NumberOfStates(0),m_States(NULL){}
            st_AncestralOrMissingValueStates( unsigned iNumberOfStates ):m_NumberOfStates(0),m_States(NULL){AllocMemory(iNumberOfStates);}
            ~st_AncestralOrMissingValueStates(void){FreeMemory();}
            void AllocMemory( unsigned iNumberOfStates )
            {
                FreeMemory();
                if( (m_NumberOfStates=iNumberOfStates) > 0 )
                {
                    m_States = new Node*[ m_NumberOfStates ];
                    if( !m_States )
                    {
                        cerr << "Not enough memory!" << endl;
                        assert( false );
                        exit( 1 );
                    }
                }
                m_Permutation.Initialize( m_NumberOfStates );
            }
            void FreeMemory( void ){if(m_NumberOfStates && m_States) delete[] m_States; m_NumberOfStates = 0; m_States = NULL; }

            //The actual states:
            unsigned m_NumberOfStates;
            Node **m_States;

            //The associated permutation:
            Permutation m_Permutation;
        };
        st_AncestralOrMissingValueStates *m_AncestralStates; //for all features
        st_AncestralOrMissingValueStates *m_MissingValueStates; //for all features

        //The list of branches:
        unsigned m_NumberOfBranches;
        Branch **m_Branches;
        Permutation m_BranchesPermutation;

        //Build these lists:
        void BuildCachedLists( void );

        //Memory management:
        void FreeMemory( void );

    public:
        //The branch length exponantial prior lambda:
        static DblType st_BranchLamba;
        //The parameter lambda for the multiplier distribution for generating new proposal branch length:
        static DblType st_MultiplierLamba;
};

#endif // TREETEMPLATE_H
