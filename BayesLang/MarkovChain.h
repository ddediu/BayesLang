#ifndef MARKOVCHAIN_H
#define MARKOVCHAIN_H

#include "Definitions.h"
#include "TreeTemplate.h"

/************************************************************************
*
*                 Contains a Markov Chain of trees
*
************************************************************************/

class MarkovChain
{
    public:
        MarkovChain();
        MarkovChain( TreeTemplate &iTreeTemplate, DblType iTemperature = 1.0, bool iRandomStartingTree = false );
        void Init( TreeTemplate &iTreeTemplate, DblType iTemperature = 1.0, bool iRandomStartingTree = false );
        virtual ~MarkovChain();

        void Print( FILE *iFile, int iWhichFeature );

        //Generate a new cadidate (return true if the candidate was accepted):
        bool NewCandidate( int iWhichFeature = (-1), DblType iAlterAncestralProb = 1.0,
                           DblType iAlterMissingProb = 1.0, DblType iAlterBranchProb = 1.0,
                           DblType iAlterRatesProb = 1.0 );

        //Run the MCMC for N generations and return the number of accepted moves:
        unsigned long MCMC( unsigned long iGenerations, int iWhichFeature = (-1), DblType iAlterAncestralProb = 1.0,
                           DblType iAlterMissingProb = 1.0, DblType iAlterBranchProb = 1.0,
                           DblType iAlterRatesProb = 1.0, bool iPrintProgress = true );

        //Get the current loglikelihood and prior (unheated):
        DblType GetCurrentLogLikelihood( void ){ return m_CurrentLogLikelihood; }
        DblType GetCurrentLogPrior( void ){ return m_CurrentLogPrior; }

        unsigned long GetGeneration( void ){ return m_Generation; }

        //Swap the states of two MCMC at different temperatures (actually, swap just the temperatures):
        static void SwapTemperatures( MarkovChain &iMC1, MarkovChain &iMC2 );

        //Reopen the results file:
        static void ReOpenResultsFile( void )
        {
            st_TreesFile = fopen( st_TreesFileName, "at" );
            if( !st_TreesFile )
            {
                cerr << "Error writing the current tree to file!" << endl;
                assert( false );
                exit( 1 );
            }
            st_TressFileCache.m_TreesFile = st_TreesFile;
        }
        //Close the results file:
        static void CloseResultsFile( void )
        {
            if( st_TreesFile ) fclose(st_TreesFile);
            st_TreesFile = st_TressFileCache.m_TreesFile = NULL;
        }
        //Flush the case:
        static void FlushCache( void ){ st_TressFileCache.FlushCache( true ); }

    protected:
    private:
        //The chain's temperature:
        DblType m_Temperature;

        //The current tree:
        TreeTemplate m_CurrentTree;

        //The candidate tree:
        TreeTemplate m_CandidateTree;

        //The current generation:
        unsigned long m_Generation;
        //Save the info every X generations:
        unsigned long m_SaveEveryNGenerations;

        //Store the current loglikelihood and prior:
        DblType m_CurrentLogLikelihood;
        DblType m_CurrentLogPrior;
        bool m_CurrentLogLikelihoodLogPriorInitialized; //Have these two been initialized?

    private:
        //Save the relevant info in the case of a successful move:
        void SaveInfo( int iWhichFeature, bool iNewStuff );
        //Print the MCMC progress:
        void PrintProgress( unsigned long iCurrentGeneration, unsigned long iTotalGenerations, unsigned iCurrentPercent );

        //Memory cache for the results file (TreesFile) to speed up the process:
        struct st_TreesFileCache_
        {
            #define MAX_RESULTS_FILE_CACHE_SIZE 10000
            //#define MAX_RESULTS_FILE_CACHE_SIZE 10

            //One line in the results file
            struct st_TreesFileCacheLine
            {
                st_TreesFileCacheLine( void ):m_Content(NULL),m_Counter(0){}
                ~st_TreesFileCacheLine( void ){ if( m_Content ) delete[] m_Content; }
                void Init( char *iContent )
                {
                    if( m_Content )
                        delete[] m_Content;
                    if( iContent )
                    {
                        m_Content = new char[strlen( iContent ) + 1];
                        if( !m_Content )
                        {
                            cerr << "Not enough memory!" << endl;
                            assert( false );
                            exit( 1 );
                        }
                        strcpy( m_Content, iContent );
                    }
                    else
                    {
                        m_Content = NULL;
                    }
                    m_Counter = 1;
                }
                void Flush( FILE *iFile )
                {
                    fprintf( iFile, "%lu\t%s", m_Counter, m_Content );
                }
                st_TreesFileCacheLine &operator=( st_TreesFileCacheLine &iLine )
                {
                    if( m_Content )
                        delete[] m_Content;
                    if( iLine.m_Content )
                    {
                        m_Content = new char[strlen( iLine.m_Content ) + 1];
                        if( !m_Content )
                        {
                            cerr << "Not enough memory!" << endl;
                            assert( false );
                            exit( 1 );
                        }
                        strcpy( m_Content, iLine.m_Content );
                    }
                    else
                    {
                        m_Content = NULL;
                    }
                    m_Counter = iLine.m_Counter;
                    return *this;
                }

                //The actual content or NULL if this is a duplicated line:
                char *m_Content;
                //The number of times this content was saved:
                unsigned long m_Counter;
            };

            st_TreesFileCache_( void ):m_NumberOfLines(0){}
            ~st_TreesFileCache_( void ){}

            //Add duplicated line:
            void AddDuplicatedLine( void )
            {
                assert( m_NumberOfLines > 0 );
                m_Lines[m_NumberOfLines-1].m_Counter++;
            }
            //Add new line:
            void AddNewLine( char *iLine )
            {
                assert( iLine );
                FlushCache( false );
                m_Lines[m_NumberOfLines++].Init( iLine );
            }

            //If the cache is full or if forced to, flush the cache to file:
            void FlushCache( bool iForceFlush = false )
            {
                if( iForceFlush || m_NumberOfLines == MAX_RESULTS_FILE_CACHE_SIZE )
                {
                    //cout << "Flushing cache to file..." << endl;
                    //Flush it to file:
                    assert( m_TreesFile );
                    for( unsigned i = 0; i < m_NumberOfLines; ++i )
                    {
                        m_Lines[i].Flush( m_TreesFile );
                    }
                    fflush( m_TreesFile );

                    //Resent info:
                    m_NumberOfLines = 0;
                }
            }

            //The vector of lines:
            unsigned m_NumberOfLines;
            st_TreesFileCacheLine m_Lines[ MAX_RESULTS_FILE_CACHE_SIZE + 1 ];

            //The file pointer:
            FILE *m_TreesFile;

            #undef MAX_RESULTS_FILE_CACHE_SIZE
        };
        static st_TreesFileCache_ st_TressFileCache;
        static pthread_mutex_t st_TreesFileCacheMutex;

    public:
        //The trees file name:
        static const char *st_TreesFileName;
        //The trees file:
        static FILE *st_TreesFile;

        //The trees structure file name:
        static const char *st_TreeStructureFileName;
};

#endif // MARKOVCHAIN_H
