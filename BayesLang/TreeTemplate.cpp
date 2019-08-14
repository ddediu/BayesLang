#include "TreeTemplate.h"

//The branch length exponantial prior lambda:
DblType TreeTemplate::st_BranchLamba = gl_BranchLengthPriorExponentialLambda;

//The parameter lambda for the multiplier distribution for generating new proposal branch length:
DblType TreeTemplate::st_MultiplierLamba = 2*log( gl_BranchLengthProposalMultiplierDistributionParam );


TreeTemplate::TreeTemplate()
:m_Root(NULL),m_Name(NULL),m_LogLikelihoods(NULL),m_CountChanges(NULL),m_AncestralStates(NULL),m_MissingValueStates(NULL),m_NumberOfBranches(0),m_Branches(NULL)
{
}

TreeTemplate::~TreeTemplate()
{
    FreeMemory();
}

//Memory management:
void TreeTemplate::FreeMemory( void )
{
    if( m_Name )
    {
        delete[] m_Name;
    }
    m_Name = NULL;

    if( m_LogLikelihoods )
    {
        delete[] m_LogLikelihoods;
    }
    m_LogLikelihoods = NULL;

    if( m_CountChanges )
    {
        delete[] m_CountChanges;
    }
    m_CountChanges = NULL;

    if( m_AncestralStates )
    {
        delete[] m_AncestralStates;
    }
    m_AncestralStates = NULL;

    if( m_MissingValueStates )
    {
        delete[] m_MissingValueStates;
    }
    m_MissingValueStates = NULL;

    if( m_NumberOfBranches && m_Branches )
    {
        delete[] m_Branches;
    }
    m_NumberOfBranches = 0;
    m_Branches = NULL;

    if( m_Root )
    {
        delete m_Root;
    }
    m_Root = NULL;
}

void TreeTemplate::Print( FILE *iFile, bool paranthesized, int iWhichFeature, bool iPrintLikelihoods, bool iPrintRateMatrices )
{
    //Print the tree's name:
    if( m_Name && *m_Name )
    {
        fprintf( iFile, "%s:", m_Name );
        if( !paranthesized )
        {
            fprintf( iFile, "\n\n" );
        }
    }

    //Print the rate matrix/matrices for the requested feature(s):
    if( iPrintRateMatrices )
    {
        m_RateMatrices.Print( iFile, iWhichFeature );
    }

    //Print the tree:
    int maxTreeDepth = 0;
    int *remaningSibs = NULL;
    int i;
    if( !paranthesized )
    {
        maxTreeDepth = GetMaxTreeDepth();
        if( maxTreeDepth > 0 )
        {
            remaningSibs = new int[ maxTreeDepth ];
            if( !remaningSibs )
            {
                cerr << "Not enough memory!" << endl;
                assert( false );
                exit( 1 );
            }
            for( i = 0; i < maxTreeDepth; ++i )
            {
                remaningSibs[i] = 0;
            }
        }
    }

    if( m_Root )
    {
        m_Root->Print( iFile, paranthesized, iWhichFeature, maxTreeDepth, remaningSibs, 0, iPrintLikelihoods );
    }
    else
    {
        fprintf( iFile, "()" );
    }

    if( maxTreeDepth && remaningSibs )
    {
        delete[] remaningSibs;
    }
}

unsigned TreeTemplate::Print( char *iBuffer, bool paranthesized, int iWhichFeature, bool iPrintLikelihoods, bool iPrintRateMatrices )
{
    char *curChar = iBuffer;

    //Print the tree's name:
    if( m_Name && *m_Name )
    {
        curChar += sprintf( curChar, "%s:", m_Name );
        if( !paranthesized )
        {
            curChar += sprintf( curChar, "\n\n" );
        }
    }

    //Print the rate matrix/matrices for the requested feature(s):
    if( iPrintRateMatrices )
    {
        curChar += m_RateMatrices.Print( curChar, iWhichFeature );
    }

    //Print the tree:
    int maxTreeDepth = 0;
    int *remaningSibs = NULL;
    int i;
    if( !paranthesized )
    {
        maxTreeDepth = GetMaxTreeDepth();
        if( maxTreeDepth > 0 )
        {
            remaningSibs = new int[ maxTreeDepth ];
            if( !remaningSibs )
            {
                cerr << "Not enough memory!" << endl;
                assert( false );
                exit( 1 );
            }
            for( i = 0; i < maxTreeDepth; ++i )
            {
                remaningSibs[i] = 0;
            }
        }
    }

    if( m_Root )
    {
        curChar += m_Root->Print( curChar, paranthesized, iWhichFeature, maxTreeDepth, remaningSibs, 0, iPrintLikelihoods );
    }
    else
    {
        curChar += sprintf( curChar, "()" );
    }

    if( maxTreeDepth && remaningSibs )
    {
        delete[] remaningSibs;
    }

    return curChar - iBuffer;
}

void TreeTemplate::SetName( const char *iName )
{
    if( m_Name )
    {
        delete[] m_Name;
    }
    m_Name = NULL;

    if( iName )
    {
        m_Name = new char[ strlen(iName) + 1 ];
        if( !m_Name )
        {
            cerr << "Not enough memory!" << endl;
            assert( false );
            exit( 1 );
        }
        strcpy( m_Name, iName );
    }
}

//Recursively add a tree:
void TreeTemplate::AddTree( const NxsSimpleNode *iRoot, LanguagesSet &iLanguagesSet, FeatureValuesMatrix &iFeatureValuesMatrix, FeaturesSet *iFeaturesSet )
{
    //Free any previously allocated tree:
    if( m_Root )
    {
        delete m_Root;
    }
    m_Root = NULL;


    if( !iRoot )
    {
        //The game is over
        return;
    }

    //Set up the rate matrices:
    m_RateMatrices.Init( *iFeaturesSet );

    //Create this tree:
    m_Root = new Node( iRoot, iLanguagesSet, iFeatureValuesMatrix, iFeaturesSet, &m_RateMatrices, this );
    if( !m_Root )
    {
        cerr << "Not enough memory!" << endl;
        assert( false );
        exit( 1 );
    }

    //Synchronize the ancestor info:
    m_Root->SetAncestor( NULL );

    if( m_LogLikelihoods )
    {
        delete[] m_LogLikelihoods;
    }
    m_LogLikelihoods = new DblType[ m_RateMatrices.GetNFeatures() + 1];
    if( !m_LogLikelihoods )
    {
        cerr << "Not enough memory!" << endl;
        assert( false );
        exit( 1 );
    }

    if( m_CountChanges )
    {
        delete[] m_CountChanges;
    }
    m_CountChanges = new unsigned[ m_RateMatrices.GetNFeatures() + 1];
    if( !m_CountChanges )
    {
        cerr << "Not enough memory!" << endl;
        assert( false );
        exit( 1 );
    }
    for( unsigned i = 0; i <= m_RateMatrices.GetNFeatures(); ++i )
    {
        m_LogLikelihoods[i] = m_CountChanges[i] = -1;
    }

    //Build the case lists of ancestral & missing values states and branches:
    BuildCachedLists();
}

