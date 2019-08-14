#ifndef SQUAREMATRIX_H
#define SQUAREMATRIX_H

#include "Definitions.h"

//The matrix datatype:
typedef DblType tMatrixDataTye;

//The maximum size of a matrix:
#define MAX_SQUARE_MATRIX_SIZE MAX_NUMBER_OF_FEATURE_VALUES
#define MAX_SQUARE_MATRIX_ALLOCATED_SIZE (MAX_SQUARE_MATRIX_SIZE * MAX_SQUARE_MATRIX_SIZE)
#define MAX_SQUARE_MATRIX_ALLOCATED_SIZE_IN_BYTES (MAX_SQUARE_MATRIX_ALLOCATED_SIZE * sizeof(tMatrixDataTye))

/*******************************************************************
*
*         Implements a real quare matrix & fast operations
*       To speed up memory allocation, arranged as a vector
*
* ATTENTION: The approximation seems valid up to branch length~10
*
*******************************************************************/

class SquareMatrix
{
    public:
        SquareMatrix();
        SquareMatrix( unsigned iSize );
        SquareMatrix( SquareMatrix &iM );
        virtual ~SquareMatrix();

        void Init( unsigned iSize );

        //Getters and setter:
        unsigned GetSize( void ){ return m_Size; }
        tMatrixDataTye GetCell( unsigned iX, unsigned iY ){ return *GetCellAddress( iX, iY ); }
        void SetCell( unsigned iX, unsigned iY, tMatrixDataTye iVal ){ *GetCellAddress( iX, iY ) = iVal; }

        void Print( FILE *iFile );
        unsigned Print( char *iBuffer );

        //The identiy matrix:
        void Identity( void );
        //The null matrix:
        void Null( void );
        //A random matrix:
        void Random( void );

        //Copy operator:
        SquareMatrix &operator=( SquareMatrix &iM );
        void CopyContent( SquareMatrix &iMatrix );

        //Equality test:
        bool operator==( SquareMatrix &iM );

        //Boolean matrices (only 0.0 and 1.0):
        bool IsBoolean( void );
        void MakeBoolean( void );

        //Matrix operations:
        //Exponentiation of matrix iM: returns e^(iM^iT):
        friend void Exponential( SquareMatrix &iM, tMatrixDataTye iT, SquareMatrix &oResult );
        //Scalar multiplication:
        friend void MultScalar( SquareMatrix &iM, tMatrixDataTye iScalar, SquareMatrix &oResult );
        //Matrix addition:
        friend void AddMat( SquareMatrix &iM1, SquareMatrix &iM2, SquareMatrix &oResult );
        //Matrix multiplication:
        friend void MultMat( SquareMatrix &iM1, SquareMatrix &iM2, SquareMatrix &oResult );
        //Rise a matrix to an integer (positivge) power:
        friend void PowMat( SquareMatrix &iM, unsigned iPower, SquareMatrix &oResult );
        //Gausssian elimination:
        friend void GaussianElimination( SquareMatrix &iM1, SquareMatrix &iM2, SquareMatrix &oResult );
        //L & U decomposition:
        friend void ComputeLandU( SquareMatrix &iM, SquareMatrix &oL, SquareMatrix &oU );
        friend void ForwardSubstitutionRow( SquareMatrix &ioM, tMatrixDataTye *ioRow );
        friend void BackSubstitutionRow( SquareMatrix &ioM, tMatrixDataTye *ioRow );

        //Likelihood computation:
        DblType ComputeLikelihood( int iAncestorValue, int iDescendantValue, DblType iBranchLength );
    protected:
    private:
        //Matrix size:
        unsigned m_Size;

        //The allocated size:
        unsigned m_AllocatedSize;

        //The actual data:
        tMatrixDataTye m_Data[MAX_SQUARE_MATRIX_ALLOCATED_SIZE];

    private:
        //Convert two indices to one:
        tMatrixDataTye *GetCellAddress( unsigned iX, unsigned iY ){ return m_Data + iX + iY*m_Size; }
        //Memory management:
        void AllocMemory( unsigned iSize );
        void FreeMemory( void );

        //Computation housekeeping stuff:
        friend unsigned LogBase2Plus1( tMatrixDataTye iX );

        //Exponentiation of matrix iM: returns e^(iM^iT): this approximation is very good for iT < 20!!!
        friend void Exponential_PadeApprox( SquareMatrix &iM, tMatrixDataTye iT, SquareMatrix &oResult );

        //The Floyd-Warshall shortest path algorithm:
        friend void ShortestPath_FloydWarshall( SquareMatrix &iM, SquareMatrix &oResult );
};

#endif // SQUAREMATRIX_H
