/*  BayesLang 0.1
 *
 *  (c) Dan Dediu, 2008
 *  Max Planck Institute for Psycholinguistics, Nijmegen
 *  ddediu@gmail.com
 *
 *  Contains code based on (and acknowledged where appropriate as such):
 *
 *    MrBayes 3.1.2
 *
 *    copyright 2002-2005
 *
 *    John P. Huelsenbeck
 *    Section of Ecology, Behavior and Evolution
 *    Division of Biological Sciences
 *    University of California, San Diego
 *    La Jolla, CA 92093-0116
 *
 *    johnh@biomail.ucsd.edu
 *
 *	  Fredrik Ronquist
 *    School of Computational Science
 *    Florida State University
 *    Tallahassee, FL 32306-4120
 *
 *    ronquist@csit.fsu.edu
 *
 * This is released under GPL (see GPL.txt for details).
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details (www.gnu.org).
 *
 */

#include "Definitions.h"
#include "LangNexusToken.h"
#include "LangNexusReader.h"
#include "NxsTypologyBlock.h"
#include "InfoHolder.h"
#include "SquareMatrix.h"
#include "FeatureRateMatrix.h"
#include "MarkovChain.h"
#include "MetropolisCoupledMCMC.h"
#include <pthread.h>

//The start-up message:
const char *gl_StartUpMessage = "BayesLang (c) Dan Dediu, 2008, ddediu@gmail.com\n"\
                                "Max Planck Institute for Psycholinguistics, Nijmegen\n\n"\
                                "This is released under GPL (see GPL.txt for details).\n"\
                                "  This program is free software; you can redistribute it and/or\n"\
                                "  modify it under the terms of the GNU General Public License\n"\
                                "  as published by the Free Software Foundation; either version 2\n"\
                                "  of the License, or (at your option) any later version.\n\n"\
                                "  This program is distributed in the hope that it will be useful,\n"\
                                "  but WITHOUT ANY WARRANTY; without even the implied warranty of\n"\
                                "  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"\
                                "  GNU General Public License for more details (www.gnu.org and gpl.txt).\n\n"\
                                "Contains code based on \"MrBayes 3.1.2\" (c) J.P. Huelsenbeck & F. Ronquist\n"\
                                "Uses the \"NCL 2.1.04\" library (c) P.O. Lewis\n"\
                                "The Windows version uses the \"pthreads-win32\" library by Ross Johnson released under LGPL (for details, see lgpl.txt)\n\n"\
                                "Usage:\n"\
                                "  BayesLang -ngens=n1 -nchains=n2 nf -trees\n"\
                                "where:\n"\
                                "  nf: Nexus file containing the languages, features and templates (see manual)\n"\
                                "  n1: the number of generations to run (default 1000000)\n"\
                                "  n2: the number of chains (at least 1; default 4)\n"\
                                "  -trees: save the trees in the results file\n"\
                                "--------------------------------------------------------------------------------\n\n";

//Distribution params:
DblType gl_BranchLengthPriorExponentialLambda = 10.0; //The lambda of the exponential prior
DblType gl_BranchLengthProposalMultiplierDistributionParam = 2.0; //The "a" of the multiplier sliding window
DblType gl_RateProposalSlidingWindowWidth = 0.1; //The rate's sliding widown widht (rates are between 0.0 and 2.0)

//The error tolerance for the Exponential_PadeApprox funtion:
unsigned gl_Exponential_PadeApprox_Error_Tolerance = 0;

//Software params:
bool gl_Software_SMP_Implementation = true; //Use multithreading/multicores/multiprocessors?

//Print the trees?
bool gl_PrintTrees = false;

//The global holders of information:
InfoHolder gl_Model;

//Read and parse the Nexus file:
bool ReadNexusFile( const char *iFileName );