//Build these lists:
void TreeTemplate::BuildCachedLists( void )
{
    //Free the previously allocated memory:
    if( m_AncestralStates )
    {
        delete[] m_AncestralStates;
    }

    if( m_MissingValueStates )
    {
        delete[] m_MissingValueStates;
    }

    if( m_NumberOfBranches && m_Branches )
    {
        delete[] m_Branches;
    }

    m_AncestralStates = new st_AncestralOrMissingValueStates[ m_RateMatrices.GetNFeatures() ];
    if( !m_AncestralStates )
    {
        cerr << "Not enough memory!" << endl;
        assert( false );
        exit( 1 );
    }
    m_MissingValueStates = new st_AncestralOrMissingValueStates[ m_RateMatrices.GetNFeatures() ];
    if( !m_MissingValueStates )
    {
        cerr << "Not enough memory!" << endl;
        assert( false );
        exit( 1 );
    }

    m_NumberOfBranches = 0;
    m_Branches = NULL;

    //Recursively build these lists:
    if( !m_Root )
    {
        //Nothing to build!
        return;
    }

    //Get the length of the lists first:
    m_Root->BuildCachedLists( m_AncestralStates, m_MissingValueStates, m_NumberOfBranches, NULL, false );

    //Alloc the required memory:
    unsigned i;
    for( i = 0; i < m_RateMatrices.GetNFeatures(); ++i )
    {
        m_AncestralStates[i].AllocMemory( m_AncestralStates[i].m_NumberOfStates );
        m_MissingValueStates[i].AllocMemory( m_MissingValueStates[i].m_NumberOfStates );

        //Prepare them for the actual building:
        m_AncestralStates[i].m_NumberOfStates = 0;
        m_MissingValueStates[i].m_NumberOfStates = 0;
    }

    if( m_NumberOfBranches > 0 )
    {
        m_Branches = new Branch*[ m_NumberOfBranches ];
        if( !m_Branches )
        {
            cerr << "Not enough memory!" << endl;
            assert( false );
            exit( 1 );
        }
        m_BranchesPermutation.Initialize( m_NumberOfBranches );
    }
    //Prepare them for the actual building:
    m_NumberOfBranches = 0;

    //And build the lists:
    m_Root->BuildCachedLists( m_AncestralStates, m_MissingValueStates, m_NumberOfBranches, m_Branches, true );

    /*//DEBUG:
    unsigned j;
    cout << "Cached stuff:" << endl;
    for( i = 0; i < m_RateMatrices.GetNFeatures(); ++i )
    {
        cout << m_AncestralStates[i].m_NumberOfStates << " ancestral states for feature " << i << ": ";
        for( j = 0; j < m_AncestralStates[i].m_NumberOfStates; ++j )
        {
            cout << m_AncestralStates[i].m_States[j]->m_Language.GetName() << " ";
        }
        cout <<  endl;

        cout << m_MissingValueStates[i].m_NumberOfStates << " missing value states for feature " << i << ": ";
        for( j = 0; j < m_MissingValueStates[i].m_NumberOfStates; ++j )
        {
            cout << m_MissingValueStates[i].m_States[j]->m_Language.GetName() << " ";
        }
        cout << endl;
    }

    cout << m_NumberOfBranches << " branches :" << endl;
    for( i = 0; i < m_NumberOfBranches; ++i )
    {
        cout << m_Branches[i] << " ";
    }
    cout << endl;*/
}

int TreeTemplate::GetMaxTreeDepth( void )
{
    if( !m_Root )
    {
        return 0;
    }
    else
    {
        return m_Root->GetMaxTreeDepth();
    }
}




//The Node:
TreeTemplate::Node::Node( void )
:m_NumberOfDescendants(0),m_Descendants(NULL),m_Ancestor(NULL),m_RateMatrices(NULL),m_LogLikelihoods(NULL),m_CountChanges(NULL),m_HasBeenAltered(false)
{
}

TreeTemplate::Node::Node( unsigned iNumberOfDescendants )
:m_NumberOfDescendants(0),m_Descendants(NULL),m_Ancestor(NULL),m_RateMatrices(NULL),m_LogLikelihoods(NULL),m_CountChanges(NULL),m_HasBeenAltered(false)
{
    Init( iNumberOfDescendants );
}

TreeTemplate::Node::Node( const NxsSimpleNode *iRoot, LanguagesSet &iLanguagesSet, FeatureValuesMatrix &iFeatureValuesMatrix, FeaturesSet *iFeaturesSet, FeatureRateMatricesSet *iRateMatrices, TreeTemplate *iTreeTemplate )
:m_Language(iFeatureValuesMatrix,iFeaturesSet),m_NumberOfDescendants(0),m_Descendants(NULL),m_Ancestor(NULL),m_RateMatrices(iRateMatrices),m_TreeTemplate(iTreeTemplate),m_LogLikelihoods(NULL),m_CountChanges(NULL),m_HasBeenAltered(false)
{
    if( !iRoot )
    {
        //Game over!
        return;
    }

    std::vector<NxsSimpleNode *> children = iRoot->GetChildren();
    Init( children.size() );

    NxsSimpleEdge edge2parent = iRoot->GetEdgeToParent();
    //cout << "[" << nodes[i]->GetTaxIndex() << "," << ((nodes[i]->GetName() && *nodes[i]->GetName()) ? nodes[i]->GetName() : m_LanguagesSet.GetLanguage(nodes[i]->GetTaxIndex())->GetName()) << ":" << edge.GetDblEdgeLen() << "] ";

    const char *languageName = NULL;
    if( iRoot->GetName() && *iRoot->GetName() )
    {
        languageName = iRoot->GetName();
    }
    else
    {
        if( iLanguagesSet.GetLanguage(iRoot->GetTaxIndex()) )
        {
            languageName = iLanguagesSet.GetLanguage(iRoot->GetTaxIndex())->GetName();
        }
        else
        {
            //Create a new language name:
            char buffer[51];
            sprintf( buffer, "L%d", iRoot->GetTaxIndex() );
            languageName = buffer;
        }
    }
    //cout << "Adding node " << languageName << " with " << children.size() << " descendants" << endl;

    //Test if this language already exists:
    if( iLanguagesSet.GetLanguage(iRoot->GetTaxIndex()) )
    {
        //Copy the info related to it:
        m_Language = *iLanguagesSet.GetLanguage(iRoot->GetTaxIndex());
    }
    else
    {
        //It's a new one:
        m_Language.Init( languageName );
    }

    //Process the children:
    for( unsigned i = 0; i < children.size(); ++i )
    {
        //Create the descendant nodes: one by one:
        Node *curDescendant = new Node( children[i], iLanguagesSet, iFeatureValuesMatrix, iFeaturesSet, m_RateMatrices, m_TreeTemplate );
        if( !curDescendant )
        {
            cerr << "Not enough memory!" << endl;
            assert( false );
            exit( 1 );
        }

        //Update the branch:
        m_Descendants[i] = new Branch( children[i]->GetEdgeToParent().GetDblEdgeLen(), this, curDescendant, m_RateMatrices, m_TreeTemplate );
        if( !m_Descendants[i] )
        {
            cerr << "Not enough memory!" << endl;
            assert( false );
            exit( 1 );
        }
   }

   //Alloc the likelihoods:
   m_LogLikelihoods = new DblType[ m_RateMatrices->GetNFeatures()+1 ];
   if( !m_LogLikelihoods )
   {
       cerr << "Not enough memory!" << endl;
       assert( false );
       exit( 1 );
   }

   //Alloc the number of changes:
   m_CountChanges = new unsigned[ m_RateMatrices->GetNFeatures()+1 ];
   if( !m_CountChanges )
   {
       cerr << "Not enough memory!" << endl;
       assert( false );
       exit( 1 );
   }
}

