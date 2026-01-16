//=============================================================================
//
// Copyright(c) 2022 Mirametrix Inc. All rights reserved.
//
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by MMX and
// are protected by copyright law. They may not be disclosed
// to third parties or copied or duplicated in any form, in whole or
// in part, without the prior written consent of MMX.
//
//=============================================================================

//=============================================================================
// I N C L U D E   F I L E S   A N D   F O R W A R D   D E C L A R A T I O N S

#include "matrix.h"

#include "app_assert.h"
#include "debug.h"
#include "errors.h"

//=============================================================================
// F U N C T I O N S   C O D E   S E C T I O N

//-----------------------------------------------------------------------------
//
void MatMul(
    const fp_mat_t op1,
    const fp_mat_t op2,
    fp_mat_t res )
{
    assert( op1.data != res.data && op2.data != res.data, EC_INVALID_MATRIX,
        "MatMul: The result matrix can't be one of the operands: op1 == %p, op2 == %p, res == %p\r\n",
        op1.data, op2.data, res.data );
    assert( op1.rows == res.rows,
        EC_MATRICES_DIM_DONT_MATCH,
        "MatMul: Operand 1 and result rows don't match: %d and %d\r\n",
        op1.rows, res.rows );
    assert( op2.cols == res.cols,
        EC_MATRICES_DIM_DONT_MATCH,
        "MatMul: Operand 2 and result cols don't match: %d and %d\r\n",
        op2.cols, res.cols );
    assert( op1.cols == op2.rows,
        EC_MATRICES_DIM_DONT_MATCH,
        "MatMul: Operand 1 cols and operand 2 rows don't match: %d and %d\r\n",
        op1.rows, op2.cols );
    assert( op1.fracBits == res.fracBits,
        EC_MATRIX_FRACBITS_DONT_MATCH,
        "MatMul: Result and op1 matrices number of fractional bits are different: %d != %d\r\n",
        op1.fracBits, res.fracBits );
    assert( op1.rowsFilter == 0 && op1.colsFilter == 0,
        EC_INVALID_MATRIX,
        "MatMul: Operand 1 rows/cols are filtered. Mul not supported\r\n",
        op1.fracBits, res.fracBits );
    assert( op2.rowsFilter == 0 && op2.colsFilter == 0,
        EC_INVALID_MATRIX,
        "MatMul: Operand 2 rows/cols are filtered. Mul not supported\r\n",
        op1.fracBits, res.fracBits );

    for( size_t i = 0; i < op1.rows; ++i )
    {
        size_t iRowPosOp1 = i * op1.rowStride;   
        size_t iRowPosRes = i * res.rowStride;
        
        for( size_t j = 0; j < op2.cols; ++j )
        {
            fp_t acc = CreateFPInt( 0, op1.fracBits );
            size_t jColPosOp2 = j * op2.colStride;

            for( size_t k = 0; k < op1.cols; ++k )
            {
                fp_t a = InterpretIntAsFP( op1.data[iRowPosOp1 + k * op1.colStride], op1.fracBits );
				fp_t b = InterpretIntAsFP( op2.data[k * op2.rowStride + jColPosOp2], op2.fracBits );
                acc = FPAdd( acc, FPMul( a, b ) );
            }
            res.data[iRowPosRes + j * res.colStride] = acc.n;
        }
    }
}

//-----------------------------------------------------------------------------
//
bool MatrixNorm( const fp_mat_t mat, fp_t *result )
{
	assert( result != NULL, EC_VALUE_NULL,
	 	"MatrixNorm: result is NULL, we cannot provide a result\r\n" );

	fp_t zero = CreateFPInt( 0, mat.fracBits );
    *result = CreateFPInt( 0, mat.fracBits );
    for( size_t i = 0; i < mat.rows; ++i )
    {
        for( size_t j = 0; j < mat.cols; ++j )
        {
            *result = FPAdd( *result, FPSqr( MatGet( mat, i, j ) ) );
        }
    }

	if( FPGe( *result, zero ) )
	{
		*result = FPSqrt( *result );
		return true;
    }
    DebugOutput( "MatrixNorm: result before square root is negative,"
    	"presumably we have overflowed. Failing.\r\n" );
    return false;
}

