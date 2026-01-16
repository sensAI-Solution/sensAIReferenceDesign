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

#ifndef MATRIX_H
#define MATRIX_H

#include <stdint.h>

#include "app_assert.h"
#include "errors.h"
#include "fixed_point.h"
#include "landmarks_config.h"
#include "types.h"

//=============================================================================
// S T R U C T S

// Represents a matrix of fixed point numbers.
// All the values in the matrix use the same fixed point representation.
typedef struct
{
    size_t rows; // number of rows
    size_t cols; // number of columns
    uint32_t *data; // address of the matrix data
    uint8_t fracBits; // number of fractional bits used for the matrix numbers

    int rowStride; // distance in memory between two elements on the same column 
                   // and two consecutive rows
    int colStride; // distance in memory between two elements on the same row
                   // and two consecutive columns
    
    // This allows to apparently filter out rows and columns from a matrix
    const uint8_t *rowsFilter; // filters to apply to the row index when 
                              // setting/getting an element
    const uint8_t *colsFilter; // filter to apply to the column index when 
                              // setting/getting an element

    bool recyclable; // can the matrix data be reused with another matrix ?
} fp_mat_t;

//=============================================================================
// M A C R O S   D E C L A R A T I O N S

// Define structs and constructor functions for differents size of matrices
#define FP_MAT( nrows, ncols ) \
    typedef struct \
    { \
        uint32_t data[nrows * ncols]; \
    } fp_mat_##nrows##x##ncols##_t; \
    \
    static inline fp_mat_##nrows##x##ncols##_t CreateFPMat##nrows##x##ncols( uint8_t fracBits ) \
    { \
        fp_mat_##nrows##x##ncols##_t mat = {}; \
        return mat; \
    }
// Helper macro to support defines as arguments
#define FP_MAT_HELPER( nrows, ncols ) FP_MAT( nrows, ncols )

// Creates a non recyclable fp_mat matrix
// This macro allocates memory on the stack to store the data by declaring 
// invisible a new variable of type fp_mat_##nrows##x##ncols##_t. 
// That's not ideal because:
// - the macro's user is not aware of this behavior
// - it could lead to problems if the hidden variable name collides with another
//   name. This is quite unlikely due to the name of the hidden variable though.
// Note this macro declares the new matrix for you, so you don't need to do it.
// For example, if you want to create a 3x3 matrix myNewMatrix filled with 10 
// fractional bits fixed point numbers, you DON'T write:
//   fp_mat_t myNewMatrix;
//   CreateFPMat(myNewMatrix, 3, 3, 10);
// instead, you only write:
//   CreateFPMat(myNewMatrix, 3, 3, 10);
#define CreateFPMat(name, nrows, ncols, nfracBits) \
    fp_mat_##nrows##x##ncols##_t create_fp_mat_##name = CreateFPMat##nrows##x##ncols( nfracBits ); \
    fp_mat_t name = { \
        .rows = nrows, \
        .cols = ncols, \
        .rowStride = ncols, \
        .colStride = 1, \
        .fracBits = nfracBits, \
        .data = create_fp_mat_##name.data, \
        .rowsFilter = 0, \
        .colsFilter = 0, \
        .recyclable = false \
    };

#define CreateLiteralFPMat(name, nrows, ncols, nfracBits, values) \
    fp_mat_##nrows##x##ncols##_t create_fp_mat_##name = { \
        .data = values \
    }; \
    fp_mat_t name = { \
        .rows = nrows, \
        .cols = ncols, \
        .rowStride = ncols, \
        .colStride = 1, \
        .fracBits = nfracBits, \
        .data = create_fp_mat_##name.data, \
        .rowsFilter = 0, \
        .colsFilter = 0, \
        .recyclable = false \
    };