TreeTemplate::Node::Node( Node &iNode, TreeTemplate *iTreeTemplate )
:m_Language(iNode.m_Language),m_NumberOfDescendants(0),m_Descendants(NULL),m_Ancestor(NULL),
 m_RateMatrices(&iTreeTemplate->m_RateMatrices),m_TreeTemplate(iTreeTemplate),m_LogLikelihoods(NULL),m_CountChanges(NULL),m_HasBeenAltered(false)
{

    unsigned i;
    //The descendants:
    m_NumberOfDescendants = iNode.m_NumberOfDescendants;
    if( m_NumberOfDescendants > 0 )
    {
        m_Descendants = new Branch*[m_NumberOfDescendants];
        if( !m_Descendants )
        {
            cerr << "Not enough memory!" << endl;
            assert( false );
            exit( 1 );
        }
    }

    //Recursively build the descendants:
    for( i = 0; i < m_NumberOfDescendants; ++i )
    {
        Node *curDescendant = new Node( *iNode.m_Descendants[i]->GetDescendant(), iTreeTemplate );
        if( !curDescendant )
        {
            cerr << "Not enough memory!" << endl;
            assert( false );
            exit( 1 );
        }
        /*//DEBUG
        cout << "Allocating descendat " << i << ":" <<  iNode.m_Descendants[i]->GetDescendant() << ":";
        iNode.m_Descendants[i]->GetDescendant()->Print( stdout, true, 0,0,0,0 );
        cout << endl;
        cout << "Allocated descendat " << i << ":" <<  curDescendant << ":";
        curDescendant->Print( stdout, true, 0,0,0,0 );
        cout << endl;
        //END DEBUG*/


        //Update the branch:
        m_Descendants[i] = new Branch( iNode.m_Descendants[i]->GetLength(), this, curDescendant, m_RateMatrices, m_TreeTemplate );
        if( !m_Descendants[i] )
        {
            cerr << "Not enough memory!" << endl;
            assert( false );
            exit( 1 );
        }
        m_Descendants[i]->SetAlterationFlag( iNode.m_Descendants[i]->HasBeenAltered() );
    }

    //The likelihoods:
    m_LogLikelihoods = new DblType[ m_RateMatrices->GetNFeatures()+1 ];
    if( !m_LogLikelihoods )
    {
        cerr << "Not enough memory!" << endl;
        assert( false );
        exit( 1 );
    }

    //The number of changes:
    m_CountChanges = new unsigned[ m_RateMatrices->GetNFeatures()+1 ];
    if( !m_CountChanges )
    {
        cerr << "Not enough memory!" << endl;
        assert( false );
        exit( 1 );
    }

    for( i = 0; i <= m_RateMatrices->GetNFeatures(); ++i )
    {
        m_LogLikelihoods[i] = iNode.m_LogLikelihoods[i];
        m_CountChanges[i] = iNode.m_CountChanges[i];
    }

    m_HasBeenAltered = iNode.m_HasBeenAltered;
}

//Recursively set the ancestor branches:
void TreeTemplate::Node::SetAncestor( Branch *iAncestor )
{
    m_Ancestor = iAncestor;

    for( unsigned i = 0; i < m_NumberOfDescendants; ++i )
    {
        m_Descendants[i]->GetDescendant()->SetAncestor( m_Descendants[i] );
    }
}

TreeTemplate::Node::~Node( void )
{
    //Free the entire tree:
    FreeMemory();
}

void TreeTemplate::Node::Init( unsigned iNumberOfDescendants )
{
    AllocMemory( iNumberOfDescendants );
}

void TreeTemplate::Node::AllocMemory( unsigned iNumberOfDescendants )
{
    //Preconditons:
    //assert( iNumberOfDescendants > 0 );

    //Alloc any already occupied memory:
    FreeMemory();

    m_NumberOfDescendants = iNumberOfDescendants;
    if( m_NumberOfDescendants > 0 )
    {
        m_Descendants = new Branch*[ m_NumberOfDescendants ];
        if( !m_Descendants )
        {
            cerr << "Not enough memory!" << endl;
            assert( false );
            exit( 1 );
        }
    }
}

void TreeTemplate::Node::FreeMemory( void )
{
    if( m_NumberOfDescendants && m_Descendants )
    {
        for( unsigned i = 0; i < m_NumberOfDescendants; ++i )
        {
            if( m_Descendants[i] )
            {
                delete m_Descendants[i];
            }
        }

        delete[] m_Descendants;
    }

    m_NumberOfDescendants = 0;
    m_Descendants = NULL;

    if( m_LogLikelihoods )
    {
        delete[] m_LogLikelihoods;
    }
    m_LogLikelihoods = NULL;

    if( m_CountChanges )
    {
        delete[] m_CountChanges;
    }
    m_CountChanges = NULL;
}

void TreeTemplate::Node::Print( FILE *iFile, bool paranthesized, int iWhichFeature, int iMaxTreeDepth, int *iRemaningSibs, int iCurLevel, bool iPrintLikelihoods )
{
    if( paranthesized )
    {
        if( m_NumberOfDescendants == 0 )
        {
            //A terminal: print it and return!
            m_Language.Print( iFile, iWhichFeature );
            fprintf( iFile, ":%.3f", m_Ancestor ? m_Ancestor->GetLength() : 0.0 );
        }
        else
        {
            //Print the descendants left->right
            fprintf( iFile, "(" );
            for( unsigned i  = 0; i < m_NumberOfDescendants; ++i )
            {
                m_Descendants[i]->GetDescendant()->Print( iFile, paranthesized, iWhichFeature, iMaxTreeDepth, iRemaningSibs, iCurLevel );
                if( i < m_NumberOfDescendants-1 )
                {
                    fprintf( iFile, "," );
                }
            }
            fprintf( iFile, ")" );
            m_Language.Print( iFile, iWhichFeature );
            fprintf( iFile, ":%.3f", m_Ancestor ? m_Ancestor->GetLength() : 0.0 );
        }
    }
    else
    {
        //Print the descendants left->right
        DrawLines( iMaxTreeDepth, iRemaningSibs, iCurLevel, iFile, true );
        fprintf( iFile, "\n" );
        DrawLines( iMaxTreeDepth, iRemaningSibs, iCurLevel, iFile, false );
        m_Language.Print( iFile, iWhichFeature, true );
        fprintf( iFile, " t=%.3f", m_Ancestor ? m_Ancestor->GetLength() : 0.0 );
        if( iPrintLikelihoods )
        {
            if( m_Ancestor )
            {
                //Only for nodes having ancestors:
                fprintf( iFile, " L(b,%s)=%.4f", iWhichFeature == (-1) ? "all" : m_RateMatrices->GetFeature( iWhichFeature )->GetName(),
                                               m_Ancestor->GetLogLikelihood(iWhichFeature) );
            }
            if( m_NumberOfDescendants )
            {
                //Only for nodes with descendants:
                fprintf( iFile, " L(s,%s)=%.4f", iWhichFeature == (-1) ? "all" : m_RateMatrices->GetFeature( iWhichFeature )->GetName(),
                                               GetLogLikelihood(iWhichFeature) );
            }
        }
        fprintf( iFile, "\n" );
        for( unsigned i = 0; i < m_NumberOfDescendants; ++i )
        {
            iRemaningSibs[iCurLevel] = m_NumberOfDescendants-i;
            m_Descendants[i]->GetDescendant()->Print( iFile, paranthesized, iWhichFeature, iMaxTreeDepth, iRemaningSibs, iCurLevel+1, iPrintLikelihoods );
        }
    }
}

unsigned TreeTemplate::Node::Print( char *iBuffer, bool paranthesized, int iWhichFeature, int iMaxTreeDepth, int *iRemaningSibs, int iCurLevel, bool iPrintLikelihoods )
{
    char *curChar = iBuffer;

    if( paranthesized )
    {
        if( m_NumberOfDescendants == 0 )
        {
            //A terminal: print it and return!
            curChar += m_Language.Print( curChar, iWhichFeature );
            curChar += sprintf( curChar, ":%.3f", m_Ancestor ? m_Ancestor->GetLength() : 0.0 );
        }
        else
        {
            //Print the descendants left->right
            curChar += sprintf( curChar, "(" );
            for( unsigned i  = 0; i < m_NumberOfDescendants; ++i )
            {
                curChar += m_Descendants[i]->GetDescendant()->Print( curChar, paranthesized, iWhichFeature, iMaxTreeDepth, iRemaningSibs, iCurLevel );
                if( i < m_NumberOfDescendants-1 )
                {
                    curChar += sprintf( curChar, "," );
                }
            }
            curChar += sprintf( curChar, ")" );
            curChar += m_Language.Print( curChar, iWhichFeature );
            curChar += printf( curChar, ":%.3f", m_Ancestor ? m_Ancestor->GetLength() : 0.0 );
        }
    }
    else
    {
        //No pretty print to string!
        assert( false );
        exit( 1 );
    }

    return curChar - iBuffer;
}