//-----------------------------------------------------------------------------
//
void ScaleMat(
    const fp_mat_t mat,
    const fp_t scale,
    fp_mat_t result )
{
    assert( mat.rows == result.rows,
        EC_MATRICES_DIM_DONT_MATCH,
        "ScaleMat: Matrices rows don't match: %d and %d\r\n",
        mat.rows, result.rows );
    assert( mat.cols == result.cols,
        EC_MATRICES_DIM_DONT_MATCH,
        "ScaleMat: Matrices cols don't match: %d and %d\r\n",
        mat.cols, result.cols );
    assert( mat.fracBits == result.fracBits,
        EC_MATRIX_FRACBITS_DONT_MATCH,
        "ScaleMat: Input and output matrices number of fractional bits are different: %d != %d\r\n",
        mat.fracBits, result.fracBits );

    for( size_t i = 0; i < mat.rows; ++i )
    {
        for( size_t j = 0; j < mat.cols; ++j )
        {
            MatSet( result, i, j, FPMul( MatGet( mat, i, j ), scale ) );
        }
    }
}

//-----------------------------------------------------------------------------
//
void InvScaleMat(
    const fp_mat_t mat,
    const fp_t scale,
    fp_mat_t result )
{
    assert( mat.rows == result.rows,
        EC_MATRICES_DIM_DONT_MATCH,
        "ScaleMat: Matrices rows don't match: %d and %d\r\n",
        mat.rows, result.rows );
    assert( mat.cols == result.cols,
        EC_MATRICES_DIM_DONT_MATCH,
        "ScaleMat: Matrices cols don't match: %d and %d\r\n",
        mat.cols, result.cols );
    assert( mat.fracBits == result.fracBits,
        EC_MATRIX_FRACBITS_DONT_MATCH,
        "MatMul: Input and output matrices number of fractional bits are different: %d != %d\r\n",
        mat.fracBits, result.fracBits );

    fp_t zero = CreateFPInt( 0, mat.fracBits );
    if( FPEq( scale, zero ) )
    {
        DebugOutput( "InvScaleMat: scale is zero!!\r\n" );
    }

    for( size_t i = 0; i < mat.rows; ++i )
    {
        for( size_t j = 0; j < mat.cols; ++j )
        {
            MatSet( result, i, j, FPDiv( MatGet( mat, i, j ), scale ) );
        }
    }
}

//-----------------------------------------------------------------------------
//
void MatAdd(
    const fp_mat_t op1,
    const fp_mat_t op2,
    fp_mat_t res )
{
    assert( op1.rows == op2.rows && op2.rows == res.rows,
        EC_MATRICES_DIM_DONT_MATCH,
        "MatAdd: Matrices rows don't match: %d, %d and %d\r\n",
        op1.rows, op2.rows, res.rows );
    assert( op1.cols == op2.cols && op2.cols == res.cols,
        EC_MATRICES_DIM_DONT_MATCH,
        "MatAdd: Matrices cols don't match: %d, %d and %d\r\n",
        op1.cols, op2.cols, res.cols );
    assert( op1.fracBits == op2.fracBits && op2.fracBits == res.fracBits,
        EC_MATRIX_FRACBITS_DONT_MATCH,
        "MatAdd: Matrices don't have the same number of fractional bits: %d, %d, %d\r\n",
        op1.fracBits, op2.fracBits, res.fracBits );

    for( size_t i = 0; i < res.rows; ++i )
    {
        for( size_t j = 0; j < res.cols; ++j )
        {
            fp_t a = MatGet( op1, i, j );
            fp_t b = MatGet( op2, i, j );
            MatSet( res, i, j, FPAdd( a, b ) );
        }
    }
}

//-----------------------------------------------------------------------------
//
void MatAddMax0(
    const fp_mat_t op1,
    const fp_mat_t op2,
    fp_mat_t res )
{
    assert( op1.rows == op2.rows && op2.rows == res.rows,
        EC_MATRICES_DIM_DONT_MATCH,
        "MatAdd: Matrices rows don't match: %d, %d and %d\r\n",
        op1.rows, op2.rows, res.rows );
    assert( op1.cols == op2.cols && op2.cols == res.cols,
        EC_MATRICES_DIM_DONT_MATCH,
        "MatAdd: Matrices cols don't match: %d, %d and %d\r\n",
        op1.cols, op2.cols, res.cols );
    assert( op1.fracBits == op2.fracBits && op2.fracBits == res.fracBits,
        EC_MATRIX_FRACBITS_DONT_MATCH,
        "MatAdd: Matrices don't have the same number of fractional bits: %d, %d, %d\r\n",
        op1.fracBits, op2.fracBits, res.fracBits );

    for( size_t i = 0; i < res.rows; ++i )
    {
        for( size_t j = 0; j < res.cols; ++j )
        {
            fp_t a = MatGet( op1, i, j );
            fp_t b = MatGet( op2, i, j );
            MatSet( res, i, j, FPAddMax0( a, b ));
        }
    }
}