#define CreateFPMatArray(name, nrows, ncols, nfracBits, size) \
    char data[size * sizeof( fp_mat_##nrows##x##ncols##_t )]; \
    fp_mat_t name[size]; \
    for ( size_t i = 0; i < size; ++i ) \
    { \
        name[i].rows = nrows; \
        name[i].cols = ncols; \
        name[i].rowStride = ncols; \
        name[i].colStride = 1; \
        name[i].fracBits = nfracBits; \
        name[i].data = (uint32_t *) &data[sizeof( fp_mat_##nrows##x##ncols##_t ) * i]; \
        name[i].rowsFilter = 0; \
        name[i].colsFilter = 0; \
        name[i].recyclable = false; \
    }

// Helper macro to support defines as arguments
#define CreateFPMat_Helper(name, nrows, ncols, nfracBits) CreateFPMat(name, nrows, ncols, nfracBits)

// Creates a non recyclable fp_mat matrix
// This macro allocates memory on the stack to store the data by declaring 
// invisible a new variable of type fp_mat_##nrows##x##ncols##_t. 
// That's not ideal because:
// - the macro's user is not aware of this behavior
// - it could lead to problems if the hidden variable name collides with another
//   name. This is quite unlikely due to the name of the hidden variable though
#define CreateRecyclableFPMat(name, nrows, ncols, nfracBits) \
    fp_mat_##nrows##x##ncols##_t create_fp_mat_##name = CreateFPMat##nrows##x##ncols( nfracBits ); \
    fp_mat_t name = { \
        .rows = nrows, \
        .cols = ncols, \
        .rowStride = ncols, \
        .colStride = 1, \
        .fracBits = nfracBits, \
        .data = create_fp_mat_##name.data, \
        .rowsFilter = 0, \
        .colsFilter = 0, \
        .recyclable = true \
    };
// Helper macro to support defines as arguments
#define CreateRecyclableFPMat_Helper(name, nrows, ncols, nfracBits) CreateRecyclableFPMat(name, nrows, ncols, nfracBits)

// Creating multiple types for multiple size of matrices
FP_MAT( 1, 1 );
FP_MAT( 1, 2 );
FP_MAT( 1, 3 );
FP_MAT( 1, 6 );
FP_MAT( 2, 1 );
FP_MAT( 2, 2 );
FP_MAT( 2, 3 );
FP_MAT( 2, 4 );
FP_MAT( 2, 5 );
FP_MAT( 3, 1 );
FP_MAT( 3, 3 );
FP_MAT( 3, 4 );
FP_MAT( 3, 5 );
FP_MAT_HELPER( 3, FITTER_LANDMARKS_NB );
FP_MAT( 5, 2 );
FP_MAT( 5, 3 );
FP_MAT( 5, 5 );
FP_MAT( 6, 1 );
FP_MAT( 6, 6 );
FP_MAT_HELPER( FITTER_LANDMARKS_NB, 3 );

FP_MAT( 1, FITTER_LANDMARKS_NB_MUL_BY_2 );
FP_MAT( 2, FITTER_LANDMARKS_NB );
FP_MAT( 3, FITTER_LANDMARKS_NB );
FP_MAT( FITTER_LANDMARKS_NB, 4 );
FP_MAT( FITTER_LANDMARKS_NB_MUL_BY_2, 6 );

FP_MAT( 16, 1 );
FP_MAT( 11, 1 );
FP_MAT( 11, 16 );
FP_MAT( 8, 11 );
FP_MAT( 6, 8 );
FP_MAT( 8, 1 );

//=============================================================================
// F U N C T I O N S   D E C L A R A T I O N S

// Get mat[row, col]
// it is assumed 0 <= row < mat.rows and 0 <= col < mat.cols.
static inline fp_t MatGet(
    const fp_mat_t mat,
    size_t row,
    size_t col )
{
    assert( row < mat.rows,
        EC_MATRICES_DIM_DONT_MATCH,
        "MatGet: row index greater than or equal to matrix rows number: %d > %d\r\n",
        row, mat.rows );
    assert( col < mat.cols,
        EC_MATRICES_DIM_DONT_MATCH,
        "MatGet: col index greater than or equal to matrix cols number: %d > %d\r\n",
        col, mat.cols );

    if( mat.rowsFilter != 0 )
    {
        row = mat.rowsFilter[row];
    }
    if( mat.colsFilter != 0 )
    {
        col = mat.colsFilter[col];
    }
    return InterpretIntAsFP(
        mat.data[row * mat.rowStride + col * mat.colStride], mat.fracBits );
}

// Set mat[row, col]
// it is assumed 0 <= row < mat.rows and 0 <= col < mat.cols.
static inline void MatSet(
    fp_mat_t mat,
    size_t row,
    size_t col,
    const fp_t value )
{
    assert( row < mat.rows,
        EC_MATRICES_DIM_DONT_MATCH,
        "MatSet: row index greater than or equal to matrix rows number: %d > %d\r\n",
        row, mat.rows );
    assert( col < mat.cols,
        EC_MATRICES_DIM_DONT_MATCH,
        "MatSet: col index greater than or equal to matrix cols number: %d > %d\r\n",
        col, mat.cols );
    assert( mat.fracBits == value.fracBits,
        EC_MATRIX_FRACBITS_DONT_MATCH,
        "MatSet: matrix and value number of fractional bits are different: %d != %d\r\n",
        mat.fracBits, value.fracBits );

    if( mat.rowsFilter != 0 )
    {
        row = mat.rowsFilter[row];
    }
    if( mat.colsFilter != 0 )
    {
        col = mat.colsFilter[col];
    }
    mat.data[row * mat.rowStride + col * mat.colStride] = value.n;
}

// Multiply matrix op1 by matrix op2 and store result in res.
// op1 number of cols has to match op2 number of rows.
// It is assumed res has dimensions op1.rows x op2.cols.
// It is assumed op1 is not the same matrix as res.
// It is assumed op2 is not the same matrix as res. 
void MatMul(
    const fp_mat_t op1,
    const fp_mat_t op2,
    fp_mat_t res );

// Compute the norm of a matrix.
// Returning a boolean to account for overflow cases.
bool MatrixNorm( const fp_mat_t mat, fp_t *result );

// Multiply matrix mat by scalar scale and store result in res.
// It is assumed mat and res have the same dimensions.
void ScaleMat(
    const fp_mat_t mat,
    const fp_t scale,
    fp_mat_t res );

// Divides matrix mat by scalar scale and store result in res.
// It is assumed scale is not zero.
// It is assumed mat and res have the same dimensions.
void InvScaleMat(
    const fp_mat_t mat,
    const fp_t scale,
    fp_mat_t res );

// Fill matrix mat with zeros, then fill its diagonal with value
// It is assumed mat is a square matrix.
void InitMatDiag(
    const fp_mat_t mat,
    const fp_t value );

// Add matrices op1 and op2 and store the result in res.
// It is assumes op1 and op2 have the same dimensions.
void MatAdd(
    const fp_mat_t op1,
    const fp_mat_t op2,
    fp_mat_t res );

// Add matrices op1 and op2 and store the result in res.
// It is assumes op1 and op2 have the same dimensions.
void MatAddMax0(
    const fp_mat_t op1,
    const fp_mat_t op2,
    fp_mat_t res );

// Subtract matrices op1 and op2 and store the result in res.
// It is assumes op1 and op2 have the same dimensions.
void MatSub(
    const fp_mat_t op1,
    const fp_mat_t op2,
    fp_mat_t res );

// Element wise division
// It is assumed op1 and op2 have the same dimensions.
// It is assumed op2 doesn't contain zeros.
void MatDiv(
    const fp_mat_t op1,
    const fp_mat_t op2,
    fp_mat_t res );

// Computes the cross product of matrices op1 and op2.
// It is assumed op1, op2 and res have the same dimensions.
// It is assumed op1, op2 and res are 1x3 or 3x1 matrices.
// It is assumed res is different from op1 and op2.
void MatCrossProduct(
    const fp_mat_t op1,
    const fp_mat_t op2,
    fp_mat_t res );

// Absolute on provided Mat
void MatAbs(
    const fp_mat_t op,
    fp_mat_t res
);

// Computes mean on provided mat's columns, resulting
// in a res of size (N, 1) if the input was (N, M)
void MatColMean(
    const fp_mat_t op,
    fp_mat_t res );

// Computes mean on provided mat's rows, resulting
// in a res of size (1, M) if the input was (N, M)
void MatRowMean(
    const fp_mat_t op,
    fp_mat_t res );

// Transpose matrix mat.
// /!\ The underlying data of mat and its transpose are the same.
// /!\ No copy is performed.
fp_mat_t TransposeMat(
    const fp_mat_t mat );

// Compute the sum of matrix max diagonal's elements.
// It is assumed mat is square matrix.
fp_t MatTrace(
    const fp_mat_t mat );

// Copy the content of matrix src in matrix dst.
// It is assumed src and dst have the same dimensions.
void MatDeepCopy(
    const fp_mat_t src,
    fp_mat_t dst );

// Invert square matrix mat and store it in inv.
// It is assumed mat is square *and* 6x6 (only for now, due to macro stuff)
// It is assumed mat and inv have same dimensions.
// Note that this method may fail due to underflow limitations when
// doing the inversion. Because of this, we return a boolean which is
// false in the case of failure.
bool InvertSquareMatrix6x6(
    const fp_mat_t mat,
    fp_mat_t inv );

// Remove matrix mat's rows that are not in the indices list.
// It is assumed mat is not already row filtered.
// it is assumed the indices list length is lesser than mat.rows
// /!\ The underlying data of mat and its filtered version are the same.
// /!\ No copy is performed.
fp_mat_t FilterRows( fp_mat_t mat, const uint8_t *indices, size_t indicesLen );

// Remove matrix mat's columns that are not in the indices list.
// It is assumed mat is not already column filtered.
// it is assumed the indices list length is lesser than mat.cols
// /!\ The underlying data of mat and its filtered version are the same.
// /!\ No copy is performed.
fp_mat_t FilterCols( fp_mat_t mat, const uint8_t *indices, size_t indicesLen );

// Modify the dimensions of matrix mat.
// It is assumed mat.rows * mat.cols = rows * cols, meaning the reshaped matrix
// has the same number of elements as mat.
// /!\ The underlying data of mat and its reshaped version are the same.
// /!\ No copy is performed.
fp_mat_t ReshapeMat(fp_mat_t mat, size_t rows, size_t cols);

// Add a row to the 2d rotation matrix rotMat2D that is orthogonal to the 
// existent rows. The row is added as a the last row. The new matrix is
// stored in matrix result.
// It is assmued rotMat2D is 2x3.
// It is assumed result is 3x3.
void AddOrthogonalRowTo2DRotMat(
    const fp_mat_t rotMat2D,
    fp_mat_t result );

// Modify matrix mat so that its rows are orthogonal. Stores the result in Q.
// It is assumed mat is 3x3.
// It is assumed Q is 3x3.
// The method is not 100% stable and can fail (we return false if so).
bool MakeMat3x3Orthogonal(
    const fp_mat_t mat,
    fp_mat_t Q );

// Use the space allocated for matrix mat to create a new matrix that can have
// up to the same number of elements as matrix max.
// This is different from ReshapeMat and should not be used the same way.
// RecycleMat should only be used to save memory by avoiding new stack allocation.
// Before recycling a matrix, it should be ensured it's not going to be use
// anymore at all, because the recycled matrix and the new one share the same 
// memory space.
fp_mat_t RecycleMat(
    fp_mat_t mat,
    size_t newRows,
    size_t newCols );

// Helper to check if any elements in the matrix are zero (useful for divisions).
bool MatHasZeros( const fp_mat_t mat );

// Compute cosine distance between two one dimensional matrices (1xN or Nx1).
// It is assumed both matrices have the same dimensions.
// It is assumes the norm of each matrix is different from zero. If that's not
// the case, the result will be zero, even though it's not mathematically sound.
fp_t MatCosineSimilarity( fp_mat_t a, fp_mat_t b );

// Display the content of a matrix on the serial connection
void DebugOutputMatrix( fp_mat_t mat );

// Display the content of a matrix on the serial connection on one line.
void DebugOutputFlatMatrix( fp_mat_t mat );
#endif