//Pretty print: draw the lines:
void TreeTemplate::Node::DrawLines( int iMaxTreeDepth, int *iRemaningSibs, int iCurLevel, FILE *iFile, bool iSpacingLine )
{
    for( int i = 0; i < min(iCurLevel,iMaxTreeDepth); ++i )
    {
        if( iRemaningSibs[i] == 0 )
        {
            fprintf( iFile, "   " );
        }
        else if( iRemaningSibs[i] == 1 )
        {
            fprintf( iFile, (i < min(iCurLevel,iMaxTreeDepth)-1) ? "   " : (iSpacingLine ? "|  " : "\\--") );
        }
        else
        {
            fprintf( iFile, (i < min(iCurLevel,iMaxTreeDepth)-1) ? "|  " : (iSpacingLine ? "|  " : "+--") );
        }
    }
}

int TreeTemplate::Node::GetMaxTreeDepth( void )
{
    int maxDepth = 0;
    for( unsigned i = 0; i < m_NumberOfDescendants; ++i )
    {
        maxDepth = max( maxDepth, m_Descendants[i]->GetDescendant()->GetMaxTreeDepth() );
    }

    return maxDepth+1;
}



//The branch:
TreeTemplate::Branch::Branch( void )
:m_Length(0.0),m_Ancestor(NULL),m_Descendant(NULL),m_RateMatrices(NULL),m_LogLikelihoods(NULL),m_CountChanges(NULL),m_HasBeenAltered(false)
{
}

TreeTemplate::Branch::Branch( DblType iLength, Node *iAncestor, Node *iDescendant, FeatureRateMatricesSet *iRateMatrices, TreeTemplate *iTreeTemplate )
:m_Length(0.0),m_Ancestor(NULL),m_Descendant(NULL),m_RateMatrices(NULL),m_LogLikelihoods(NULL),m_CountChanges(NULL),m_TreeTemplate(iTreeTemplate),m_HasBeenAltered(false)
{
    Init( iLength, iAncestor, iDescendant, iRateMatrices, iTreeTemplate );
}

TreeTemplate::Branch::~Branch( void )
{
    //The delection flows downward the tree: delete the descendant (if any):
    if( m_Descendant )
    {
        delete m_Descendant;
    }
    if( m_LogLikelihoods )
    {
        delete[] m_LogLikelihoods;
    }
    if( m_CountChanges )
    {
        delete[] m_CountChanges;
    }
}

void TreeTemplate::Branch::Init( DblType iLength, Node *iAncestor, Node *iDescendant, FeatureRateMatricesSet *iRateMatrices, TreeTemplate *iTreeTemplate )
{
    //Free any preallocated memory:
    if( m_Descendant )
    {
        delete m_Descendant;
    }
    if( m_LogLikelihoods )
    {
        delete[] m_LogLikelihoods;
    }
    if( m_CountChanges )
    {
        delete[] m_CountChanges;
    }

    //Copy the info:
    if( iLength == (-1) )
    {
        //Ok, generate a random one from the prior (following MrBayes, exponential(10.0)):
        m_Length = RandExponential( TreeTemplate::st_BranchLamba );
    }
    else
    {
        //It's really a true branch length:
        m_Length = iLength;
    }

    m_Ancestor = iAncestor;
    m_Descendant = iDescendant;
    m_RateMatrices = iRateMatrices;
    m_TreeTemplate = iTreeTemplate;

    m_LogLikelihoods = new DblType[ m_RateMatrices->GetNFeatures() + 1 ];
    if( !m_LogLikelihoods )
    {
        cerr << "Not enough memory!" << endl;
        assert( false );
        exit( 1 );
    }
    m_CountChanges = new unsigned[ m_RateMatrices->GetNFeatures() + 1 ];
    if( !m_CountChanges )
    {
        cerr << "Not enough memory!" << endl;
        assert( false );
        exit( 1 );
    }
    //All likelohoods are initially (-1) == undefined and all changes counts are 0:
    for( unsigned i = 0; i <= m_RateMatrices->GetNFeatures(); ++i )
    {
        m_LogLikelihoods[i] = -1;
        m_CountChanges[i] = 0;
    }

    m_HasBeenAltered = false;
}



/************************************************************************************
*
*                                 Likelihood computation:
*
************************************************************************************/

//Compute the likelihood for this tree template (iWhichFeature = -1 == all features):
DblType TreeTemplate::ComputeLikelihood( int iWhichFeature )
{
    //The rough way: compute the likeliood for each branch given the current feature values and branch lengths and multiply them:
    assert( m_Root );

    unsigned i;
    DblType loglikelihood = 0;

    if( iWhichFeature == (-1) )
    {
        //For each feature separately:
        for( i = 0; i < m_RateMatrices.GetNFeatures(); ++i )
        {
            loglikelihood += m_Root->ComputeLogLikelihood( i );
        }

        //Update the likelihood for all features
        m_Root->UpdateFullLogLikelihood();
    }
    else
    {
        //For this feature only:
        loglikelihood = m_Root->ComputeLogLikelihood( iWhichFeature );
    }

    /*//DEBUG:
    if( loglikelihood == 0.0 )
    {
        //For each feature separately:
        for( i = 0; i < m_RateMatrices.GetNFeatures(); ++i )
        {
            loglikelihood += m_Root->ComputeLogLikelihood( i );
        }

        //Update the likelihood for all features
        m_Root->UpdateFullLogLikelihood();
    }
    //END DEBUG*/

    //Copy the likelihoods for the entire tree here:
    for(  i = 0; i <= m_RateMatrices.GetNFeatures(); ++i )
    {
        m_LogLikelihoods[i] = m_Root->m_LogLikelihoods[i];
    }

    return loglikelihood;
}

//Compute the full loglikelihood for all features simply by adding the already computed loglikelihoods for each feature:
void TreeTemplate::Node::UpdateFullLogLikelihood( void )
{
    DblType loglikelihood = 0.0;
    unsigned i;
    for( i = 0; i < m_RateMatrices->GetNFeatures(); ++i )
    {
        loglikelihood += m_LogLikelihoods[i];
    }
    m_LogLikelihoods[m_RateMatrices->GetNFeatures()] = loglikelihood;

    //Recursively in the tree:
    for( unsigned i = 0; i < m_NumberOfDescendants; ++i )
    {
        m_Descendants[i]->UpdateFullLogLikelihood();
    }
}

void TreeTemplate::Branch::UpdateFullLogLikelihood( void )
{
    DblType loglikelihood = 0.0;
    unsigned i;
    for( i = 0; i < m_RateMatrices->GetNFeatures(); ++i )
    {
        loglikelihood += m_LogLikelihoods[i];
    }
    m_LogLikelihoods[m_RateMatrices->GetNFeatures()] = loglikelihood;

    //Recursively in the tree:
    m_Descendant->UpdateFullLogLikelihood();
}