//-----------------------------------------------------------------------------
//
void MatSub(
    const fp_mat_t op1,
    const fp_mat_t op2,
    fp_mat_t res )
{
    assert( op1.rows == op2.rows && op2.rows == res.rows,
        EC_MATRICES_DIM_DONT_MATCH,
        "MatSub: Matrices rows don't match: %d, %d and %d\r\n",
        op1.rows, op2.rows, res.rows );
    assert( op1.cols == op2.cols && op2.cols == res.cols,
        EC_MATRICES_DIM_DONT_MATCH,
        "MatSub: Matrices cols don't match: %d, %d and %d\r\n",
        op1.cols, op2.cols, res.cols );
    assert( op1.fracBits == op2.fracBits && op2.fracBits == res.fracBits,
        EC_MATRIX_FRACBITS_DONT_MATCH,
        "MatSub: Matrices don't have the same number of fractional bits: %d, %d, %d\r\n",
        op1.fracBits, op2.fracBits, res.fracBits );

    for( size_t i = 0; i < res.rows; ++i )
    {
        for( size_t j = 0; j < res.cols; ++j )
        {
            fp_t a = MatGet( op1, i, j );
            fp_t b = MatGet( op2, i, j );
            MatSet( res, i, j, FPSub( a, b ) );
        }
    }
}

//-----------------------------------------------------------------------------
//
void MatDiv(
    const fp_mat_t op1,
    const fp_mat_t op2,
    fp_mat_t res )
{
    assert( op1.rows == op2.rows && op2.rows == res.rows,
        EC_MATRICES_DIM_DONT_MATCH,
        "MatDiv: Matrices rows don't match: %d, %d and %d\r\n",
        op1.rows, op2.rows, res.rows );
    assert( op1.cols == op2.cols && op2.cols == res.cols,
        EC_MATRICES_DIM_DONT_MATCH,
        "MatDiv: Matrices cols don't match: %d, %d and %d\r\n",
        op1.cols, op2.cols, res.cols );
    assert( op1.fracBits == op2.fracBits && op2.fracBits == res.fracBits,
        EC_MATRIX_FRACBITS_DONT_MATCH,
        "MatDiv: Matrices don't have the same number of fractional bits: %d, %d, %d\r\n",
        op1.fracBits, op2.fracBits, res.fracBits );

	fp_t zero = CreateFPInt( 0, op1.fracBits );

    for( size_t i = 0; i < res.rows; ++i )
    {
        for( size_t j = 0; j < res.cols; ++j )
        {
            fp_t a = MatGet( op1, i, j );
            fp_t b = MatGet( op2, i, j );

			if( FPEq( b, zero ) )
			{
				DebugOutput( "MatDiv: op2[%d,%d] is zero, cannot continue with MatDiv!\r\n",
					i, j );
			}
            MatSet( res, i, j, FPDiv( a, b ) );
        }
    }
}

