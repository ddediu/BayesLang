#ifndef METROPOLISCOUPLEDMCMC_H
#define METROPOLISCOUPLEDMCMC_H

#include "Definitions.h"
#include "MarkovChain.h"
#include <pthread.h>

/********************************************************************************************
*
* This class implements a Metropolis-Coupled Markov Chain Monte Carlo (MCMCMC or (MC)^3)
* with the given number of chains (chain 0 is the cold one) and heating coefficient (lamba),
* using heating scheme T = 1/(1+i*lambda) for the i-th chain
*
*********************************************************************************************/

class MetropolisCoupledMCMC
{
    public:
        MetropolisCoupledMCMC();
        MetropolisCoupledMCMC( TreeTemplate &iTreeTemplate, unsigned iNumberOfChains = 1, DblType iHeatingCoeff = 0.20, unsigned iGenerationsBetweenSwaps = 100 );
        void Init( TreeTemplate &iTreeTemplate, unsigned iNumberOfChains = 1, DblType iHeatingCoeff = 0.20, unsigned iGenerationsBetweenSwaps = 100 );
        virtual ~MetropolisCoupledMCMC();

        void Print( FILE *iFile, int iWhichFeature );

        MarkovChain &GetColdChain( void ){ return m_Chains[0]; }

        //Run the MCMC for N generations and return the number of accepted moves:
        unsigned long MCMCMC( unsigned long iGenerations, int iWhichFeature = (-1), DblType iAlterAncestralProb = 1.0,
                              DblType iAlterMissingProb = 1.0, DblType iAlterBranchProb = 1.0,
                              DblType iAlterRatesProb = 1.0, bool iPrintProgress = true );

    protected:
    private:
        //The heating coefficient:
        DblType m_HeatingCoeff;

        //The number of chains:
        unsigned m_NumberOfChains;

        //The number of generations between swaps:
        unsigned long m_GenerationsBetweenSwaps;

        //The chains:
        MarkovChain *m_Chains;
        //The temperatures:
        DblType *m_Temperatures;

        //A permutation of chain indeices (used for randomly picking two of them for swaping):
        Permutation m_Permutation;

    private:
        //Memory management:
        void AllocMemory( unsigned iNumberOfChains );
        void FreeMemory( void );

        //Swap two chains:
        void SwapChains( void );

        //Print progress:
        static void PrintProgress( unsigned long iCurrentGeneration, unsigned long iTotalGenerations, unsigned iCurrentPercent );


        /***************************************************
        *               Multithreading stuff:
        ***************************************************/
        //Thread data struct:
        struct st_ThreadData
        {
            //The chain:
            MarkovChain *m_Chain;
            //The chain id:
            unsigned m_ID;
            //The temperature:
            DblType m_Temperature;

            //Number of cycles to run:
            unsigned long m_NumberOfCycles;
            //Number of generations to run per cycle:
            unsigned m_GenerationsPerCycle;
            //Which feature(s):
            int m_WhichFeatures;
            //The ancestral states alteration probability:
            DblType m_AlterAncestralProb;
            //The missing data states alteration probability:
            DblType m_AlterMissingProb;
            //The branch length alteration probability:
            DblType m_AlterBranchProb;
            //The rate matrix alteration probability:
            DblType m_AlterRatesProb;

            //The container MCMCMC:
            MetropolisCoupledMCMC *m_MCMCMC;

            //The return value:
            unsigned long m_NumberOfAcceptedMoves;
        };

        //Where the thread data initialized?
        bool m_AreThreadDataInitialized;
        //The thread data:
        st_ThreadData *m_ThreadsData;
        //The thread id's:
        pthread_t *m_TheThreads;

        //Swapping chains info:
        struct st_SwappingCandidateChais
        {
            unsigned m_Chain1;
            unsigned m_Chain2;
        };
        st_SwappingCandidateChais *m_SwappingCandidateChais;

        //Progressprinting stuff:
        bool m_PrintProgress;
        unsigned long m_OnePercent; //Actually, 10%
        unsigned m_CurrentPercent;
        unsigned long m_DoneSoFar;

        pthread_mutex_t m_SwappingMutex; //mutex used to synchronize the swaps data (single one)
        //Mutex and condition one per possible pair, indexed by the two threads numbers:
        pthread_mutex_t **m_SynchronizingThreadsMutex;
        pthread_cond_t  **m_SynchronizingThreadsCondition;
        bool            **m_SynchronizingThreadsFlag;

        //The thread worker function:
        static void *ThreadMCMC( void *iThreadData );
        //Swap two chains:
        void SwapChains( unsigned iChain1, unsigned iChain2 );
};

#endif // METROPOLISCOUPLEDMCMC_H