//Compute the loglikelihood for this tree template:
DblType TreeTemplate::Node::ComputeLogLikelihood( int iWhichFeature )
{
    //The likelihood:
    DblType loglikelihood = 0.0;

    assert( iWhichFeature >= 0 );

    //Compute the loglikelihood recursively for the descendants:
    for( unsigned i = 0; i < m_NumberOfDescendants; ++i )
    {
        loglikelihood += m_Descendants[i]->ComputeLogLikelihood( iWhichFeature );
    }
    m_LogLikelihoods[ iWhichFeature ] = loglikelihood;

    return loglikelihood;
}

//Compute the loglikelihood for this tree template:
DblType TreeTemplate::Branch::ComputeLogLikelihood( int iWhichFeature )
{
    DblType loglikelihood = 0.0;

    //Optimization: see if we really need to update the likelihood of this branch:
    if( m_TreeTemplate->m_RateMatrices.GetRateMatrix4Feature(iWhichFeature)->HasBeenAltered() ||
        this->HasBeenAltered() || m_Ancestor->HasBeenAltered() || m_Descendant->HasBeenAltered() )
    {
        //Get the ancestor and descendant feature values:
        int ancestorValue = m_Ancestor->GetFeatureValue( iWhichFeature );
        int descendantValue = m_Descendant->GetFeatureValue( iWhichFeature );

        //Test for missing values on this tree:
        assert( ancestorValue != FeatureValuesMatrix::c_MissingData && descendantValue != FeatureValuesMatrix::c_MissingData );

        //Compute the likelihood of this branch:
        loglikelihood = m_RateMatrices->GetRateMatrix4Feature( iWhichFeature )->ComputeLogLikelihood( ancestorValue, descendantValue, m_Length );
        m_LogLikelihoods[ iWhichFeature ] = loglikelihood;
    }

    loglikelihood += m_Descendant->ComputeLogLikelihood( iWhichFeature );

    return loglikelihood;
}

//Generate values for the features with missing values (iWhichFeature = -1 == all features):
void TreeTemplate::GenerateMissingValues( int iWhichFeature )
{
    assert( m_Root );

    m_Root->GenerateMissingValues( iWhichFeature );

    //Set all the optimization flags to true:
    unsigned i, j;
    for( i = 0; i < m_RateMatrices.GetNFeatures(); ++i )
    {
        for( j = 0; j < m_AncestralStates[i].m_NumberOfStates; ++j )
        {
            m_AncestralStates[i].m_States[j]->SetAlterationFlag( true );
        }
        for( j = 0; j < m_MissingValueStates[i].m_NumberOfStates; ++j )
        {
            m_MissingValueStates[i].m_States[j]->SetAlterationFlag( true );
        }
        m_RateMatrices.GetRateMatrix4Feature(i)->SetAlterationFlag( true );
    }
    for( j = 0; j < m_NumberOfBranches; ++j )
    {
        m_Branches[j]->SetAlterationFlag( true );
    }
}

//Generate values for the features with missing values (iWhichFeature = -1 == all features):
void TreeTemplate::Node::GenerateMissingValues( int iWhichFeature )
{
    unsigned i;
    if( iWhichFeature == (-1) )
    {
        //All features:
        for( i = 0; i < m_Language.GetNFeatures(); ++i )
        {
            m_Language.GenerateMissingValues( m_RateMatrices->GetRateMatrix4Feature( i ), i );
        }
    }
    else
    {
        //For a single feature:
        m_Language.GenerateMissingValues( m_RateMatrices->GetRateMatrix4Feature( iWhichFeature ), iWhichFeature );
    }

    for( i = 0; i < m_NumberOfDescendants; ++i )
    {
        m_Descendants[i]->GenerateMissingValues( iWhichFeature );
    }
}

//Generate values for the features with missing values (iWhichFeature = -1 == all features):
void TreeTemplate::Branch::GenerateMissingValues( int iWhichFeature )
{
    m_Descendant->GenerateMissingValues( iWhichFeature );
}


/*****************************************************************************
*
*                       Generate new candidate trees
*
*****************************************************************************/

//Generate a new candidate tree (for the given feature(s)) from the current one,
//given the probabilities of altering the ancestral states, missing values, branch lengths and the rate matrix;
//return the Hasting Ratio for this move or (-1) if the move was unseccessful:
DblType TreeTemplate::NewCandidate( int iWhichFeature, DblType iAlterAncestralProb, DblType iAlterMissingProb, DblType iAlterBranchProb, DblType iAlterRatesProb )
{
    /*//DEBUG
    cout << "TreeTemplate::NewCandidate, iWhichFeature=" << iWhichFeature << endl;
    if( ComputeLikelihood( iWhichFeature ) == 0.0 )
    {
        cout << "OOPS!" << endl;
    }*/


    DblType hastingsRatio = 1.0;
    DblType curVal = -1.0;
    bool atLeastOne = false;

    if( iAlterAncestralProb > 0.0 && RandomProbability() <= iAlterAncestralProb )
    {
        curVal = AlterAncestralStates( iWhichFeature, NUMBER_OF_ALTERED_ANCESTRAL_STATES );
        if( curVal >= 0 )
        {
            hastingsRatio *= curVal;
            atLeastOne = true;
        }
    }
    /*//DEBUG
    if( ComputeLikelihood( iWhichFeature ) == 0.0 )
    {
        cout << "OOPS!" << endl;
        ComputeLikelihood( iWhichFeature );
    }*/

    if( iAlterMissingProb > 0.0 && RandomProbability() <= iAlterMissingProb )
    {
        curVal = AlterMissingValueStates( iWhichFeature, NUMBER_OF_ALTERED_MISSING_VALUE_STATES );
        if( curVal >= 0 )
        {
            hastingsRatio *= curVal;
            atLeastOne = true;
        }
    }
    /*//DEBUG
    if( ComputeLikelihood( iWhichFeature ) == 0.0 )
    {
        cout << "OOPS!" << endl;
        ComputeLikelihood( iWhichFeature );
    }*/

    if( iAlterBranchProb > 0.0 && RandomProbability() <= iAlterBranchProb )
    {
        curVal = AlterBeanchLengths( NUMBER_OF_ALTERED_BRANCHES );
        if( curVal >= 0 )
        {
            hastingsRatio *= curVal;
            atLeastOne = true;
        }
    }
    /*//DEBUG
    if( ComputeLikelihood( iWhichFeature ) == 0.0 )
    {
        cout << "OOPS!" << endl;
        ComputeLikelihood( iWhichFeature );
    }*/

    if( iAlterRatesProb > 0.0 && RandomProbability() <= iAlterRatesProb )
    {
        curVal = AlterRatesMatrix( iWhichFeature );
        if( curVal >= 0 )
        {
            hastingsRatio *= curVal;
            atLeastOne = true;
        }
    }
    /*//DEBUG
    if( ComputeLikelihood( iWhichFeature ) == 0.0 )
    {
        cout << "OOPS!" << endl;
        ComputeLikelihood( iWhichFeature );
    }*/

    return atLeastOne ? hastingsRatio : (-1.0);
}

//Candidate tree generation by components (return the Hastings ratio or -1):
DblType TreeTemplate::AlterAncestralStates( int iWhichFeature, unsigned iHowManyStates2Alter )
{
    unsigned i;

    if( iWhichFeature == (-1) )
    {
        iWhichFeature = RandomInt( 0, m_RateMatrices.GetNFeatures()-1 );
    }

    iHowManyStates2Alter = min( iHowManyStates2Alter, m_AncestralStates[iWhichFeature].m_NumberOfStates );
    if( iHowManyStates2Alter == 0 )
    {
        //Nothing to generate:
        return (-1.0);
    }

    //Reset the optimization flags:
    for( i = 0; i < m_AncestralStates[iWhichFeature].m_NumberOfStates; ++i )
    {
        m_AncestralStates[iWhichFeature].m_States[i]->ResetAlterationFlag();
    }

    //Randomly permute the ancestral states and pick the first iHowManyStates2Alter:
    m_AncestralStates[iWhichFeature].m_Permutation.GenerateRandomPermutation();

    DblType hastingsRatio = 1.0;
    DblType curVal;
    bool atLeastOne = false;
    for( i = 0; i < iHowManyStates2Alter; ++i )
    {
        curVal = m_AncestralStates[iWhichFeature].m_States[m_AncestralStates[iWhichFeature].m_Permutation[i]]->NewCandidate( iWhichFeature );
        if( curVal >= 0 )
        {
            //This state has been altered:
            hastingsRatio *= curVal;
            atLeastOne = true;
        }
    }

    return atLeastOne ? hastingsRatio : (-1.0);
}