//-----------------------------------------------------------------------------
//
void MatCrossProduct(
    const fp_mat_t op1,
    const fp_mat_t op2,
    fp_mat_t res )
{
    assert( op1.rows != 1 || op1.rows != 3 || 
            op1.cols != 1 || op1.cols != 3, EC_MATRICES_DIM_DONT_MATCH,
        "OneMatrixCrossProduct: a is a %dx%d matrix. 3x1 or 1x3 is expected",
        op1.rows, op1.cols );
    assert( op2.rows != 1 || op2.rows != 3 || 
            op2.cols != 1 || op2.cols != 3, EC_MATRICES_DIM_DONT_MATCH,
        "OneMatrixCrossProduct: b is a %dx%d matrix. 3x1 or 1x3 is expected",
        op2.rows, op2.cols );
    assert( op2.rows != 1 || op2.rows != 3, EC_MATRICES_DIM_DONT_MATCH,
        "OneMatrixCrossProduct: a is a %dx%d matrix. 3x1 or 1x3 is expected",
        op1.rows );
    assert( op1.rows == op2.rows, EC_MATRICES_DIM_DONT_MATCH,
        "OneMatrixCrossProduct: Matrices rows don't match: %d and %d\r\n",
        op1.rows, op2.rows );
    assert( op2.rows == res.rows, EC_MATRICES_DIM_DONT_MATCH,
        "OneMatrixCrossProduct: Matrices rows don't match: %d and %d\r\n",
        op2.rows, res.rows );
    assert( op1.cols == op2.cols, EC_MATRICES_DIM_DONT_MATCH,
        "OneMatrixCrossProduct: Matrices cols don't match: %d and %d\r\n",
        op1.cols, op2.cols );
    assert( op2.cols == res.cols, EC_MATRICES_DIM_DONT_MATCH,
        "OneMatrixCrossProduct: Matrices cols don't match: %d and %d\r\n",
        op1.cols, op2.cols );
    assert( op1.data != res.data && op2.data != res.data, EC_INVALID_MATRIX,
        "OneMatrixCrossProduct: The result matrix can't be one of the operands: a == %p, b == %p, res == %p\r\n",
        op1.data, op2.data, res.data );
    
    if( op1.cols == 3 )
    {
        fp_mat_t op1T = TransposeMat( op1 );
        fp_mat_t op2T = TransposeMat( op2 );
        fp_mat_t resT = TransposeMat( res );
        MatCrossProduct( op1T, op2T, resT );
        return;
    }
    
    fp_t a1 = MatGet( op1, 0, 0 );
    fp_t a2 = MatGet( op1, 1, 0 );
    fp_t a3 = MatGet( op1, 2, 0 );
    
    fp_t b1 = MatGet( op2, 0, 0 );
    fp_t b2 = MatGet( op2, 1, 0 );
    fp_t b3 = MatGet( op2, 2, 0 );
    
    MatSet( res, 0, 0, FPSub( FPMul( a2, b3 ), FPMul( a3, b2 ) ) );
    MatSet( res, 1, 0, FPSub( FPMul( a3, b1 ), FPMul( a1, b3 ) ) );
    MatSet( res, 2, 0, FPSub( FPMul( a1, b2 ), FPMul( a2, b1 ) ) );
}

//-----------------------------------------------------------------------------
//
void MatAbs(
    const fp_mat_t op,
    fp_mat_t res )
{
    assert( op.rows == res.rows, EC_MATRICES_DIM_DONT_MATCH,
        "MatAbs: Matrices rows don't match: %d and %d\r\n",
        op.rows, res.rows );
    assert( op.cols == res.cols, EC_MATRICES_DIM_DONT_MATCH,
        "MatAbs: Matrices cols don't match: %d and %d\r\n",
        op.cols, res.cols );

    for( size_t i = 0; i < res.rows; ++i )
    {
        for( size_t j = 0; j < res.cols; ++j )
        {
            fp_t a = MatGet( op, i, j );
            MatSet( res, i, j, FPAbs( a ) );
        }
    }
}

//-----------------------------------------------------------------------------
//
void MatColMean(
    const fp_mat_t op,
    fp_mat_t res )
{
    assert( res.rows == 1,
        EC_MATRICES_DIM_DONT_MATCH,
        "MatColMean: res matrix should have 1 row : %d\r\n",
        res.rows );
    assert( res.cols == op.cols,
        EC_MATRICES_DIM_DONT_MATCH,
        "MatColMean: res matrix should have %d columns: %d\r\n",
        op.cols, res.cols );

    for( size_t j = 0; j < op.cols; ++j )
    {
        fp_t acc = MatGet( op, 0, j );
        for( size_t i = 1; i < op.rows; ++i )
        {
            acc = FPAdd( acc, MatGet( op, i, j ) );
        }
        acc.n /= op.rows;
        MatSet( res, 0, j, acc );
    }
}

//-----------------------------------------------------------------------------
//
void MatRowMean(
    const fp_mat_t op,
    fp_mat_t res )
{
    assert( res.rows == op.rows,
        EC_MATRICES_DIM_DONT_MATCH,
        "MatRowMean: res matrix should have %d rows : %d\r\n",
        op.rows, res.rows );
    assert( res.cols == 1,
        EC_MATRICES_DIM_DONT_MATCH,
        "MatRowMean: res matrix should have 1 column: %d\r\n",
        res.cols );

    MatColMean( TransposeMat( op ), TransposeMat( res ) );
}

