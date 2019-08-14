#include "MetropolisCoupledMCMC.h"

MetropolisCoupledMCMC::MetropolisCoupledMCMC()
:m_HeatingCoeff(0),m_NumberOfChains(0),m_GenerationsBetweenSwaps(0),m_Chains(NULL),m_Temperatures(NULL),
 m_AreThreadDataInitialized(false),m_ThreadsData(NULL),m_TheThreads(NULL),m_SwappingCandidateChais(NULL),
 m_SynchronizingThreadsMutex(NULL),
 m_SynchronizingThreadsCondition(NULL),m_SynchronizingThreadsFlag(NULL)
{
}

MetropolisCoupledMCMC::MetropolisCoupledMCMC( TreeTemplate &iTreeTemplate, unsigned iNumberOfChains, DblType iHeatingCoeff, unsigned iGenerationsBetweenSwaps )
:m_HeatingCoeff(0),m_NumberOfChains(0),m_GenerationsBetweenSwaps(0),m_Chains(NULL),m_Temperatures(NULL),
 m_AreThreadDataInitialized(false),m_ThreadsData(NULL),m_TheThreads(NULL),m_SwappingCandidateChais(NULL),
 m_SynchronizingThreadsMutex(NULL),
 m_SynchronizingThreadsCondition(NULL),m_SynchronizingThreadsFlag(NULL)
{
    Init( iTreeTemplate, iNumberOfChains, iHeatingCoeff, iGenerationsBetweenSwaps );
}

MetropolisCoupledMCMC::~MetropolisCoupledMCMC()
{
    FreeMemory();
}

void MetropolisCoupledMCMC::Init( TreeTemplate &iTreeTemplate, unsigned iNumberOfChains, DblType iHeatingCoeff, unsigned iGenerationsBetweenSwaps )
{
    if( (m_HeatingCoeff = iHeatingCoeff) < 0 )
    {
        cerr << "The heating coefficient must pe at least 0.0!" << endl;
        assert( false );
        exit( 1 );
    }

    if( iNumberOfChains < 1 )
    {
        cerr << "There must be at least 1 chain!" <<  endl;
        assert( false );
        exit( 1 );
    }

    AllocMemory( iNumberOfChains );

    for( unsigned i = 0; i < m_NumberOfChains; ++i )
    {
        m_Temperatures[i] = 1/(1 + m_HeatingCoeff*i);
        m_Chains[i].Init( iTreeTemplate, m_Temperatures[i], true );
    }

    m_GenerationsBetweenSwaps = iGenerationsBetweenSwaps;

    m_Permutation.Initialize( m_NumberOfChains );

    m_AreThreadDataInitialized = false;
}