DblType TreeTemplate::AlterMissingValueStates( int iWhichFeature, unsigned iHowManyStates2Alter )
{
    unsigned i = 0;

    if( iWhichFeature == (-1) )
    {
        iWhichFeature = RandomInt( 0, m_RateMatrices.GetNFeatures()-1 );
    }

    iHowManyStates2Alter = min( iHowManyStates2Alter, m_MissingValueStates[iWhichFeature].m_NumberOfStates );
    if( iHowManyStates2Alter == 0 )
    {
        //Nothing to generate:
        return (-1.0);
    }

    //Reset the optimization flags:
    for( i = 0; i < m_MissingValueStates[iWhichFeature].m_NumberOfStates; ++i )
    {
        m_MissingValueStates[iWhichFeature].m_States[i]->ResetAlterationFlag();
    }

    //Randomly permute the missing value states and pick the first iHowManyStates2Alter:
    m_MissingValueStates[iWhichFeature].m_Permutation.GenerateRandomPermutation();

    DblType hastingsRatio = 1.0;
    DblType curVal;
    bool atLeastOne = false;
    for( i = 0; i < iHowManyStates2Alter; ++i )
    {
        curVal = m_MissingValueStates[iWhichFeature].m_States[m_MissingValueStates[iWhichFeature].m_Permutation[i]]->NewCandidate( iWhichFeature );
        if( curVal >= 0 )
        {
            //This state has been altered:
            hastingsRatio *= curVal;
            atLeastOne = true;
        }
    }

    return atLeastOne ? hastingsRatio : (-1.0);
}

DblType TreeTemplate::AlterBeanchLengths( unsigned iHowManyBranches2Alter )
{
    unsigned i;

    //Reset the optimization flags:
    for( i = 0; i < m_NumberOfBranches; ++i )
    {
        m_Branches[i]->ResetAlterationFlag();
    }

    iHowManyBranches2Alter = min( iHowManyBranches2Alter, m_NumberOfBranches );

    //Randomly permute the missing value states and pick the first iHowManyStates2Alter:
    m_BranchesPermutation.GenerateRandomPermutation();

    DblType hastingsRatio = 1.0;
    DblType curVal;
    bool atLeastOne = false;
    for( unsigned i = 0; i < iHowManyBranches2Alter; ++i )
    {
        curVal = m_Branches[ m_BranchesPermutation[i] ]->NewCandidate();
        if( curVal >= 0 )
        {
            //This branch has been altered:
            hastingsRatio *= curVal;
            atLeastOne = true;
       }
    }

    return atLeastOne ? hastingsRatio : (-1.0);
}

DblType TreeTemplate::AlterRatesMatrix( int iWhichFeature )
{
    if( iWhichFeature == (-1) )
    {
        iWhichFeature = RandomInt( 0, m_RateMatrices.GetNFeatures()-1 );
    }

    //Reset the optimization flags:
    for( unsigned i = 0; i < m_RateMatrices.GetNFeatures(); ++i )
    {
        m_RateMatrices.GetRateMatrix4Feature(i)->ResetAlterationFlag();
    }

    DblType retVal = m_RateMatrices.NewCandidate( iWhichFeature );

    return retVal;
}

//Build the cached lists of paramteres (iActuallyBuildLists== true) or just count them (iActuallyBuildLists == false):
void TreeTemplate::Node::BuildCachedLists( st_AncestralOrMissingValueStates *ioAncestralStates, st_AncestralOrMissingValueStates *ioMisingValuesStates,
                                           unsigned &iNumberOfBranches, Branch **iBranches, bool iActuallyBuildLists )
{
    unsigned i;
    for( i = 0; i < m_Language.GetNFeatures(); ++i )
    {
        switch( m_Language.GetFeatureType(i) )
        {
        case LanguageWithFeatures::VALUE_ANCESTRAL:
            ++ioAncestralStates[i].m_NumberOfStates;
            if( iActuallyBuildLists )
            {
                ioAncestralStates[i].m_States[ ioAncestralStates[i].m_NumberOfStates-1 ] = this;
            }
            break;
        case LanguageWithFeatures::VALUE_MISSING:
            ++ioMisingValuesStates[i].m_NumberOfStates;
            if( iActuallyBuildLists )
            {
                ioMisingValuesStates[i].m_States[ ioMisingValuesStates[i].m_NumberOfStates-1 ] = this;
            }
            break;
        default:
            //Nothing to do:
            break;
        }
    }

    for( i = 0; i < m_NumberOfDescendants; ++i )
    {
        if( iActuallyBuildLists )
        {
            iBranches[iNumberOfBranches] = m_Descendants[i];
        }
        iNumberOfBranches++;

        //Recursively continue building the lists:
        m_Descendants[i]->GetDescendant()->BuildCachedLists( ioAncestralStates, ioMisingValuesStates, iNumberOfBranches, iBranches, iActuallyBuildLists );
    }
}

//Generate a new candidate value for a given real feature:
DblType TreeTemplate::Node::NewCandidate( int iWhichFeature )
{
    DblType hastingRatio = m_Language.NewCandidate( iWhichFeature );
    if( hastingRatio >= 0.0 )
    {
        //New candidate generated:
        SetAlterationFlag( true );
    }
    return hastingRatio;
}

//Generate a new candidate value:
DblType TreeTemplate::Branch::NewCandidate( void )
{
    /***************************************************************************
    * Use the rate multiplier like MrBayes does:
    * The length is updated by multiplication with a random multiplier
    * m = exp( lambda * (u - 0.5))
    * with lambda = 2ln(a), a being the "window size" (bigger a is bolder) and
    * m is uniform random in (0,1)
    ***************************************************************************/
    DblType u = RandomProbability();
    DblType m = exp( st_MultiplierLamba * (u - 0.5) );
    m_Length *= m;

    //New candidate generated:
    SetAlterationFlag( true );

    //The Hastings ratio is "u" in this case:
    return u;
}


//Copy operators:
TreeTemplate &TreeTemplate::operator=( const TreeTemplate &iTree )
{
    unsigned i;

    //Free the memory:
    FreeMemory();

    //Copy the name:
    m_Name = new char[ strlen(iTree.m_Name)+1 ];
    if( !m_Name )
    {
        cerr << "Not enough memory!" << endl;
        assert( false );
        exit( 1 );
    }
    strcpy( m_Name, iTree.m_Name );

    //Copy the rate matrices:
    m_RateMatrices = iTree.m_RateMatrices;

    //And the minimum changes count:
    m_CountChanges = new unsigned[ m_RateMatrices.GetNFeatures()+1 ];
    if( !m_CountChanges )
    {
        cerr << "Not enough memory!" << endl;
        assert( false );
        exit( 1 );
    }

    //And the likelihoods:
    m_LogLikelihoods = new DblType[ m_RateMatrices.GetNFeatures()+1 ];
    if( !m_LogLikelihoods )
    {
        cerr << "Not enough memory!" << endl;
        assert( false );
        exit( 1 );
    }
    for( i = 0; i <= m_RateMatrices.GetNFeatures(); ++i )
    {
        m_CountChanges[i] = iTree.m_CountChanges[i];
        m_LogLikelihoods[i] = iTree.m_LogLikelihoods[i];
    }

    //Copy the tree per se:
    if( !iTree.m_Root )
    {
        m_Root = NULL;
    }
    else
    {
        m_Root = new Node( *iTree.m_Root, this );

        //Synchronize the ancestor info:
        m_Root->SetAncestor( NULL );
    }

    //Build the cached lists:
    BuildCachedLists();

    return *this;
}