//-----------------------------------------------------------------------------
//
void InitMatDiag(
    fp_mat_t mat,
    const fp_t value )
{
    assert( mat.cols == mat.rows,
        EC_MATRICES_DIM_DONT_MATCH,
        "InitMatDiag: Matrix is not square: %dx%d\r\n",
        mat.rows, mat.cols );
    assert( mat.fracBits == value.fracBits,
        EC_MATRIX_FRACBITS_DONT_MATCH,
        "InitMatDiag: Matrix and diagonal value number of fractional bits are different: %d != %d\r\n",
        mat.fracBits, value.fracBits );

    for( size_t i = 0; i < mat.rows; ++i )
    {
        for( size_t j = 0; j < mat.cols; ++j )
        {
            if( i != j )
            {
                MatSet( mat, i, j, CreateFPInt( 0, mat.fracBits ) );
            }
            else
            {
                MatSet( mat, i, j, value );
            }
        }
    }
}

//-----------------------------------------------------------------------------
//
fp_t MatTrace( const fp_mat_t mat )
{
    assert( mat.cols == mat.rows,
        EC_MATRICES_DIM_DONT_MATCH,
        "MatTrace: Input matrix isn't square: %dx%d\r\n",
        mat.rows, mat.cols );

    fp_t result = MatGet( mat, 0, 0 );
    for( size_t i = 1; i < mat.rows; ++i )
    {
        result = FPAdd( result, MatGet( mat, i, i ) );
    }
    return result;
}

//-----------------------------------------------------------------------------
//
fp_mat_t TransposeMat(
    const fp_mat_t mat )
{
    fp_mat_t result = {
        .rows = mat.cols,
        .cols = mat.rows,
        .colStride = mat.rowStride,
        .rowStride = mat.colStride,
        .fracBits = mat.fracBits,
        .data = mat.data,
        .rowsFilter = mat.colsFilter,
        .colsFilter = mat.rowsFilter,
        .recyclable = false
    };
    return result;
}

//-----------------------------------------------------------------------------
// This method uses Gauss-Jordan elimintation as its implementation:
// (see https://integratedmlai.com/matrixinverse/)
bool InvertSquareMatrix6x6(
    const fp_mat_t mat,
    fp_mat_t inv )
{
    assert( mat.data != inv.data, EC_INVALID_MATRIX,
        "InvertSquareMatrix6x6: The result matrix can't be the same as the input matrix: mat == %p, inv == %p\r\n",
        mat, inv );
    assert( mat.cols == mat.rows && mat.cols == 6,
        EC_MATRICES_DIM_DONT_MATCH,
        "InvertSquareMatrix6x6: Matrix is not square and 6x6!: %dx%d\r\n",
        mat.rows, mat.cols );

    CreateFPMat( A, 6, 6, mat.fracBits );
    MatDeepCopy( mat, A );

	fp_t zero = CreateFPInt( 0, A.fracBits );
    fp_t one = CreateFPInt( 1, A.fracBits );
    InitMatDiag( inv, one );

    for( size_t fd = 0; fd < A.rows; ++fd )
    {
        fp_t pivotVal = MatGet( A, fd, fd );
        if( FPEq( pivotVal, zero ) )
        {
            DebugOutput( "InvertSquareMatrix6x6: for iteration %d"
                " A[%d, %d] is zero, cannot continue!\r\n", fd, fd );
            return false;
        }

		fp_t fdScaler = FPDiv( one, pivotVal );

        for( size_t j = 0; j < A.cols; ++j )
        {
            MatSet( A, fd, j, FPMul( MatGet( A, fd, j ), fdScaler ) );
            MatSet( inv, fd, j, FPMul( MatGet( inv, fd, j ), fdScaler ) );
        }
        for( size_t i = 0; i < A.rows; ++i )
        {
            if( i != fd )
            {
                fp_t crScaler = MatGet( A, i, fd );
                for( size_t j = 0; j < A.cols; ++j )
                {
                    MatSet( A, i, j, FPSub(
                        MatGet( A, i, j ),
                        FPMul( crScaler, MatGet( A, fd, j ) ) ) );

                    MatSet( inv, i, j, FPSub(
                        MatGet( inv, i, j ),
                        FPMul( crScaler, MatGet( inv, fd, j ) ) ) );
                }
            }
        }
    }
    return true;
}