//Memory management:
void MetropolisCoupledMCMC::AllocMemory( unsigned iNumberOfChains )
{
    unsigned i, j;

    //Free any pre-allocated memory:
    FreeMemory();

    assert( iNumberOfChains > 0 );
    m_Chains = new MarkovChain[ m_NumberOfChains = iNumberOfChains ];
    if( !m_Chains )
    {
        cerr << "Not enough memory!" << endl;
        assert( false );
        exit( 1 );
    }

    m_Temperatures = new DblType[ m_NumberOfChains ];
    if( !m_Temperatures )
    {
        cerr << "Not enough memory!" << endl;
        assert( false );
        exit( 1 );
    }

    m_ThreadsData = new st_ThreadData[ m_NumberOfChains ];
    if( !m_ThreadsData )
    {
        cerr << "Not enough memory!" << endl;
        assert( false );
        exit( 1 );
    }

    m_TheThreads = new pthread_t[ m_NumberOfChains ];
    if( !m_TheThreads )
    {
        cerr << "Not enough memory!" << endl;
        assert( false );
        exit( 1 );
    }

    //Alloc & initialize the mutexes, conditions and flags:
    if( m_SynchronizingThreadsMutex )
    {
        for( i = 0; i < m_NumberOfChains; ++i )
        {
            delete[] m_SynchronizingThreadsMutex[i];
        }
        delete[] m_SynchronizingThreadsMutex;
    }
    m_SynchronizingThreadsMutex = new pthread_mutex_t*[ m_NumberOfChains ];
    if( !m_SynchronizingThreadsMutex )
    {
        cerr << "Not enough memory!" << endl;
        assert( false );
        exit( 1 );
    }
    for( i = 0; i < m_NumberOfChains; ++i )
    {
        m_SynchronizingThreadsMutex[i] = new pthread_mutex_t[ m_NumberOfChains ];
        if( !m_SynchronizingThreadsMutex[i] )
        {
            cerr << "Not enough memory!" << endl;
            assert( false );
            exit( 1 );
        }
    }

    if( m_SynchronizingThreadsCondition )
    {
        for( i = 0; i < m_NumberOfChains; ++i )
        {
            delete[] m_SynchronizingThreadsCondition[i];
        }
        delete[] m_SynchronizingThreadsCondition;
    }
    m_SynchronizingThreadsCondition = new pthread_cond_t*[ m_NumberOfChains ];
    if( !m_SynchronizingThreadsCondition )
    {
        cerr << "Not enough memory!" << endl;
        assert( false );
        exit( 1 );
    }
    for( i = 0; i < m_NumberOfChains; ++i )
    {
        m_SynchronizingThreadsCondition[i] = new pthread_cond_t[ m_NumberOfChains ];
        if( !m_SynchronizingThreadsCondition[i] )
        {
            cerr << "Not enough memory!" << endl;
            assert( false );
            exit( 1 );
        }
    }

    if( m_SynchronizingThreadsFlag )
    {
        for( i = 0; i < m_NumberOfChains; ++i )
        {
            delete[] m_SynchronizingThreadsFlag[i];
        }
        delete[] m_SynchronizingThreadsFlag;
    }
    m_SynchronizingThreadsFlag = new bool*[ m_NumberOfChains ];
    if( !m_SynchronizingThreadsFlag )
    {
        cerr << "Not enough memory!" << endl;
           assert( false );
        exit( 1 );
    }
    for( i = 0; i < m_NumberOfChains; ++i )
    {
        m_SynchronizingThreadsFlag[i] = new bool[ m_NumberOfChains ];
        if( !m_SynchronizingThreadsFlag[i] )
        {
            cerr << "Not enough memory!" << endl;
            assert( false );
            exit( 1 );
        }
    }

    for( i = 0; i < m_NumberOfChains; ++i )
    {
        for( j = 0; j < m_NumberOfChains; ++j )
        {
            pthread_mutex_init( &m_SynchronizingThreadsMutex[i][j], NULL );
            pthread_cond_init( &m_SynchronizingThreadsCondition[i][j], NULL );
            m_SynchronizingThreadsFlag[i][j] = false;
        }
    }
}

void MetropolisCoupledMCMC::FreeMemory( void )
{
    unsigned i;

    if( m_NumberOfChains && m_Chains )
    {
        delete[] m_Chains;
    }

    if( m_NumberOfChains && m_Temperatures )
    {
        delete[] m_Temperatures;
    }

    if( m_NumberOfChains && m_ThreadsData )
    {
        delete[] m_ThreadsData;
    }

    if( m_NumberOfChains && m_TheThreads )
    {
        delete[] m_TheThreads;
    }

    if( m_NumberOfChains && m_SynchronizingThreadsMutex )
    {
        for( i = 0; i < m_NumberOfChains; ++i )
        {
            delete[] m_SynchronizingThreadsMutex[i];
        }
        delete[] m_SynchronizingThreadsMutex;
    }

    if( m_NumberOfChains && m_SynchronizingThreadsCondition )
    {
        for( i = 0; i < m_NumberOfChains; ++i )
        {
            delete[] m_SynchronizingThreadsCondition[i];
        }
        delete[] m_SynchronizingThreadsCondition;
    }

    if( m_NumberOfChains && m_SynchronizingThreadsFlag )
    {
        for( i = 0; i < m_NumberOfChains; ++i )
        {
            delete[] m_SynchronizingThreadsFlag[i];
        }
        delete[] m_SynchronizingThreadsFlag;
    }
}

