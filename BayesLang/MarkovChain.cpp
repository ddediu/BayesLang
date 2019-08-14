#include "MarkovChain.h"

//The cache of the results file:
MarkovChain::st_TreesFileCache_ MarkovChain::st_TressFileCache;
//The trees file name:
const char *MarkovChain::st_TreesFileName = "MCMCResults.csv";
//The trees file:
FILE *MarkovChain::st_TreesFile = NULL;
//The trees structure file name:
const char *MarkovChain::st_TreeStructureFileName = "MCMCModelStructure.csv";

pthread_mutex_t MarkovChain::st_TreesFileCacheMutex = PTHREAD_MUTEX_INITIALIZER;

MarkovChain::MarkovChain()
:m_Temperature(1.0),m_Generation(0),m_SaveEveryNGenerations(100),m_CurrentLogLikelihood(1),
 m_CurrentLogPrior(1),m_CurrentLogLikelihoodLogPriorInitialized(false)
{
}

MarkovChain::MarkovChain( TreeTemplate &iTreeTemplate, DblType iTemperature, bool iRandomStartingTree )
:m_Temperature(iTemperature),m_Generation(0),m_SaveEveryNGenerations(100),m_CurrentLogLikelihood(1),
 m_CurrentLogPrior(1),m_CurrentLogLikelihoodLogPriorInitialized(false)
{
    Init( iTreeTemplate, iTemperature, iRandomStartingTree );
}

void MarkovChain::Init( TreeTemplate &iTreeTemplate, DblType iTemperature, bool iRandomStartingTree )
{
    m_CurrentTree = iTreeTemplate;
    m_CurrentTree.GenerateMissingValues( -1 ); //Make sure the trees does not have missing values
    m_CurrentTree.ComputeLikelihood( -1 ); //And compute its likelihood

    m_CandidateTree = m_CurrentTree;
    if( iRandomStartingTree )
    {
        //Generate a random staring point:
        m_CandidateTree.GenerateMissingValues(-1);
        m_CandidateTree.ComputeLikelihood( -1 );
    }

    m_Temperature = iTemperature;

    m_Generation = 0;
    m_SaveEveryNGenerations = 100;

    if( m_Temperature == 1.0 )
    {
        //Always save the first tree:
        SaveInfo( -1, true );

        //The result files:
        st_TreesFile = fopen( st_TreesFileName, "wt" );
        if( !st_TreesFile )
        {
            cerr << "Error creating the results file \"" << st_TreesFileName << "\"!" << endl;
            assert( false );
            exit( 1 );
        }
        fprintf( st_TreesFile, "Count\tGeneration\t" );
        if( gl_PrintTrees )
        {
            fprintf( st_TreesFile, "Tree\t" );
        }
        fprintf( st_TreesFile, "LogLikelihood\t" );
        for( unsigned i = 0; i < m_CurrentTree.GetFeatureRateMatricesSet()->GetNFeatures(); ++i )
        {
            fprintf( st_TreesFile, "NChanges.%s\t", m_CurrentTree.GetFeatureRateMatricesSet()->GetFeature(i)->GetName() );
        }
        fprintf( st_TreesFile, "NChanges.All\t" );
        m_CurrentTree.WriteParamsHeader( st_TreesFile );
        fprintf( st_TreesFile, "\n" );
        fclose( st_TreesFile );

        //Also export the structure of the model:
        FILE *file = fopen( st_TreeStructureFileName, "wt" );
        if( !file )
        {
            cerr << "Error creating the model structure file \"" << st_TreeStructureFileName << "\"!" << endl;
            assert( false );
            exit( 1 );
        }
        m_CurrentTree.WriteStructure( file );
        fclose( file );
    }

    st_TreesFile = NULL;
    st_TressFileCache.m_TreesFile = NULL;

    m_CurrentLogLikelihoodLogPriorInitialized = false;
}

MarkovChain::~MarkovChain()
{
}

void MarkovChain::Print( FILE *iFile, int iWhichFeature )
{
    fprintf( iFile, "Markov Chain:\n" );
    fprintf( iFile, "Current tree:\n" );
    m_CurrentTree.Print( iFile, false, iWhichFeature, true, true );
    fprintf( iFile, "\n" );
}

//Generate a new cadidate (return true if the candidate was accepted):
bool MarkovChain::NewCandidate( int iWhichFeature, DblType iAlterAncestralProb, DblType iAlterMissingProb, DblType iAlterBranchProb, DblType iAlterRatesProb )
{
    bool acceptedMove = false;

    //Increment the current generation
    ++m_Generation;

    //Generate a new tree candidate:
    DblType hastingsRatio = m_CandidateTree.NewCandidate( iWhichFeature, iAlterAncestralProb, iAlterMissingProb, iAlterBranchProb, iAlterRatesProb );
    if( hastingsRatio == (-1) )
    {
        //Nothing was generated!
        return false;
    }

    //Compute the new likelihood:
    m_CandidateTree.ComputeLikelihood( iWhichFeature );

    /*//Debug:
    if( m_CandidateTree.GetLogLikelihood( iWhichFeature ) == 0.0 )
    {
        cout << "OOPS (likelihood == 0.0): current tree:" << endl;
        m_CurrentTree.Print(stdout,false,-1);
        cout << "candidate tree:" << endl;
        m_CandidateTree.Print(stdout,false,-1);
        cout << endl;
    }*/

    //Compute the acceptance probability r = min(1, prior_ratio * likelihood_ratio * hastings_ratio ):
    DblType candidateLogPrior = m_CandidateTree.GetLogPriorProbability( iWhichFeature);
    DblType candidateLogLikelihood = m_CandidateTree.GetLogLikelihood( iWhichFeature );

    if( !m_CurrentLogLikelihoodLogPriorInitialized )
    {
        //Not yet inialized:
        m_CurrentLogPrior = m_CurrentTree.GetLogPriorProbability( iWhichFeature);
        m_CurrentLogLikelihood = m_CurrentTree.GetLogLikelihood( iWhichFeature );
        m_CurrentLogLikelihoodLogPriorInitialized = true;

        //Also compute the minimum number of changes on this tree:
        m_CurrentTree.CountChangesParsimony();
    }

    DblType priorsLogRatio = candidateLogPrior - m_CurrentLogPrior;
    DblType loglikelihoodsRatio = candidateLogLikelihood - m_CurrentLogLikelihood;
    DblType r = min( 1.0, exp( m_Temperature * (priorsLogRatio + loglikelihoodsRatio) ) * hastingsRatio );

    if( RandomProbability() < r )
    {
        //Move accepted:
        m_CurrentTree.CopyContent( m_CandidateTree );
        m_CurrentLogPrior = candidateLogPrior;
        m_CurrentLogLikelihood = candidateLogLikelihood;

        //Also compute the minimum number of changes on this tree:
        m_CurrentTree.CountChangesParsimony();

        acceptedMove = true;
    }
    else
    {
        //The move was rejected
        m_CandidateTree.CopyContent( m_CurrentTree );
        acceptedMove = false;
    }

    //Save the relevant info:
    SaveInfo( iWhichFeature, acceptedMove );

    return acceptedMove;
}