//-----------------------------------------------------------------------------
//
fp_mat_t FilterRows( fp_mat_t mat, const uint8_t *indices, size_t indicesLen )
{
    assert( mat.rowsFilter == 0,
        EC_MATRIX_ALREADY_FILTERED,
        "FilterRows: rows already filtered\r\n" );
    assert( 0 < indicesLen && indicesLen <= mat.rows,
        EC_MATRIX_BAD_FILTER_LENGTH,
        "FilterRows: bad filter length: %d, should be between %d and %d \r\n",
        indicesLen, 1, mat.rows );

    // If we're trying to filter rows from a transposed matrix
    if( mat.colStride != 1 )
    {
        return TransposeMat( 
            FilterCols( TransposeMat( mat ), indices, indicesLen ) );
    }
    else
    {
        fp_mat_t filteredMat = {
            .rows = indicesLen,
            .cols = mat.cols,
            .rowStride = mat.rowStride,
            .colStride = mat.colStride,
            .fracBits = mat.fracBits,
            .data = mat.data,
            .rowsFilter = indices,
            .colsFilter = mat.colsFilter,
            .recyclable = false
        };
        return filteredMat;
    }
}
//-----------------------------------------------------------------------------
//
fp_mat_t FilterCols( fp_mat_t mat, const uint8_t *indices, size_t indicesLen )
{
    assert( mat.colsFilter == 0,
        EC_MATRIX_ALREADY_FILTERED,
        "FilterCols: columns already filtered\r\n" );
    assert( 0 < indicesLen && indicesLen <= mat.cols,
        EC_MATRIX_BAD_FILTER_LENGTH,
        "FilterCols: bad filter length: %d, should be between %d and %d \r\n",
        indicesLen, 1, mat.cols );

    // If we're trying to filter columns from a transposed matrix
    if( mat.colStride != 1 )
    {
        return TransposeMat(
            FilterRows( TransposeMat( mat ), indices, indicesLen ) );
    }
    else
    {
        fp_mat_t filteredMat = {
            .rows = mat.rows,
            .cols = indicesLen,
            .rowStride = mat.rowStride,
            .colStride = mat.colStride,
            .fracBits = mat.fracBits,
            .data = mat.data,
            .rowsFilter = mat.rowsFilter,
            .colsFilter = indices,
            .recyclable = false
        };
        return filteredMat;
    }
}

//-----------------------------------------------------------------------------
//
fp_mat_t ReshapeMat( fp_mat_t mat, size_t rows, size_t cols )
{
    assert( mat.rowsFilter == 0,
        EC_MATRIX_ALREADY_FILTERED,
        "ReshapeMat: Can't reshape a row-filtered matrix\r\n" );
    assert( mat.colsFilter == 0,
        EC_MATRIX_ALREADY_FILTERED,
        "ReshapeMat: Can't reshape a column-filtered matrix\r\n" );
    assert( mat.colStride == 1,
        EC_INVALID_MATRIX,
        "ReshapeMat: Can't reshape a transposed matrix\r\n" );
    assert( rows * cols == mat.rows * mat.cols,
        EC_MATRIX_BAD_RESHAPE_DIM,
        "ReshapeMat: Matrix size not compatible with new dimensions: %dx%d -> size %d, new dimensions %dx%d\r\n",
        mat.rows, mat.cols, mat.rows * mat.cols, rows, cols );

    fp_mat_t result = {
        .rows = rows,
        .cols = cols,
        .rowStride = cols,
        .colStride = 1,
        .fracBits = mat.fracBits,
        .data = mat.data,
        .rowsFilter = 0,
        .colsFilter = 0,
        .recyclable = mat.recyclable
    };
    return result;
}

