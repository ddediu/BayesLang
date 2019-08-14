#include "SquareMatrix.h"

SquareMatrix::SquareMatrix()
:m_Size(0),m_AllocatedSize(0)/*,m_Data(NULL)*/
{
}

SquareMatrix::SquareMatrix( unsigned iSize )
:m_Size(0),m_AllocatedSize(0)/*,m_Data(NULL)*/
{
    AllocMemory( iSize );
}

SquareMatrix::SquareMatrix( SquareMatrix &iM )
:m_Size(0),m_AllocatedSize(0)/*,m_Data(NULL)*/
{
    operator=(iM);
}

SquareMatrix::~SquareMatrix()
{
    FreeMemory();
}

void SquareMatrix::Init( unsigned iSize )
{
    AllocMemory( iSize );
}

//Copy operator:
SquareMatrix &SquareMatrix::operator=( SquareMatrix &iM )
{
    if( m_Size != iM.m_Size )
    {
        AllocMemory( iM.m_Size );
    }

    //Copy the data:
    /*for( unsigned i = 0; i < m_AllocatedSize; ++i )
    {
        m_Data[i] = iM.m_Data[i];
    }*/
    memcpy( m_Data, iM.m_Data, m_AllocatedSize*sizeof(tMatrixDataTye) );

    return *this;
}

void SquareMatrix::CopyContent( SquareMatrix &iMatrix )
{
    assert( m_Size == iMatrix.m_Size );

    memcpy( m_Data, iMatrix.m_Data, m_AllocatedSize*sizeof(tMatrixDataTye) );
}

//Equality test:
bool SquareMatrix::operator==( SquareMatrix &iM )
{
    if( m_Size != iM.m_Size )
    {
        return false;
    }

    for( unsigned i = 0; i < m_AllocatedSize; ++i )
    {
        if( m_Data[i] != iM.m_Data[i] )
        {
            return false;
        }
    }

    return true;
}

void SquareMatrix::AllocMemory( unsigned iSize )
{
    assert( iSize > 0 && iSize <= MAX_SQUARE_MATRIX_SIZE );
    FreeMemory();

    m_Size = iSize;
    m_AllocatedSize = m_Size*m_Size;

    /*m_Data = new tMatrixDataTye[ m_AllocatedSize ];
    if( !m_Data )
    {
        cerr << "Not enough memory!" << endl;
        assert( false );
        exit( 1 );
    }*/
}

void SquareMatrix::FreeMemory( void )
{
    /*if( m_Size && m_Data )
    {
        delete[] m_Data;
    }*/

    m_Size = m_AllocatedSize = 0;
    /*m_Data = NULL;*/
}

//The identiy matrix:
void SquareMatrix::Identity( void )
{
    Null();
    for( unsigned i = 0; i < m_Size; ++i )
    {
        SetCell(i,i,1);
    }
}

//The null matrix:
void SquareMatrix::Null( void )
{
    /*for( unsigned i = 0; i < m_AllocatedSize; ++i )
    {
        m_Data[i] = 0;
    }*/
    memset( m_Data, 0, m_AllocatedSize*sizeof(tMatrixDataTye) );
}

//A random matrix:
void SquareMatrix::Random( void )
{
    for( unsigned i = 0; i < m_AllocatedSize; ++i )
    {
        m_Data[i] = rand() % 10;
    }
}

void SquareMatrix::Print( FILE *iFile )
{
    //fprintf( iFile, "Square matrix of size %d:\n", m_Size );
    for( unsigned x = 0; x < m_Size; ++x )
    {
        fprintf( iFile, x == 0 ? " / " : x == m_Size-1 ? " \\ " : "|  " );
        for( unsigned y = 0; y < m_Size; ++y )
        {
            fprintf( iFile, "% .5f ", (float)GetCell(x,y) );
        }
        fprintf( iFile, x == 0 ? "\\\n" : x == m_Size-1 ? "/\n" : " |\n" );
    }
    //fprintf( iFile, "\n" );
}