//Run the MCMC for N generations and return the number of accepted moves:
unsigned long MarkovChain::MCMC( unsigned long iGenerations, int iWhichFeature, DblType iAlterAncestralProb, DblType iAlterMissingProb, DblType iAlterBranchProb, DblType iAlterRatesProb, bool iPrintProgress )
{
    unsigned long onePercent = iGenerations / 10; //Actually, 10%
    unsigned currentPercent = 1;
    unsigned long doneSoFar = 0;
    unsigned long successfulMoves = 0;

    if( iPrintProgress )
    {
        cout << "Current MCMC progress (0 to 9): ";
        PrintProgress( 0, iGenerations, 0 );
    }

    for( unsigned long i = 0; i < iGenerations; ++i )
    {
        if( NewCandidate( iWhichFeature, iAlterAncestralProb, iAlterMissingProb, iAlterBranchProb, iAlterRatesProb ) )
        {
            ++successfulMoves;
        }

        if( iPrintProgress )
        {
            if( doneSoFar == onePercent )
            {
                PrintProgress( i, iGenerations, currentPercent );
                doneSoFar = 0;
                ++currentPercent;
            }
            else
            {
                ++doneSoFar;
            }
        }
    }
    if( iPrintProgress )
    {
        cout << endl;
    }

    return successfulMoves;
}

//Save the relevant info in the case of a successful move:
void MarkovChain::SaveInfo( int iWhichFeature, bool iNewStuff )
{
    if( m_Temperature != 1.0 )
    {
        //Only the cold chain is saved!
        return;
    }

    if( m_Generation % m_SaveEveryNGenerations != 0 )
    {
        //Not yet!
        return;
    }

    if( iNewStuff == false )
    {
        //Old stuff: simply duplicate the last added info:
        st_TressFileCache.AddDuplicatedLine();
    }
    else
    {
        //New line here:

        //Print the info to the buffer:
        #define LINE_BUFFER_MAX_SIZE 1024001
        static char lineBuffer[ LINE_BUFFER_MAX_SIZE ];
        #undef LINE_BUFFER_MAX_SIZE
        char *curChar = lineBuffer;
        curChar += sprintf( curChar, "%lu\t", m_Generation );
        if( gl_PrintTrees )
        {
            curChar += m_CurrentTree.Print( curChar, true, iWhichFeature, false, false );
            curChar += sprintf( curChar, "\t" );
        }
        curChar += sprintf( curChar, "%f\t", m_CurrentTree.GetLogLikelihood( iWhichFeature ) );
        if( iWhichFeature == (-1) )
        {
            for( unsigned i = 0; i < m_CurrentTree.GetFeatureRateMatricesSet()->GetNFeatures(); ++i )
            {
                curChar += sprintf( curChar, "%d\t", m_CurrentTree.GetNumberOfChangesParsimony( i ) );
            }
            curChar += sprintf( curChar, "%d\t", m_CurrentTree.GetNumberOfChangesParsimony( -1 ) );
        }
        else
        {
            curChar += sprintf( curChar, "%d\t", m_CurrentTree.GetNumberOfChangesParsimony( iWhichFeature ) );
        }
        curChar += m_CurrentTree.WriteParamsValues( curChar );
        sprintf( curChar, "\n" );

        assert( m_CurrentTree.GetLogLikelihood( iWhichFeature ) );

        st_TressFileCache.AddNewLine( lineBuffer );
    }

    //TEST
    //fputs( lineBuffer, m_TreesFile );

    //Add the current line to the cache:
}

//Print the MCMC progress:
void MarkovChain::PrintProgress( unsigned long iCurrentGeneration, unsigned long iTotalGenerations, unsigned iCurrentPercent )
{
    //cout << iCurrentPercent << "%";
    //fprintf( stdout, "." );
    fprintf( stdout, "%1d", iCurrentPercent );
    fflush( stdout );
}

//Swap the states of two MCMC at different temperatures:
void MarkovChain::SwapTemperatures( MarkovChain &iMC1, MarkovChain &iMC2 )
{
    //Exchange only the temperatures!
    DblType aux = iMC1.m_Temperature;
    iMC1.m_Temperature = iMC2.m_Temperature;
    iMC2.m_Temperature = aux;
}