TreeTemplate &TreeTemplate::CopyContent( const TreeTemplate &iTree )
{
    //Assume the topology and tree info are the same and copy just the branch lengths and states:
    if( !m_Root )
    {
        //Nothing to copy:
    }
    else
    {
        //Do it recursively:
        m_Root->CopyContent( *iTree.m_Root );
    }

    //Copy the loglikelihoods and count changes:
    for( unsigned i = 0; i <= m_RateMatrices.GetNFeatures(); ++i )
    {
        m_LogLikelihoods[i] = iTree.m_LogLikelihoods[i];
        m_CountChanges[i] = iTree.m_CountChanges[i];
    }

    //Copy the rate matrices:
    m_RateMatrices.CopyContent( iTree.m_RateMatrices );

    return *this;
}

//Copy the node content:
void TreeTemplate::Node::CopyContent( Node &iNode )
{
    //Copy the feature states:
    m_Language.CopyContent( iNode.m_Language );

    //Copy the descendants:
    unsigned i;
    for( i = 0; i < m_NumberOfDescendants; ++i )
    {
        m_Descendants[i]->CopyContent( *iNode.m_Descendants[i] );
    }

    //Copy the likelihoods:
    for( i = 0; i <= m_RateMatrices->GetNFeatures(); ++i )
    {
        m_LogLikelihoods[i] = iNode.m_LogLikelihoods[i];
    }
}

//Copy the vloatile info (branch length and likelihoods):
void TreeTemplate::Branch::CopyContent( Branch &iBranch )
{
    m_Length = iBranch.m_Length;

    m_Descendant->CopyContent( *iBranch.m_Descendant );

    //Copy the likelihoods:
    for( unsigned i = 0; i <= m_RateMatrices->GetNFeatures(); ++i )
    {
        m_LogLikelihoods[i] = iBranch.m_LogLikelihoods[i];
        m_CountChanges[i] = iBranch.m_CountChanges[i];
    }
}




/****************************************************************************
*                           The prior probabilities
****************************************************************************/

//Get the log of the prior probability of this tree (for ratio computation, i.e. without any multiplicative constants):
DblType TreeTemplate::GetLogPriorProbability( int iWhichFeature )
{
    //Compute it recursively considering them independent:
    assert( m_Root );

    return m_Root->GetLogPriorProbability( iWhichFeature );
}

//Get the log of the prior probability of this node (for ratio computation, i.e. without any multiplicative constants):
DblType TreeTemplate::Node::GetLogPriorProbability( int iWhichFeature )
{
    DblType logprior = m_Language.GetLogPriorProbability( iWhichFeature );
    for( unsigned i = 0; i < m_NumberOfDescendants; ++i )
    {
        logprior += m_Descendants[i]->GetLogPriorProbability( iWhichFeature );
    }
    return logprior;
}

//Get the log of the prior probability of this branch (for ratio computation, i.e. without any multiplicative constants):
DblType TreeTemplate::Branch::GetLogPriorProbability( int iWhichFeature )
{
    //The branch length prior is exponential with parameter st_BranchLamba:
    /*return (st_BranchLamba * exp( -st_BranchLamba * m_Length )) * m_Descendant->GetPriorProbability( iWhichFeature );*/

    return (-st_BranchLamba * m_Length) + m_Descendant->GetLogPriorProbability( iWhichFeature );
}



/****************************************************************************
*               Writing data to file for later analysis:
****************************************************************************/

void TreeTemplate::WriteParamsHeader( FILE *iFile )
{
    //Watch the flexible feature values at all nodes and all the branch lengths:
    assert( m_Root );
    m_Root->WriteParamsHeader( iFile );

    //...and the rate matrices as well:
    m_RateMatrices.WriteParamsHeader( iFile );
}

void TreeTemplate::Node::WriteParamsHeader( FILE *iFile )
{
    unsigned i, j;

    m_Language.WriteParamsHeader( iFile );

    for( j = 0; j < m_TreeTemplate->m_RateMatrices.GetNFeatures(); ++j )
    {
        fprintf( iFile, "NC.%s.%s\t", m_Language.GetName(), m_TreeTemplate->m_RateMatrices.GetFeature(j)->GetName() );
    }
    fprintf( iFile, "NC.%s.%s\t", m_Language.GetName(), "All" );

    //Also print the branch lenghts:
    for( i = 0; i < m_NumberOfDescendants; ++i )
    {
        fprintf( iFile, "%s..%s\t", m_Language.GetName(), m_Descendants[i]->GetDescendant()->GetLanguageName() );

        for( j = 0; j < m_TreeTemplate->m_RateMatrices.GetNFeatures(); ++j )
        {
            fprintf( iFile, "%s..%s.%s\t", m_Language.GetName(), m_Descendants[i]->GetDescendant()->GetLanguageName(), m_TreeTemplate->m_RateMatrices.GetFeature(j)->GetName() );
        }
        fprintf( iFile, "%s..%s.All\t", m_Language.GetName(), m_Descendants[i]->GetDescendant()->GetLanguageName() );

        m_Descendants[i]->GetDescendant()->WriteParamsHeader( iFile );
    }
}

unsigned TreeTemplate::WriteParamsValues( char *iBuffer )
{
    char *curChar = iBuffer;

    //Watch the flexible feature values at all nodes and all the branch lengths:
    assert( m_Root );
    curChar += m_Root->WriteParamsValues( curChar );

    //...and the rate matrices as well:
    curChar += m_RateMatrices.WriteParamsValues( curChar );

    return curChar - iBuffer;
}

void TreeTemplate::WriteParamsValues( FILE *iFile )
{
    //Watch the flexible feature values at all nodes and all the branch lengths:
    assert( m_Root );
    m_Root->WriteParamsValues( iFile );

    //...and the rate matrices as well:
    m_RateMatrices.WriteParamsValues( iFile );
}

void TreeTemplate::Node::WriteParamsValues( FILE *iFile )
{
    unsigned i, j;

    m_Language.WriteParamsValues( iFile );

    for( j = 0; j <= m_TreeTemplate->m_RateMatrices.GetNFeatures(); ++j )
    {
        fprintf( iFile, "%d\t", m_CountChanges[j] );
    }

    //Also print the branch lenghts:
    for( i = 0; i < m_NumberOfDescendants; ++i )
    {
        fprintf( iFile, "%f\t", m_Descendants[i]->GetLength() );

        for( j = 0; j < m_TreeTemplate->m_RateMatrices.GetNFeatures(); ++j )
        {
            fprintf( iFile, "%d\t", m_Descendants[i]->GetNumberOfChangesParsimony(j) );
        }
        fprintf( iFile, "%d\t", m_Descendants[i]->GetNumberOfChangesParsimony(-1) );

        m_Descendants[i]->GetDescendant()->WriteParamsValues( iFile );
    }
}