//-----------------------------------------------------------------------------
// Performs QR decomposition using the modified Gram-Schmidt process
// (see http://web.mit.edu/18.06/www/Essays/gramschmidtmat.pdf and
// https://arnold.hosted.uark.edu/NLA/Pages/CGSMGS.pdf).
bool MakeMat3x3Orthogonal(
    const fp_mat_t mat,
    fp_mat_t Q )
{
    assert( mat.data != Q.data, EC_INVALID_MATRIX,
        "MakeMatOrthogonal: The result matrix can't be the same as the input "
        "matrix: mat == %p, Q == %p\r\n", mat, Q );

    CreateFPMat( R, 3, 3, mat.fracBits );

    fp_t zero = CreateFPInt( 0, mat.fracBits );
    for( size_t i = 0; i < R.rows; ++i )
    {
        for( size_t j = 0; j < R.cols; ++j )
        {
            MatSet( R, i, j, zero );
            MatSet( Q, i, j, zero );
        }
    }

    for( size_t j = 0; j < mat.cols; ++j )
    {
        CreateFPMat( v, 3, 1, mat.fracBits );

        for( size_t i = 0; i < mat.rows; ++i )
        {
            MatSet( v, i, 0, MatGet( mat, i, j ) );
        }

        for( size_t i = 0; i < j; ++i )
        {
            fp_t value = CreateFPInt( 0, Q.fracBits );

            for( size_t k = 0; k < Q.rows; ++k )
            {
                fp_t a = MatGet( Q, k, i );
                fp_t b = MatGet( mat, k, j );
                value = FPAdd( value, FPMul( a, b ) );
            }

            MatSet( R, i, j, value );

            for( size_t k = 0; k < v.rows; ++k )
            {
                fp_t b = MatGet( Q, k, i );
                fp_t c = MatGet( v, k, 0 );
                MatSet( v, k, 0, FPSub( c, FPMul( value, b ) ) );
            }
        }

		fp_t vNorm = zero;
		if( !MatrixNorm( v, &vNorm ) )
		{
			return false;
		}
        MatSet( R, j, j, vNorm );

        for( size_t i = 0; i < v.rows; ++i )
        {
            fp_t a = MatGet( v, i, 0 );
            fp_t b = MatGet( R, j, j );

            if( FPEq( b, zero ) )
            {
            	DebugOutput( "MakeMat3x3Orthogonal: for Q[%d, %d], R[%d, %d] is zero!!\r\n",
            		i, j, j, j );
				return false;
            }
            MatSet( Q, i, j, FPDiv( a, b ) );
        }
    }
    return true;
}

//-----------------------------------------------------------------------------
//
// Not general
void AddOrthogonalRowTo2DRotMat(
    const fp_mat_t rotMat2D,
    fp_mat_t result )
{
    assert( rotMat2D.cols == 3 && rotMat2D.rows == 2,
        EC_MATRICES_DIM_DONT_MATCH,
        "AddOrthogonalRowTo2DRotMat: input matrix is not 2x3: %dx%d\r\n",
        rotMat2D.rows, rotMat2D.cols );

    assert( result.cols == 3 && result.rows == 3,
        EC_MATRICES_DIM_DONT_MATCH,
        "AddOrthogonalRowTo2DRotMat: input matrix is not 3x3: %dx%d\r\n",
        result.rows, result.cols );

    MatSet( result, 2, 0, FPSub(
        FPMul( MatGet( rotMat2D, 0, 1 ), MatGet( rotMat2D, 1, 2 ) ),
        FPMul( MatGet( rotMat2D, 0, 2 ), MatGet( rotMat2D, 1, 1 ) )
    ) );

    MatSet( result, 2, 1, FPSub(
        FPMul( MatGet( rotMat2D, 0, 2 ), MatGet( rotMat2D, 1, 0 ) ),
        FPMul( MatGet( rotMat2D, 0, 0 ), MatGet( rotMat2D, 1, 2 ) )
    ) );
        
    MatSet( result, 2, 2, FPSub(
        FPMul( MatGet( rotMat2D, 0, 0 ), MatGet( rotMat2D, 1, 1 ) ),
        FPMul( MatGet( rotMat2D, 0, 1 ), MatGet( rotMat2D, 1, 0 ) )
    ) );

    MatSet( result, 0, 0, MatGet( rotMat2D, 0, 0 ) );
    MatSet( result, 0, 1, MatGet( rotMat2D, 0, 1 ) );
    MatSet( result, 0, 2, MatGet( rotMat2D, 0, 2 ) );
    MatSet( result, 1, 0, MatGet( rotMat2D, 1, 0 ) );
    MatSet( result, 1, 1, MatGet( rotMat2D, 1, 1 ) );
    MatSet( result, 1, 2, MatGet( rotMat2D, 1, 2 ) );
}

//-----------------------------------------------------------------------------
//
void MatDeepCopy(
    const fp_mat_t original,
    fp_mat_t copy )
{
    assert( original.rows == copy.rows,
        EC_MATRICES_DIM_DONT_MATCH,
        "MatDeepCopy: original matrix rows number different from copy matrix rows number: %d != %d\r\n",
        original.rows, copy.rows );
    assert( original.cols == copy.cols,
        EC_MATRICES_DIM_DONT_MATCH,
        "MatDeepCopy: original matrix cols number different from copy matrix cols number: %d != %d\r\n",
        original.cols, copy.cols );

    for( size_t i = 0; i < original.rows; ++i )
    {
        for( size_t j = 0; j < original.cols; ++j )
        {
            MatSet( copy, i, j, MatGet( original, i, j ) );
        }
    }
}