//Run the MCMC for N generations and return the number of accepted moves:
unsigned long MetropolisCoupledMCMC::MCMCMC( unsigned long iGenerations, int iWhichFeature, DblType iAlterAncestralProb, DblType iAlterMissingProb, DblType iAlterBranchProb, DblType iAlterRatesProb, bool iPrintProgress )
{
    unsigned long numberOfCycles = (unsigned long)ceil( (DblType)iGenerations / (DblType)m_GenerationsBetweenSwaps );
    unsigned long numberOfAcceptedMoves = 0;
    m_OnePercent = numberOfCycles / 10; //Actually, 10%
    m_CurrentPercent = 1;
    m_DoneSoFar = 0;
    m_PrintProgress = iPrintProgress;

    if( m_PrintProgress )
    {
        cout << "Current MCMC progress (0 to 9): ";
        PrintProgress( 0, iGenerations, 0 );
    }

    //Open the results file:
    MarkovChain::ReOpenResultsFile();

    unsigned long i;
    unsigned j;

    if( m_NumberOfChains == 1 )
    {
        //No need for multithreading: just do it sequentially
        for( i = 0; i < numberOfCycles; ++i )
        {
            //Do the chains:
            numberOfAcceptedMoves += m_Chains[0].MCMC( m_GenerationsBetweenSwaps, iWhichFeature, iAlterAncestralProb, iAlterMissingProb, iAlterBranchProb, iAlterRatesProb, false );

            if( m_PrintProgress )
            {
                if( m_DoneSoFar == m_OnePercent )
                {
                    PrintProgress( i, iGenerations, m_CurrentPercent );
                    m_DoneSoFar = 0;
                    ++m_CurrentPercent;
                }
                else
                {
                    ++m_DoneSoFar;
                }
            }

            //Swap two chains:
            SwapChains();
        }
    }
    else if( gl_Software_SMP_Implementation == true )
    {
        //Create the threads:
        if( !m_AreThreadDataInitialized )
        {
            //Init the thread data:
            for( j = 0; j < m_NumberOfChains; ++j )
            {
                m_ThreadsData[j].m_Chain = &m_Chains[j];
                m_ThreadsData[j].m_ID = j;
                m_ThreadsData[j].m_Temperature = m_Temperatures[j];

                m_ThreadsData[j].m_NumberOfCycles = numberOfCycles;
                m_ThreadsData[j].m_GenerationsPerCycle = m_GenerationsBetweenSwaps;

                m_ThreadsData[j].m_WhichFeatures = iWhichFeature;
                m_ThreadsData[j].m_AlterAncestralProb = iAlterAncestralProb;
                m_ThreadsData[j].m_AlterMissingProb = iAlterMissingProb;
                m_ThreadsData[j].m_AlterBranchProb = iAlterBranchProb;
                m_ThreadsData[j].m_AlterRatesProb = iAlterRatesProb;

                m_ThreadsData[j].m_MCMCMC = this;
            }
        }

        //Pre-generate the sequence of swaps:
        if( m_SwappingCandidateChais )
        {
            delete[] m_SwappingCandidateChais;
        }
        m_SwappingCandidateChais = new st_SwappingCandidateChais[ numberOfCycles ];
        if( !m_SwappingCandidateChais )
        {
            cerr << "Not enough memory!" << endl;
            assert( false );
            exit( 1 );
        }
        for( i = 0; i < numberOfCycles; ++i )
        {
            //Pick two random chains to exchange states:
            m_Permutation.GenerateRandomPermutation();
            m_SwappingCandidateChais[i].m_Chain1 = m_Permutation[0];
            m_SwappingCandidateChais[i].m_Chain2 = m_Permutation[1];
        }

        //Reset the mutexes, conditions and synchronization flag:
        pthread_mutex_init( &m_SwappingMutex, NULL );
        for( j = 0; j < m_NumberOfChains; ++j )
        {
            for( unsigned k = 0; k < m_NumberOfChains; ++k )
            {
                pthread_mutex_init( &m_SynchronizingThreadsMutex[j][k], NULL );
                pthread_cond_init( &m_SynchronizingThreadsCondition[j][k], NULL );
                m_SynchronizingThreadsFlag[j][k] = false;
            }
        }

        //Start the threads:
        for( j = 0; j < m_NumberOfChains; ++j )
        {
            if( pthread_create( &m_TheThreads[j], NULL, ThreadMCMC, (void*) &m_ThreadsData[j]) )
            {
                cerr << "Error creating MCMC thread: aborting..." << endl;
                assert( false );
                exit( 1 );
            }
        }

        //Wait for the threads to finish:
        for( j = 0; j < m_NumberOfChains; ++j )
        {
            //Wait for this thread:
            pthread_join( m_TheThreads[j], NULL);

            //...and use the result:
            numberOfAcceptedMoves += m_ThreadsData[j].m_NumberOfAcceptedMoves;
        }

        //Free the memory:
         if( m_SwappingCandidateChais )
        {
            delete[] m_SwappingCandidateChais;
        }
   }
   else
   {
       //Do it sequentially:
        for( i = 0; i < numberOfCycles; ++i )
        {
            //Do the chains:
            for( j = 0; j < m_NumberOfChains; ++j )
            {
                numberOfAcceptedMoves += m_Chains[j].MCMC( m_GenerationsBetweenSwaps, iWhichFeature, iAlterAncestralProb, iAlterMissingProb, iAlterBranchProb, iAlterRatesProb, false );

                if( m_PrintProgress )
                {
                    if( m_DoneSoFar == m_OnePercent )
                    {
                        PrintProgress( i, iGenerations, m_CurrentPercent );
                        m_DoneSoFar = 0;
                        ++m_CurrentPercent;
                    }
                    else
                    {
                        ++m_DoneSoFar;
                    }
                }

                //Swap two chains:
                SwapChains();
            }
        }
   }

    if( m_PrintProgress )
    {
        cout << endl;
    }

    //Flush and close the results file:
    MarkovChain::FlushCache();
    MarkovChain::CloseResultsFile();

    return numberOfAcceptedMoves;
}

