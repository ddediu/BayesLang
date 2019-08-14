#ifndef DEFINITIONS_H_INCLUDED
#define DEFINITIONS_H_INCLUDED

/*************************************************************************
*                                                                        *
*                   Global defintions and includes                       *
*                                                                        *
**************************************************************************/

#include <iostream>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <vector>

using namespace std;

//The floating point type:
typedef float DblType; //Could be either float (faster) or double (more precision)

//The maximum number of feature values:
#define MAX_NUMBER_OF_FEATURE_VALUES 10

//Distribution params:
extern DblType gl_BranchLengthPriorExponentialLambda; //The lambda of the exponential prior
extern DblType gl_BranchLengthProposalMultiplierDistributionParam; //The "a" of the multiplier sliding window
extern DblType gl_RateProposalSlidingWindowWidth; //The rate's sliding widows width (rates are between 0.0 and 2.0)

//Software params:
extern bool gl_Software_SMP_Implementation; //Use multithreading/multicores/multiprocessors?

//Print the trees?
extern bool gl_PrintTrees;

//The error tolerance for the Exponential_PadeApprox funtion:
extern unsigned gl_Exponential_PadeApprox_Error_Tolerance;


//Global functions:
unsigned long Factorial( unsigned iN );
DblType RandomDouble( DblType iMin, DblType iMax );
DblType RandomProbability( void );
int RandomInt( int iMin, int iMax );
DblType RandExponential( DblType iLambda );


#endif // DEFINITIONS_H_INCLUDED