//-----------------------------------------------------------------------------
//
fp_mat_t RecycleMat( fp_mat_t mat, size_t newRows, size_t newCols )
{
    assert( mat.recyclable,
        EC_INVALID_MATRIX,
        "RecycleMat: matrix is not recyclable\r\n" );
    assert( newRows * newCols <= mat.rows * mat.cols,
        EC_MATRIX_BAD_RESHAPE_DIM,
        "RecycleMat: new matrix larger than the recycled one: original matrix %dx%d = %d, recycle matrix %dx%d = %d\r\n",
        mat.rows, mat.cols, mat.rows * mat.cols,
        newRows, newCols, newRows * newCols );
    fp_mat_t newMat =
    {
        .rows = newRows,
        .cols = newCols,
        .fracBits = mat.fracBits,
        .data = mat.data,
        .rowsFilter = 0,
        .colsFilter = 0,
        .rowStride = newCols,
        .colStride = 1,
        .recyclable = mat.recyclable
    };
    return newMat;
}

//-----------------------------------------------------------------------------
//
bool MatHasZeros(
    const fp_mat_t mat )
{
	fp_t zero = CreateFPInt( 0, mat.fracBits );

    for( size_t i = 0; i < mat.rows; ++i )
    {
        for( size_t j = 0; j < mat.cols; ++j )
        {
			if( FPEq( MatGet( mat, i, j ), zero ) )
			{
				return true;
			}
        }
    }
    return false;
}

//-----------------------------------------------------------------------------
//
fp_t MatCosineSimilarity( fp_mat_t a, fp_mat_t b )
{
    assert( a.rows == 1 || a.cols == 1, EC_INVALID_MATRIX,
        "MatCosineDistance: matrix a has shape %dx%d\r\n", a.rows, a.cols );
    assert( b.rows == 1 || b.cols == 1, EC_INVALID_MATRIX,
        "MatCosineDistance: matrix b has shape %dx%d\r\n", b.rows, b.cols );
    assert( a.rows == b.rows, EC_MATRICES_DIM_DONT_MATCH,
        "MatCosineDistance: matrices have differents shapes, %dx%d and %dx%d\r\n",
        a.rows, a.cols, b.rows, b.cols );
    assert( a.cols == b.cols, EC_MATRICES_DIM_DONT_MATCH,
        "MatCosineDistance: matrices have differents shapes, %dx%d and %dx%d\r\n",
        a.rows, a.cols, b.rows, b.cols );
    
    fp_t zero = CreateFPInt( 0, a.fracBits );
    fp_t normA;
    fp_t normB;
    MatrixNorm( a, &normA );
    MatrixNorm( b, &normB );
    
    assert( FPEq( normA, zero ), EC_VECTOR_NORM_IS_ZERO,
        "MatCosineSimilarity: vector a has norm 0\r\n" );
    assert( FPEq( normB, zero ), EC_VECTOR_NORM_IS_ZERO,
        "MatCosineSimilarity: vector b has norm 0\r\n" );
    
    CreateFPMat( distance, 1, 1, a.fracBits );
    if( a.rows == 1 )
    {
        MatMul( a, TransposeMat( b ), distance );
    }
    else
    {
        MatMul( TransposeMat( a ), b, distance );
    }
    
    if( FPEq( normA, zero ) || FPEq( normB, zero ) )
    {
        return zero;
    }
    return FPDiv( MatGet(distance, 0, 0), FPMul( normA, normB ) );
}

//-----------------------------------------------------------------------------
//
void DebugOutputMatrix( fp_mat_t mat )
{
    for( size_t i = 0; i < mat.rows; ++i )
    {
        for( size_t j = 0; j < mat.cols; ++j )
        {
            DebugOutput( "%d ", MatGet( mat, i, j ).n );
        }
        DebugOutput( "\r\n" );
    }
}

//-----------------------------------------------------------------------------
//
void DebugOutputFlatMatrix( fp_mat_t mat )
{
    for( size_t i = 0; i < mat.rows; ++i )
    {
        for( size_t j = 0; j < mat.cols; ++j )
        {
            DebugOutput( "%d ", MatGet( mat, i, j ).n );
        }
    }
    DebugOutput( "\r\n" );
}