unsigned SquareMatrix::Print( char *iBuffer )
{
    char *curChar = iBuffer;

    //fprintf( iFile, "Square matrix of size %d:\n", m_Size );
    for( unsigned x = 0; x < m_Size; ++x )
    {
        curChar += sprintf( curChar, x == 0 ? " / " : x == m_Size-1 ? " \\ " : "|  " );
        for( unsigned y = 0; y < m_Size; ++y )
        {
            curChar += sprintf( curChar, "% .5f ", (float)GetCell(x,y) );
        }
        curChar += sprintf( curChar, x == 0 ? "\\\n" : x == m_Size-1 ? "/\n" : " |\n" );
    }
    //fprintf( iFile, "\n" );

    return curChar - iBuffer;
}


//Computation housekeeping stuff:
unsigned LogBase2Plus1( tMatrixDataTye iX )
{
    //Based on MrBayes' LogBase2Plus1() function:
    unsigned j;
    for( j = 0; iX > 1.0 - 1.0e-07; iX /= 2.0, ++j )
        ;
	return j;
}

//Matrix operations:
//Exponentiation of matrix iM: returns e^(iM^iT):
void Exponential( SquareMatrix &iM, tMatrixDataTye iT, SquareMatrix &oResult )
{
    assert( iT >= 0 );

    if( iT <= 1.0 )
    {
        //Simply call the Pade approximation:
        Exponential_PadeApprox( iM, iT, oResult );
    }
    else
    {
        /*******************************************************************************************
        * Use the idea that iT=1*([iT] + {iT}), which gives that:
        * e^(M*t) = e^(M*([t]+{t})) = e^(M*[t] + M*{t}) = (e^(M*1))^[t]*e^(M*{t})
        *******************************************************************************************/
        DblType integ = floor( iT ); //[iT]
        DblType fract = iT - integ;  //{iT}
        SquareMatrix origM = iM;

        SquareMatrix exp_iM_fract(iM.GetSize()); //Hold the e^(M*{t})
        Exponential_PadeApprox( iM, fract, exp_iM_fract );

        SquareMatrix exp_iM_1(iM.GetSize()); //Hold the e^(M*1)
        Exponential_PadeApprox( origM, 1.0, exp_iM_1 );

        SquareMatrix exp_iM_integ(iM.GetSize()); //Hold the e^(M*{t})
        PowMat( exp_iM_1, (int)integ, exp_iM_integ );

        MultMat( exp_iM_fract, exp_iM_integ, oResult ); //The final result is the product io these two matrices
    }
}

//Exponentiation of matrix iM: returns e^(iM^iT):
void Exponential_PadeApprox( SquareMatrix &iM, tMatrixDataTye iT, SquareMatrix &oResult )
{
    /****************************************************************************************************************************************
    * Based on MrBayes' TiProbsUsingPadeApprox(), SetQvalue(), ComputeMatrixExponential(), DivideByTwos()
    * whis are based in turn on the Pade approximation, described in:
    * Golub, G. H., and C. F. Van Loan. 1996. Matrix Computations, Third Edition. The Johns Hopkins University Press, Baltimore, Maryland.
    ****************************************************************************************************************************************/

    //Preconditions:
    assert( iM.GetSize() == oResult.GetSize() && iT <= 20 );

    /*//Warning message:
    if( iT > 20 )
    {
        cerr << "ATTENTION: the matrix exponential computation seems reliable up to t=20!" << endl;
    }*/

    //Alloc the new matrix:
    SquareMatrix a(iM.GetSize());
    MultScalar( iM, iT, a );

    //Use the approximate exponent based on the desired error tolerance, computed previously in main():
    unsigned q = gl_Exponential_PadeApprox_Error_Tolerance;

    unsigned i, j, k;
    tMatrixDataTye maxAValue, c;
    SquareMatrix d(iM.GetSize()), n(iM.GetSize()), x(iM.GetSize()), cX(iM.GetSize());
    d.Identity();
    n.Identity();
    x.Identity();

	maxAValue = 0;
	for ( i = 0; i < iM.GetSize(); ++i )
	{
		maxAValue = max( maxAValue, a.GetCell(i,i));
	}

	j = max(0, (int)LogBase2Plus1(maxAValue) );

    tMatrixDataTye pow2 = pow(2,j);
    for( i = 0; i < a.m_AllocatedSize; ++i )
    {
        a.m_Data[i] /= pow2;
    }

	c = 1;
	for( k = 1; k <= q; ++k )
    {
		c = c * (q - k + 1.0) / ((2.0 * q - k + 1.0) * k);

		/* X = AX */
		MultMat( a, x, x );

		/* N = N + cX */
		MultScalar( x, c, cX );
		AddMat( n, cX, n );

		/* D = D + (-1)^k*cX */
		if( k % 2 )
		{
			MultScalar( cX, -1, cX );
		}
		AddMat( d, cX, d );
    }

	GaussianElimination( d, n, oResult );

	for( k = 0; k < j; ++k )
	{
		MultMat( oResult, oResult, oResult );
	}

	for( i = 0; i < iM.m_AllocatedSize; ++i )
    {
        oResult.m_Data[i] = fabs( oResult.m_Data[i] );
    }
}