unsigned TreeTemplate::Node::WriteParamsValues( char *iBuffer )
{
    unsigned i, j;

    char *curChar = iBuffer;

    curChar += m_Language.WriteParamsValues( curChar );

    for( j = 0; j <= m_TreeTemplate->m_RateMatrices.GetNFeatures(); ++j )
    {
        curChar += sprintf( curChar, "%d\t", m_CountChanges[j] );
    }

    //Also print the branch lenghts:
    for( i = 0; i < m_NumberOfDescendants; ++i )
    {
        curChar += sprintf( curChar, "%f\t", m_Descendants[i]->GetLength() );

        for( j = 0; j < m_TreeTemplate->m_RateMatrices.GetNFeatures(); ++j )
        {
            curChar += sprintf( curChar, "%d\t", m_Descendants[i]->GetNumberOfChangesParsimony(j) );
        }
        curChar += sprintf( curChar, "%d\t", m_Descendants[i]->GetNumberOfChangesParsimony(-1) );

        curChar += m_Descendants[i]->GetDescendant()->WriteParamsValues( curChar );
    }

    return curChar - iBuffer;
}

void TreeTemplate::WriteStructure( FILE *iFile )
{
    unsigned i, j;

    //Write the structure of the tree as the adjacency matrix:
    assert( m_Root );
    assert( iFile );

    unsigned numberOfNodes = GetNumberOfNodes();

    //The list of nodes (to be used as indices):
    Node **nodesList = new Node*[numberOfNodes];
    if( !nodesList )
    {
        cerr << "Not enough memory!" << endl;
        assert( false );
        exit( 1 );
    }
    unsigned currentIndex = 0;
    m_Root->BuildNodesList( nodesList, currentIndex );
    assert( currentIndex == numberOfNodes );

    //The adjacency matrix:
    int **adjacencyMatrix = new int*[ numberOfNodes ];
    if( !adjacencyMatrix )
    {
        cerr << "Not enough memory!" << endl;
        assert( false );
        exit( 1 );
    }
    for( i = 0; i < numberOfNodes; ++i )
    {
        adjacencyMatrix[i] = new int[ numberOfNodes ];
        if( !adjacencyMatrix[i] )
        {
            cerr << "Not enough memory!" << endl;
            assert( false );
            exit( 1 );
        }
        memset( adjacencyMatrix[i], 0, numberOfNodes*sizeof(int) );
    }

    //Mark the edges recursively:
    m_Root->MarkEdges( nodesList, numberOfNodes, adjacencyMatrix );

    //Export the adjacency matrix to file:
    //The header:
    fprintf( iFile, "Nodes\t" );
    for( i = 0; i < numberOfNodes; ++i )
    {
        fprintf( iFile, "%s%s", nodesList[i]->GetLanguageName(), (i < numberOfNodes-1) ? "\t" : "" );
    }
    fprintf( iFile, "\n" );

    //The rows, one by one:
    for( i = 0; i < numberOfNodes; ++i )
    {
        fprintf( iFile, "%s\t", nodesList[i]->GetLanguageName() );

        for( j = 0; j < numberOfNodes; ++j )
        {
            const char *adjacencyType = NULL;
            if( adjacencyMatrix[i][j] == 0 )
            {
                adjacencyType = "NONE";
            }
            else if( adjacencyMatrix[i][j] == 2 )
            {
                adjacencyType = "SELF";
            }
            else if( adjacencyMatrix[i][j] == 1 )
            {
                adjacencyType = "DESCENDANT";
            }
            else if( adjacencyMatrix[i][j] == -1 )
            {
                adjacencyType = "ANCESTOR";
            }
            else
            {
                cerr << "Unknown adjacency id!" << endl;
                assert( false );
            }

            fprintf( iFile, "%s%s", adjacencyType, (j < numberOfNodes-1) ? "\t" : "" );
        }
        fprintf( iFile, "\n" );
    }

    //Free up the memory:
    delete[] nodesList;
    for( i = 0; i < numberOfNodes; ++i )
    {
        delete[] adjacencyMatrix[i];
    }
    delete[] adjacencyMatrix;
}

unsigned TreeTemplate::Node::GetNumberOfNodes( void )
{
    unsigned numberOfNodes = 1; //Including self
    for( unsigned i = 0; i < m_NumberOfDescendants; ++i )
    {
        numberOfNodes += m_Descendants[i]->GetDescendant()->GetNumberOfNodes();
    }
    return numberOfNodes;
}

void TreeTemplate::Node::BuildNodesList( Node **ioNodesList, unsigned &ioCurrentNode )
{
    ioNodesList[ioCurrentNode] = this;
    m_NodeIndex = ioCurrentNode;
    ++ioCurrentNode;

    for( unsigned i = 0; i < m_NumberOfDescendants; ++i )
    {
        m_Descendants[i]->GetDescendant()->BuildNodesList( ioNodesList, ioCurrentNode );
    }
}

void TreeTemplate::Node::MarkEdges( Node **ioNodesList, unsigned iNumberOfNodes, int **ioAdjacencymatrix )
{
    ioAdjacencymatrix[m_NodeIndex][m_NodeIndex] = 2; //Self link
    for( unsigned i = 0; i < m_NumberOfDescendants; ++i )
    {
        ioAdjacencymatrix[m_NodeIndex][m_Descendants[i]->GetDescendant()->m_NodeIndex] = 1; //Descendant link
        ioAdjacencymatrix[m_Descendants[i]->GetDescendant()->m_NodeIndex][m_NodeIndex] = -1; //Ancestor link

        //Recursively do it:
        m_Descendants[i]->GetDescendant()->MarkEdges( ioNodesList, iNumberOfNodes, ioAdjacencymatrix );
    }
}



//Given a tree, count the minimim number of changes taking place on this tree (parsimony-style):
void TreeTemplate::CountChangesParsimony( bool iIgnoreMissingDataNodes )
{
    assert( m_Root );

    //Reset the counters:
    memset( m_CountChanges, 0, (m_RateMatrices.GetNFeatures()+1) * sizeof(*m_CountChanges) );
    memset( m_Root->m_CountChanges, 0, (m_RateMatrices.GetNFeatures()+1) * sizeof(*m_CountChanges) );

    //Do the count recursively:
    m_Root->CountChangesParsimony( m_CountChanges, iIgnoreMissingDataNodes );
}

//Count the minimim number of changes taking place on this tree (parsimony-style):
void TreeTemplate::Node::CountChangesParsimony( unsigned *iCounter, bool iIgnoreMissingDataNodes )
{
    unsigned i;

    //For each branch, count the changes for all features:
    for( i = 0; i < m_NumberOfDescendants; ++i )
    {
        m_Descendants[i]->CountChangesParsimony( iCounter, iIgnoreMissingDataNodes );
    }
}

//Count the minimim number of changes taking place on this tree (parsimony-style):
void TreeTemplate::Branch::CountChangesParsimony( unsigned *iCounter, bool iIgnoreMissingDataNodes )
{
    unsigned changes = 0, i, result;

    //For each feature:
    for( i = 0; i < m_TreeTemplate->m_RateMatrices.GetNFeatures(); ++i )
    {
        //The changes on this branch alone:
        if( iIgnoreMissingDataNodes && m_Descendant->IsMissing(i) )
        {
            result = 0; //Ignore missing data nodes!
        }
        else
        {
            result = m_TreeTemplate->m_RateMatrices.GetRateMatrix4Feature(i)->CountChangesParsimony( m_Ancestor->GetFeatureValue(i), m_Descendant->GetFeatureValue(i) );
        }
        m_CountChanges[i] = result;
        iCounter[i] += result;
        changes += result;
        m_Descendant->m_CountChanges[i] = m_Ancestor->m_CountChanges[i] + result;
    }
    //And the total number of changes:
    m_CountChanges[i] = changes;
    iCounter[i] += changes;
    m_Descendant->m_CountChanges[i] = m_Ancestor->m_CountChanges[i] + changes;

    //Continue the recursion:
    m_Descendant->CountChangesParsimony( iCounter, iIgnoreMissingDataNodes );
}