int main( int argc, char **argv )
{
    //Init the random generator:
    srand( time( NULL ) );

    //Display the startup message:
    cout << gl_StartUpMessage;

    //Compute the error tolerance for the Pade Approximation for matrix exponentiation:
    DblType tolerance = 0.0000001;//0.000000001;
    unsigned q;
    for( q = 0; pow(2.0, 3.0 - 2*q) * Factorial(q) * Factorial (q) / (Factorial(2*q) * Factorial (2*q+1)) > tolerance; ++q )
        ;
    gl_Exponential_PadeApprox_Error_Tolerance = q;

    //Display software params:
#ifdef PTHREADS_WIN32
    cout << endl << (gl_Software_SMP_Implementation ? "SMP Implmentation (uses pthreads-win32, http://sourceware.org/pthreads-win32)" : "Sequential Implementation") << "..." << endl << endl;
#else
    cout << endl << (gl_Software_SMP_Implementation ? "SMP Implmentation (uses pthreads)" : "Sequential Implementation") << "..." << endl << endl;
#endif //PTHREADS_WIN32

    //Simulation params:
    unsigned long generations = 1000000;
    int nChains = 4;

    //Parse the params:
    const char *ngenarg="-ngen=";
    const char *nchainsarg="-nchains=";
    const char *treesarg = "-trees";
    bool readNexusFile = false;
    unsigned i;
    for( i = 1; (int)i < argc; ++i )
    {
        if( strncmp( argv[i], ngenarg, strlen(ngenarg) )== 0 )
        {
            if( sscanf( argv[i]+strlen(ngenarg), "%lu", &generations ) != 1 )
            {
                cerr << "Error reading the number of generations \"" << argv[i] << "\"!" << endl;
                assert( false );
                exit( 1 );
            }
            cout << "Runing " << generations << " generations..." << endl;
        }
        else if( strncmp( argv[i], nchainsarg, strlen(nchainsarg) )== 0 )
        {
            if( sscanf( argv[i]+strlen(nchainsarg), "%d", &nChains ) != 1 )
            {
                cerr << "Error reading the number of chains \"" << argv[i] << "\"!" << endl;
                assert( false );
                exit( 1 );
            }
            if( nChains < 1 )
            {
                cerr << "There must be at least one chain!" << endl;
                assert( false );
                exit( 1 );
            }
            cout << "Runing " << nChains << " chains..." << endl;
       }
       else if( strncmp( argv[i], treesarg, strlen(treesarg) )== 0 )
       {
            gl_PrintTrees = true;
       }
       else
       {
           if( readNexusFile == true )
           {
               cerr << "Unknown param \"" << argv[i] << "\"!" << endl;
               assert( false );
               exit( 1 );
           }

            //Try to parse the Nexus file:
            if( !ReadNexusFile( argv[i] ) )
            {
                cout << "Error reading the Nexus file! Aborting..." << endl;
                assert( false );
                exit( 1 );
            }
            readNexusFile = true;
       }
    }

    if( !readNexusFile )
    {
        cerr << "The Nexus file is required! Aborting..." << endl;
        assert( false );
        exit( 1 );
    }

    //Test matrix:
    /*SquareMatrix m(10);
    m.Identity();
    m.Print( stdout );
    MultScalar( m, 10, m );
    m.Print( stdout );

    unsigned x = 20;
    cout << x << "!=" << Factorial(x) << endl;

    SquareMatrix m1(10),m2(10);
    m1.Identity();
    m2.Identity();
    MultScalar(m1,2,m1);
    MultMat(m1,m1,m1);
    m1.Print( stdout );

    m.Random();
    m.Print( stdout );
    SquareMatrix m4 = m;
    ComputeLandU(m,m1,m2);
    m1.Print( stdout );
    m2.Print( stdout );
    SquareMatrix m3(10);
    MultMat(m1,m2,m3);
    m3.Print( stdout );
    m4.Print( stdout );
    cout << (m4 == m3) << endl;

    m.Random();
    m.Print( stdout );
    Exponential( m, 1, m1 );
    m.Print( stdout );
    m1.Print( stdout );*/

    /*SquareMatrix q1(2),q2(2),q3(2),q4(2);
    q1.SetCell(0,0,-0.1);
    q1.SetCell(0,1,0.1);
    q1.SetCell(1,0,0.3);
    q1.SetCell(1,1,-0.3);
    q1.Print( stdout );

    DblType t = 50.0;
    cout << "e^(m*" << t << "):" << endl;
    Exponential(q1,t,q2);
    q2.Print( stdout );*/

    time_t startTime, endTime;
    time(&startTime);

    /*//Test log and exp:
    for( unsigned i = 0; i< 10; ++i )
    {
        DblType x=RandomProbability();
        cout << "x=" << x << " log(x)=" << log(x) << " exp(x)=" << exp(x) << endl;
    }
    return 0;*/

    /*//Test matrix exponentiation:
    SquareMatrix q1(3),q2(3);
    q1.SetCell(0,0,-0.3);
    q1.SetCell(0,1,0.1);
    q1.SetCell(0,2,0.2);
    q1.SetCell(1,0,0.4);
    q1.SetCell(1,1,-0.5);
    q1.SetCell(1,2,0.1);
    q1.SetCell(2,0,0.3);
    q1.SetCell(2,1,0.3);
    q1.SetCell(2,2,-0.6);
    q1.Print( stdout );

    DblType t = 10;
    cout << "e^(m*" << t << "):" << endl;
    Exponential(q1,t,q2);
    q2.Print( stdout );*/


    /*//Test feature likelihood computation:
    FeatureRateMatrix featureRateMatrix;
    featureRateMatrix.SetFeature( gl_Model.GetFeature(0) );
    SquareMatrix q1(2);
    q1.SetCell(0,0,-0.1);
    q1.SetCell(0,1,0.1);
    q1.SetCell(1,0,0.3);
    q1.SetCell(1,1,-0.3);
    featureRateMatrix.SetRateMatrix( q1 );

    featureRateMatrix.Print( stdout );

    DblType t = 1.0;
    cout << "p(0->0|t=" << t << ")=" << featureRateMatrix.ComputeLikelihood('0','0',t ) << endl;
    cout << "p(0->1|t=" << t << ")=" << featureRateMatrix.ComputeLikelihood('0','1',t ) << endl;
    cout << "p(1->0|t=" << t << ")=" << featureRateMatrix.ComputeLikelihood('1','0',t ) << endl;
    cout << "p(1->1|t=" << t << ")=" << featureRateMatrix.ComputeLikelihood('1','1',t ) << endl;*/



    /*//Tree likelihood:
    TreeTemplate *tree1 = gl_Model.GetTreeTemplate(1);
    tree1->GenerateMissingValues(-1);
    tree1->ComputeLikelihood(-1);
    tree1->Print( stdout, false, -1, true, true );

    cout << endl << "Generate new candidate tree for feature 3:" << tree1->NewCandidate( 3, 1.0, 1.0, 1.0, 1.0 ) << endl;
    tree1->ComputeLikelihood(3);
    tree1->Print( stdout, false, 3, true, true );


    //Test tree copying:
    TreeTemplate tree2;
    cout << endl << endl << "*** tree2 = *tree1 ***" << endl;
    tree2 = *tree1;
    tree1->Print( stdout, true, 0, false, false );
    cout << endl;
    tree2.Print( stdout, true, 0, true, 0 );

    TreeTemplate tree3;
    cout << endl << endl << "*** tree3 = *tree1 ***" << endl;
    tree3 = *tree1;
    tree3.Print( stdout, true, 0, true, 0 );

    //cout << endl << endl << "*** tree3.NewCandidate( 0, 1.0, 1.0, 1.0, 1.0 ) ***: Hastings ratio=" << tree3.NewCandidate( 0, 1.0, 1.0, 1.0, 1.0 ) << endl;
    cout << endl << endl << "*** tree3.NewCandidate( 0, 0.0, 0.0, 0.0, 1.0 ) ***: Hastings ratio=" << tree3.NewCandidate( 0, 0.0, 0.0, 0.0, 1.0 ) << endl;
    tree3.Print( stdout, true, 0, true, 0 );
    tree3.Print( stdout, false, 0, true, true );

    cout << endl << endl << "*** tree3.CopyContent(*tree1) ***" << endl;
    tree3.CopyContent( *tree1 );
    tree3.Print( stdout, true, 0, true, 0 );*/


    //cout << endl << "L(tree|all.feats)=" << tree1->ComputeLikelihood(0) << endl;

    /*//Test likelihood:
    TreeTemplate *tree1 = gl_Model.GetTreeTemplate(1);
    tree1->GenerateMissingValues(-1);

    tree1->ComputeLikelihood(0);
    tree1->Print( stdout, false, 0, true, true );

    //tree1->ComputeLikelihood(1);
    //tree1->Print( stdout, false, 1, true, true );

    //tree1->ComputeLikelihood(2);
    //tree1->Print( stdout, false, 2, true, true );

    //tree1->ComputeLikelihood(-1);
    //tree1->Print( stdout, false, -1, true, true );

    cout << endl << endl << "*** tree1.NewCandidate( 0, 0.0, 0.0, 1.0, 0.0 ) ***: Hastings ratio=" << tree1->NewCandidate( 0, 0.0, 0.0, 1.0, 0.0 ) << endl;
    tree1->ComputeLikelihood(0);
    tree1->Print( stdout, false, 0, true, true );

    return 0;*/

    /*//Test the MCMC:
    MarkovChain mc( *gl_Model.GetTreeTemplate(1) );
    mc.Print( stdout, 3 );

    unsigned long generations = 10000000;
    cout << "After " << generations << " generations, " << mc.MCMC( generations, 3, 0.5, 0.5, 0.5, 0.5 ) << " moves were accepted." << endl;

    cout << "Final candidate: " << endl;
    mc.Print( stdout, 3 );*/

    /*//Test MCMCMC:
    int whichFeature = -1;
    MetropolisCoupledMCMC mc( *gl_Model.GetTreeTemplate(1), nChains, 0.20, 50 );
    //mc.Print( stdout, whichFeature );

    //cout << "After " << generations << " generations, " << mc.MCMCMC( generations, whichFeature, 0.5, 0.5, 0.5, 0.5 ) << " moves were accepted." << endl;
    mc.MCMCMC( generations, whichFeature, 0.5, 0.5, 0.1, 0.0 );
    cout << "Processed " << generations << " generations!" << endl;*/


    //The features to be processes:
    int whichFeature = -1;

    //Save the list of trees (language families processed):
    FILE *file = fopen( "MCMCTreesInfo.csv", "wt" );
    if( !file )
    {
        cerr << "Error creating file \"" << "MCMCTreesInfo.csv" << "\" containing the list of processed language families!" << endl;
        assert( false );
        exit( 1 );
    }
    fprintf( file, "TreeName\n" );
    for( i = 0; i < gl_Model.GetNTreeTemplates(); ++i )
    {
        fprintf( file, "%s\n", gl_Model.GetTreeTemplate(i)->GetName() );
    }
    fclose( file );

    //For each tree in the trees set, do the MCMCMC independently and export it to file:
    for( i = 0; i < gl_Model.GetNTreeTemplates(); ++i )
    {
        //Display progress info:
        cout << "Processing tree " << gl_Model.GetTreeTemplate(i)->GetName() << " (" << (i+1) << " out of " << gl_Model.GetNTreeTemplates() << ")...";

        //Set the results file names:
        char treesFileName[10240];
        sprintf( treesFileName, "MCMCResults-%s.csv", gl_Model.GetTreeTemplate(i)->GetName() );
        MarkovChain::st_TreesFileName = treesFileName;

        char treesStructFileName[10240];
        sprintf( treesStructFileName, "MCMCModelStructure-%s.csv", gl_Model.GetTreeTemplate(i)->GetName() );
        MarkovChain::st_TreeStructureFileName = treesStructFileName;

        char treesFeaturesFileName[10240];
        sprintf( treesFeaturesFileName, "MCMCFeaturesInfo-%s.csv", gl_Model.GetTreeTemplate(i)->GetName() );
        InfoHolder::st_FeaturesFileName = treesFeaturesFileName;

        //Init the MCMCMC for this tree:
        MetropolisCoupledMCMC mc( *gl_Model.GetTreeTemplate(i), nChains, 0.20, 50 );

        //Do the MCMCMC:
        mc.MCMCMC(  generations, whichFeature,
                    0.5, // iAlterAncestralProb
                    0.5, // iAlterMissingProb
                    0.1, // iAlterBranchProb
                    0.1  // iAlterRatesProb
                 );

        time(&endTime);
        cout << " so far took: " << endTime - startTime << " seconds" << endl;

    }


    time(&endTime);
    cout << endl << "Time taken: " << endTime - startTime << endl;

    return 0;
}