//Scalar multiplication:
void MultScalar( SquareMatrix &iM, tMatrixDataTye iScalar, SquareMatrix &oResult )
{
    //Preconditions:
    assert( iM.GetSize() == oResult.GetSize() );

    unsigned i;
    tMatrixDataTye *s, *d;
    for( i = 0, s = iM.m_Data, d = oResult.m_Data; i < iM.m_AllocatedSize; ++i, ++s, ++d )
    {
        *d = *s * iScalar;
    }
}

//Matrix addition:
void AddMat( SquareMatrix &iM1, SquareMatrix &iM2, SquareMatrix &oResult )
{
    //Preconditions:
    assert( iM1.GetSize() == oResult.GetSize() && iM2.GetSize() == oResult.GetSize() );

    for( unsigned i = 0; i < iM1.m_AllocatedSize; ++i )
    {
        oResult.m_Data[i] = iM1.m_Data[i] + iM2.m_Data[i];
    }
}

//Matrix multiplication:
void MultMat( SquareMatrix &iM1, SquareMatrix &iM2, SquareMatrix &oResult )
{
    //Preconditions:
    assert( iM1.GetSize() == oResult.GetSize() && iM2.GetSize() == oResult.GetSize() );

	unsigned i, j, k, l;
	SquareMatrix temp(iM1.m_Size);

    tMatrixDataTye *m0 = temp.m_Data;
    tMatrixDataTye *m1 = iM1.m_Data;
	for( i = 0; i < iM1.m_Size; ++i, ++m1, ++m0 )
    {
		//for( j = 0; j < iM1.GetSize(); ++j )
		tMatrixDataTye *m2 = iM2.m_Data;
		for( j = 0; j < iM1.m_AllocatedSize; j+=iM1.m_Size, m2+=iM1.m_Size )
        {
			tMatrixDataTye aux = 0.0;
			//tMatrixDataTye *m2 = iM2.m_Data + j;
			for( k = 0, l = 0; k < iM1.GetSize(); ++k, l+=iM1.m_Size )
            {
				//aux += iM1.GetCell(i,k) * iM2.GetCell(k,j);
				aux += *(m1+l) * *(m2+k);
			}
			//*temp.GetCellAddress(i,j) = aux;
			*(m0 + j) = aux;
        }
    }
	oResult = temp;
}

//Rise a matrix to an integer (positivge) power:
void PowMat( SquareMatrix &iM, unsigned iPower, SquareMatrix &oResult )
{
    /*//Do it the brut way for now and optimize later (if necessary):
    oResult.Identity();
    for( unsigned i = 0; i < iPower; ++i )
    {
        MultMat( oResult, iM, oResult );
    }*/

    //Optimization based on: a^(2k) = (a^2)^k and a^(2k+1) = a*a^(2k)
    if( iPower == 0 )
    {
        oResult.Identity();
    }
    else if( iPower == 1 )
    {
        oResult = iM;
    }
    else if( iPower == 2 )
    {
        MultMat( iM, iM, oResult );
    }
    else if( iPower % 2 == 0 )
    {
        SquareMatrix a2(iM.GetSize());
        MultMat( iM, iM, a2 );
        PowMat( a2, iPower/2, oResult );
    }
    else
    {
        SquareMatrix a2(iM.GetSize());
        MultMat( iM, iM, a2 );
        PowMat( a2, iPower/2, oResult );
        MultMat( oResult, iM, oResult );
    }
}