//The thread worker function:
void *MetropolisCoupledMCMC::ThreadMCMC( void *iThreadData )
{
    bool swappingNow = false;
    unsigned theOtherThread = 0, thisThread = 0;
    unsigned thread1 = 0, thread2 = 0;

    //Convert the data to the appropriate format:
    st_ThreadData *data = (st_ThreadData*)iThreadData;
    thisThread = data->m_ID;

    //Start the MCMC:
    for( unsigned long cycle = 0; cycle < data->m_NumberOfCycles; ++cycle )
    {
        //Run the MCMC for the current cycle:
        data->m_NumberOfAcceptedMoves = data->m_Chain->MCMC( data->m_GenerationsPerCycle, data->m_WhichFeatures, data->m_AlterAncestralProb, data->m_AlterMissingProb, data->m_AlterBranchProb, data->m_AlterRatesProb, false );

        //See if this chain must be swapped now:
        swappingNow = false;

        thread1 = data->m_MCMCMC->m_SwappingCandidateChais[cycle].m_Chain1;
        thread2 = data->m_MCMCMC->m_SwappingCandidateChais[cycle].m_Chain2;

        if( thread1 == thisThread )
        {
            theOtherThread = thread2;
            swappingNow = true;
        }
        else if( thread2 == thisThread )
        {
            theOtherThread = thread1;
            swappingNow = true;
        }

        if( swappingNow )
        {
            //Lock the appropriate mutex
            pthread_mutex_lock( &data->m_MCMCMC->m_SynchronizingThreadsMutex[thread1][thread2] );

                //Signal that I've got here:
                data->m_MCMCMC->m_SynchronizingThreadsFlag[thisThread][theOtherThread] = true;

                //See if the other got here before me:
                if( data->m_MCMCMC->m_SynchronizingThreadsFlag[theOtherThread][thisThread] == true )
                {
                    //Ok, so the other was waiting for me: do the swap, reset the data and free it!

                    //Do the swap...
                    data->m_MCMCMC->SwapChains( thisThread, theOtherThread );

                    //print the progress...
                    if( data->m_MCMCMC->m_PrintProgress )
                    {
                        if( data->m_MCMCMC->m_DoneSoFar == data->m_MCMCMC->m_OnePercent )
                        {
                            PrintProgress( cycle, 0, data->m_MCMCMC->m_CurrentPercent );
                            data->m_MCMCMC->m_DoneSoFar = 0;
                            ++data->m_MCMCMC->m_CurrentPercent;
                        }
                        else
                        {
                            ++data->m_MCMCMC->m_DoneSoFar;
                        }
                    }

                    //Reset the data:
                    data->m_MCMCMC->m_SynchronizingThreadsFlag[thisThread][theOtherThread] = data->m_MCMCMC->m_SynchronizingThreadsFlag[theOtherThread][thisThread] = false;

                    //Release the other thread:
                    pthread_cond_signal( &data->m_MCMCMC->m_SynchronizingThreadsCondition[thread1][thread2] );
                }
                else
                {
                    //The other got here first: wait to release me...
                    pthread_cond_wait( &data->m_MCMCMC->m_SynchronizingThreadsCondition[thread1][thread2],
                                       &data->m_MCMCMC->m_SynchronizingThreadsMutex[thread1][thread2] );
                }

            //Unlock the appropriate mutex
            pthread_mutex_unlock( &data->m_MCMCMC->m_SynchronizingThreadsMutex[thread1][thread2] );
        }
    }

        //cout << "THREAD  " << data->m_ID << " FINISHES!!!!" << endl;
    return NULL;
}