//Read and parse the Nexus file:
bool ReadNexusFile( const char *iFileName )
{
    cout << "Reading & parsing Nexus input file \"" << iFileName << "\"..." << endl << endl;

    //Alloc the sections of the Nexus file:
    NxsTaxaBlock	   *taxa	  = NULL;
    NxsCharactersBlock *charactes = NULL;
    NxsTreesBlock	   *trees	  = NULL;
    NxsTypologyBlock   *typology  = NULL;

	taxa = new NxsTaxaBlock();
	charactes = new NxsCharactersBlock(taxa,NULL);
	trees = new NxsTreesBlock(taxa);
	typology = new NxsTypologyBlock(charactes);

    //Read and parse the Nexus file:
	LangNexusReader nexus(iFileName);
	nexus.Add(taxa);
	nexus.Add(charactes);
	nexus.Add(trees);
	nexus.Add(typology);

	LangNexusToken token(nexus.inf, nexus.outf);
	nexus.Execute(token);

	taxa->Report(nexus.outf);
	charactes->Report(nexus.outf);
	trees->Report(nexus.outf);
	typology->Report(nexus.outf);

	//Process the info:
	gl_Model.Init( taxa, charactes, typology, trees );

	gl_Model.Print( stdout );

	//Free the memory:
	delete taxa;
	delete charactes;
	delete trees;
	delete typology;

    return true;
}

//Global functions:
unsigned long Factorial( unsigned iN )
{
    unsigned long fact = 1;
    for( ; iN > 0; --iN )
    {
        fact *= iN;
    }
    return fact;
}

DblType RandomDouble( DblType iMin, DblType iMax )
{
    return iMin + iMax * (float)rand()/(float)RAND_MAX;
}

DblType RandomProbability( void )
{
    return (float)rand()/(float)RAND_MAX;
}

int RandomInt( int iMin, int iMax )
{
    assert( iMin <= iMax );
    if( iMin == iMax )
    {
        return iMin;
    }
    return iMin + rand() % (iMax - iMin + 1);
}

DblType RandExponential( DblType iLambda )
{
    assert( iLambda > 0 );
    return -log( RandomProbability() )/iLambda;
}