//Gausssian elimination:
void GaussianElimination( SquareMatrix &iM1, SquareMatrix &iM2, SquareMatrix &oResult )
{
    //Based on MrBayes' GaussianElimination() functrion:

    //Preconditions:
    assert( iM1.GetSize() == oResult.GetSize() && iM2.GetSize() == oResult.GetSize() );

	/*tMatrixDataTye *bVec = new tMatrixDataTye[iM1.GetSize()];
	if( !bVec )
	{
	    cerr << "Not enough memory!" << endl;
	    assert( false );
	    exit( 1 );
	}*/
	tMatrixDataTye bVec[MAX_SQUARE_MATRIX_SIZE];

	SquareMatrix lMat(iM1.GetSize()), uMat(iM1.GetSize());

	ComputeLandU( iM1, lMat, uMat );

	for( unsigned k = 0; k < iM1.GetSize(); ++k )
    {
		/*for( i = 0; i < iM1.GetSize(); ++i )
		{
			bVec[i] = iM2.GetCell(i,k);
		}*/
		memcpy( bVec, iM2.m_Data+k*iM2.m_Size, iM2.m_Size*sizeof(tMatrixDataTye) );

		/* Answer of Ly = b (which is solving for y) is copied into b. */
		ForwardSubstitutionRow( lMat, bVec );

		/* Answer of Ux = y (solving for x and the y was copied into b above) is also copied into b. */
		BackSubstitutionRow( uMat, bVec );

		/*for( i = 0; i < iM1.GetSize(); ++i )
		{
			oResult.SetCell(i,k,bVec[i]);
		}*/
		memcpy( oResult.m_Data+k*iM2.m_Size, bVec, iM1.m_Size*sizeof(tMatrixDataTye) );
    }

	/*delete[] bVec*/;
}

//L & U decomposition:
void ComputeLandU( SquareMatrix &iM, SquareMatrix &oL, SquareMatrix &oU )
{
    //Based on MrNayes' ComputeLandU() function: return oL and oU such that iM = oL * oU:

    //Preconditions:
    assert( iM.GetSize() == oL.GetSize() && iM.GetSize() == oU.GetSize() );

	unsigned i, j, k, m, row, col;
	unsigned auxj;

	for( j = 0; j < iM.GetSize(); ++j )
    {
        auxj = iM.m_Size*j;
		for( k = 0; k < j; ++k )
		{
			for( i = k+1; i < j; ++i )
			{
				//iM.SetCell( i, j, iM.GetCell(i,j) - iM.GetCell(i,k) * iM.GetCell(k,j));
				*(iM.m_Data + i + auxj) -= *(iM.m_Data + i + iM.m_Size*k) * *(iM.m_Data + k + auxj);
			}
		}

		for( k = 0; k < j; ++k )
		{
			for( i = j; i < iM.GetSize(); ++i )
			{
				//iM.SetCell( i, j, iM.GetCell(i,j) - iM.GetCell(i,k) * iM.GetCell(k,j));
				*(iM.m_Data + i + auxj) -= *(iM.m_Data + i + iM.m_Size*k) * *(iM.m_Data + k + auxj);
			}
		}

		for( m = j+1; m < iM.GetSize(); ++m )
		{
  		    //iM.SetCell(m,j, iM.GetCell(m,j) / iM.GetCell(j,j));
  		    *(iM.m_Data + m + auxj) /= *(iM.m_Data + j + auxj);
		}
    }

	for( row = 0; row < iM.GetSize(); ++row )
    {
		for( col = 0; col < iM.GetSize(); ++col )
        {
			if( row <= col )
            {
				oU.SetCell( row, col, iM.GetCell( row, col ) );
				oL.SetCell( row, col, row == col ? 1.0 : 0.0 );
			}
			else
            {
				oL.SetCell( row, col, iM.GetCell( row, col ) );
				oU.SetCell( row, col, 0.0 );
            }
        }
    }
}