//Print the MCMC progress:
void MetropolisCoupledMCMC::PrintProgress( unsigned long iCurrentGeneration, unsigned long iTotalGenerations, unsigned iCurrentPercent )
{
    //cout << iCurrentPercent << "%";
    //fprintf( stdout, "." );
    fprintf( stdout, "%1d", iCurrentPercent );
    fflush( stdout );
}

//Swap two chains:
void MetropolisCoupledMCMC::SwapChains( void )
{
    unsigned chain1, chain2;

    if( m_NumberOfChains == 0 )
    {
        assert( false );
    }
    else if( m_NumberOfChains == 1 )
    {
        //Nothing to swap!
        return;
    }
    else if( m_NumberOfChains == 2 )
    {
        //Clear what to swap:
        chain1 = 0;
        chain2 = 1;
    }
    else
    {
        //Pick two random chains to exchange states:
        m_Permutation.GenerateRandomPermutation();
        chain1 = m_Permutation[0];
        chain2 = m_Permutation[1];
    }

    SwapChains( chain1, chain2 );
}

void MetropolisCoupledMCMC::SwapChains( unsigned iChain1, unsigned iChain2 )
{
    //Assume that the mutex is already managed outside...

    /******************************************************************************
    * Compute the swap acceptance ratio:
    * In Altekar et al. (2004) Bioinformatics 20:407-415 it is given as:
    *
    *
    *                   beta             beta
    *                       j                k
    *         f(psi |X)       * f(psi |X)
    *              k                 j
    * min( 1, --------------------------------- )
    *                   beta             beta
    *                       j                k
    *         f(psi |X)       * f(psi |X)
    *              j                 k
    *
    * which, after applying Bayes' theorem, reduces to:
    *
    *                            beta - beta
    *                                j      k
    *         /p(X|psi ) p(psi )\
    *         |       k       k |
    * min( 1, |-----------------|              )
    *         |p(X|psi ) p(psi )|
    *         \       j       j /
    *
    * where p(X|psi) is the likelihood and p(psi) is the prior probability
    ******************************************************************************/

    DblType loglikelihood1 = m_Chains[iChain1].GetCurrentLogLikelihood();
    DblType loglikelihood2 = m_Chains[iChain2].GetCurrentLogLikelihood();
    DblType logprior1 = m_Chains[iChain1].GetCurrentLogPrior();
    DblType logprior2 = m_Chains[iChain2].GetCurrentLogPrior();
    DblType temp1 = m_Temperatures[iChain1];
    DblType temp2 = m_Temperatures[iChain2];

    DblType swapAcceptanceRatio = min(1.0, exp( (temp2 - temp1) * (loglikelihood1 - loglikelihood2 + logprior1 - logprior2) ) );

    if( swapAcceptanceRatio == 1.0 || RandomProbability() < swapAcceptanceRatio )
    {
        //Swap states: for optimization, change only the temperatures:
        MarkovChain::SwapTemperatures( m_Chains[iChain1], m_Chains[iChain2] );
        DblType aux = m_Temperatures[iChain1];
        m_Temperatures[iChain1] = m_Temperatures[iChain2];
        m_Temperatures[iChain2] = aux;
    }
}

void MetropolisCoupledMCMC::Print( FILE *iFile, int iWhichFeature )
{
    fprintf( iFile, "Metropolis-Couples MCMC with %d heated chains, with heating coefficient %.4f:\n", m_NumberOfChains, m_HeatingCoeff );
    for( unsigned i = 0; i < m_NumberOfChains; ++i )
    {
        fprintf( iFile, ">> Chain %d, temperature %.4f:\n", i, m_Temperatures[i] );
        m_Chains[i].Print( iFile, iWhichFeature );
        fprintf( iFile, "\n" );
    }
    fprintf( iFile, "\n" );
}