void ForwardSubstitutionRow( SquareMatrix &ioM, tMatrixDataTye *ioRow )
{
	unsigned  i, j;
	tMatrixDataTye dotProduct;

	ioRow[0] = ioRow[0] / ioM.GetCell(0,0);
	for( i = 1; i < ioM.GetSize(); ++i )
    {
		dotProduct = 0.0;
		for( j = 0; j < i; ++j )
		{
      		dotProduct += ioM.GetCell(i,j) * ioRow[j];
		}
		ioRow[i] = (ioRow[i] - dotProduct) / ioM.GetCell(i,i);
	}
}

void BackSubstitutionRow( SquareMatrix &ioM, tMatrixDataTye *ioRow )
{
	int  i, j;
	tMatrixDataTye dotProduct;

	ioRow[ioM.GetSize()-1] /= ioM.GetCell( ioM.GetSize()-1, ioM.GetSize()-1 );
	for( i = ioM.GetSize()-2; i >= 0; --i )
    {
		dotProduct = 0.0;
		for( j = i+1; j < (int)ioM.GetSize(); ++j )
		{
			dotProduct += ioM.GetCell(i,j) * ioRow[j];
		}
		ioRow[i] = (ioRow[i] - dotProduct) / ioM.GetCell(i,i);
	}
}


/****************************************************************************
*
*                  Likelihood computation for branches
*
****************************************************************************/

//Likelihood computation:
DblType SquareMatrix::ComputeLikelihood( int iAncestorValue, int iDescendantValue, DblType iBranchLength )
{
    return 1.0;
}



/****************************************************************************
*
*                       Boolean matrices (only 0.0 and 1.0)
*
****************************************************************************/

bool SquareMatrix::IsBoolean( void )
{
    for( unsigned i = 0; i < m_AllocatedSize; ++i )
    {
        if( m_Data[i] != 0.0 && m_Data[i] != 1.0 )
        {
            return false;
        }
    }

    return true;
}

void SquareMatrix::MakeBoolean( void )
{
    for( unsigned i = 0; i < m_AllocatedSize; ++i )
    {
        m_Data[i] = (m_Data[i] != 0.0) ? 1.0 : 0.0;
    }
}



/****************************************************************************
*
*                   Shortest path in the associated graph
*
****************************************************************************/

//The Floyd-Warshall shortest path algorithm:
void ShortestPath_FloydWarshall( SquareMatrix &iM, SquareMatrix &oResult )
{
    assert( iM.IsBoolean() );

    /*oResult = iM;

    for( unsigned k = 0; k < iM.GetSize(); ++k )
    {
        for( unsigned i = 0; i < iM.GetSize(); ++i )
        {
            for( unsigned j = 0; j < iM.GetSize(); ++j )
            {
                oResult.SetCell(i,j, (bool)oResult.GetCell(i,j) || ((bool)oResult.GetCell(i,k) && (bool)oResult.GetCell(k,j) ) );
            }
        }
    }*/

    unsigned i;

    oResult.Init( iM.GetSize() );

    #define INFINITY_CELL 100000

    //Transform the boolean adjacency matrix in a distance matrix:
    for( i = 0; i < iM.m_AllocatedSize; ++i )
    {
        oResult.m_Data[i] = (iM.m_Data[i] == 0.0) ? INFINITY_CELL : 1.0;
    }

    //Do the algorithm
    for( unsigned k = 0; k < iM.GetSize(); ++k )
    {
        for( unsigned i = 0; i < iM.GetSize(); ++i )
        {
            for( unsigned j = 0; j < iM.GetSize(); ++j )
            {
                oResult.SetCell(i,j, min( oResult.GetCell(i,j), oResult.GetCell(i,k) + oResult.GetCell(k,j) ) );
            }
        }
    }

    //Transform it back:
    for( i = 0; i < oResult.m_AllocatedSize; ++i )
    {
        oResult.m_Data[i] = (oResult.m_Data[i] >= INFINITY_CELL) ? 0.0 : oResult.m_Data[i];
    }
}


