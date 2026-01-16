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

#ifndef FIXED_POINT_H
#define FIXED_POINT_H

//=============================================================================
// I N C L U D E   F I L E S   A N D   F O R W A R D   D E C L A R A T I O N S

#include <stdint.h>
#include <stdlib.h>

#include "app_assert.h"
#include "errors.h"
#include "isqrt.h"
#include "types.h"

//=============================================================================
// S T R U C T   D E C L A R A T I O N S

// Represents a real number n * 2^-fracBits
// fracBits is assumed to be positive
typedef struct
{
    int32_t n;
    uint8_t fracBits;
} fp_t;

// fixed point approximation of pi / 4
static const fp_t QUARTER_PI = { 804, 10 };

// fixed point approximation of pi / 2
static const fp_t HALF_PI = { 1608, 10 };

// fixed point approximation of pi
static const fp_t PI = { 3217, 10 };

// fixed point approximation of 2pi
static const fp_t TAU = { 3217 * 2, 10 };

// Lookup table to compute cosine values
#define COS_LUT_LEN 100
static const fp_t COS_LUT[COS_LUT_LEN] =
{
    { 1024, 10 }, { 1024, 10 }, { 1023, 10 }, { 1023, 10 }, { 1022, 10 }, { 1021, 10 }, { 1019, 10 }, { 1018, 10 }, { 1016, 10 },
    { 1014, 10 }, { 1011, 10 }, { 1008, 10 }, { 1005, 10 }, { 1002, 10 }, { 999, 10 }, { 995, 10 }, { 991, 10 }, { 987, 10 },
    { 983, 10 }, { 978, 10 }, { 973, 10 }, { 968, 10 }, { 962, 10 }, { 957, 10 }, { 951, 10 }, { 944, 10 }, { 938, 10 },
    { 931, 10 }, { 925, 10 }, { 917, 10 }, { 910, 10 }, { 903, 10 }, { 895, 10 }, { 887, 10 }, { 879, 10 }, { 870, 10 },
    { 861, 10 }, { 853, 10 }, { 843, 10 }, { 834, 10 }, { 825, 10 }, { 815, 10 }, { 805, 10 }, { 795, 10 }, { 784, 10 },
    { 774, 10 }, { 763, 10 }, { 752, 10 }, { 741, 10 }, { 730, 10 }, { 718, 10 }, { 707, 10 }, { 695, 10 }, { 683, 10 },
    { 671, 10 }, { 658, 10 }, { 646, 10 }, { 633, 10 }, { 620, 10 }, { 607, 10 }, { 594, 10 }, { 581, 10 }, { 567, 10 },
    { 554, 10 }, { 540, 10 }, { 526, 10 }, { 512, 10 }, { 498, 10 }, { 484, 10 }, { 469, 10 }, { 455, 10 }, { 440, 10 },
    { 425, 10 }, { 411, 10 }, { 396, 10 }, { 381, 10 }, { 365, 10 }, { 350, 10 }, { 335, 10 }, { 320, 10 }, { 304, 10 },
    { 288, 10 }, { 273, 10 }, { 257, 10 }, { 241, 10 }, { 226, 10 }, { 210, 10 }, { 194, 10 }, { 178, 10 }, { 162, 10 },
    { 146, 10 }, { 130, 10 }, { 113, 10 }, { 97, 10 }, { 81, 10 }, { 65, 10 }, { 49, 10 }, { 32, 10 }, { 16, 10 },
    { 0, 10 }
};

// Lookup table to compute arcos values
#define ACOS_LUT_LEN 100
static const fp_t ACOS_LUT[ACOS_LUT_LEN] =
{
    { 1608, 10 }, { 1598, 10 }, { 1588, 10 }, { 1577, 10 }, { 1567, 10 }, { 1557, 10 }, { 1546, 10 }, { 1536, 10 }, { 1526, 10 },
    { 1515, 10 }, { 1505, 10 }, { 1494, 10 }, { 1484, 10 }, { 1474, 10 }, { 1463, 10 }, { 1453, 10 }, { 1442, 10 }, { 1432, 10 },
    { 1421, 10 }, { 1411, 10 }, { 1400, 10 }, { 1390, 10 }, { 1379, 10 }, { 1368, 10 }, { 1358, 10 }, { 1347, 10 }, { 1336, 10 },
    { 1326, 10 }, { 1315, 10 }, { 1304, 10 }, { 1293, 10 }, { 1282, 10 }, { 1271, 10 }, { 1261, 10 }, { 1250, 10 }, { 1238, 10 },
    { 1227, 10 }, { 1216, 10 }, { 1205, 10 }, { 1194, 10 }, { 1183, 10 }, { 1171, 10 }, { 1160, 10 }, { 1148, 10 }, { 1137, 10 },
    { 1125, 10 }, { 1114, 10 }, { 1102, 10 }, { 1090, 10 }, { 1078, 10 }, { 1066, 10 }, { 1054, 10 }, { 1042, 10 }, { 1030, 10 },
    { 1018, 10 }, { 1005, 10 }, { 993, 10 }, { 980, 10 }, { 968, 10 }, { 955, 10 }, { 942, 10 }, { 929, 10 }, { 916, 10 },
    { 902, 10 }, { 889, 10 }, { 875, 10 }, { 861, 10 }, { 847, 10 }, { 833, 10 }, { 819, 10 }, { 804, 10 }, { 790, 10 },
    { 775, 10 }, { 759, 10 }, { 744, 10 }, { 728, 10 }, { 712, 10 }, { 696, 10 }, { 679, 10 }, { 662, 10 }, { 645, 10 },
    { 627, 10 }, { 609, 10 }, { 590, 10 }, { 571, 10 }, { 551, 10 }, { 531, 10 }, { 509, 10 }, { 487, 10 }, { 464, 10 },
    { 440, 10 }, { 414, 10 }, { 387, 10 }, { 358, 10 }, { 327, 10 }, { 292, 10 }, { 253, 10 }, { 206, 10 }, { 146, 10 },
    { 0, 10 }
};


//=============================================================================
// M A C R O S

#define CreateLiteralFPInt( n, fracBits ) \
    { ( n * ( 1 << fracBits) ), fracBits }

#define CreateLiteralFP( num, den, fracBits ) \
    { ( num * ( 1 << fracBits) ) / den, fracBits }

// Get fixed point number from float point number in compilation time
#define FloatToFP( floatNum, fpFracBits ) { .n= ( int32_t )( floatNum * ( 1 << fpFracBits ) ), .fracBits = fpFracBits }


//=============================================================================
// I N L I N E   F U N C T I O N S   C O D E   S E C T I O N

//-----------------------------------------------------------------------------
// Returns a fixed point representation of number (num / den) using fracBits
// fractional bits.
// It is assumed den is different from 0.
// It is assumed fracBits is greater than or equal to 0.
static inline fp_t CreateFP( int32_t num, int32_t den, uint8_t fracBits )
{
	assert( den != 0, EC_ZERO_VALUE,
	    "CreateFP: Denominator equal to 0.\r\n" );

	fp_t res = {
		.n = ( num << fracBits ) / den,
		.fracBits = fracBits };

	return res;
}

//-----------------------------------------------------------------------------
// Returns a fixed point representation of an integer n using fracBits
// fractional bits.
// It is assumed n can be represented using 32 - fracBits bits.
// It is assumed fracBits is greater than or equal to 0.
static inline fp_t CreateFPInt( int32_t n, uint8_t fracBits )
{
    assert( ( n >= 0 && n <= (INT32_MAX >> fracBits) ) ||
            ( n < 0 && n >= (INT32_MIN >> fracBits) ),
            EC_FP_CANNOT_REPRESENT_NUMBER,
            "CreateFPInt: Integer %d cannot be represented with a 32 bits fixed point number using %d fractional bits.\r\n", 
            n, fracBits );
	fp_t res = { .n = n << fracBits, .fracBits = fracBits };
	return res;
}

//-----------------------------------------------------------------------------
// Returns the maximum value of a fixed point number given a number of 
// fractional bits.
// It is assumed fracBits is greater than or equal to 0.
static inline fp_t CreateFPMax(uint8_t fracBits)
{
    fp_t res = { .n = INT32_MAX, .fracBits = fracBits };
    return res;
}

//-----------------------------------------------------------------------------
// Returns the minimum value of a fixed point number given a number of
// fractional bits.
// It is assumed fracBits is greater than or equal to 0.
static inline fp_t CreateFPMin(uint8_t fracBits)
{
    fp_t res = { .n = INT32_MIN, .fracBits = fracBits };
    return res;
}

//-----------------------------------------------------------------------------
// Returns the maximum integer value of a fixed point number given a number of
// fractional bits.
// It is assumed fracBits is greater than or equal to 0.
static inline fp_t CreateFPMaxInt(uint8_t fracBits)
{
    fp_t res = { .n = INT32_MAX >> fracBits, .fracBits = fracBits };
    return res;
}

//-----------------------------------------------------------------------------
// Returns the minimum integer value of a fixed point number given a number of
// fractional bits.
// It is assumed fracBits is greater than or equal to 0.
static inline fp_t CreateFPMinInt(uint8_t fracBits)
{
    fp_t res = { .n = INT32_MIN >> fracBits, .fracBits = fracBits };
    return res;
}

//-----------------------------------------------------------------------------
// Returns a fixed point representation of a number whose bits are layed out
// as the ones representing n and which uses fracBits fractional bits.
// It is assumed fracBits is greater than or equal to 0.
static inline fp_t InterpretIntAsFP( int32_t n, uint8_t fracBits )
{
	fp_t res = { .n = n, .fracBits = fracBits };
	return res;
}

//-----------------------------------------------------------------------------
// Returns a new fixed point representation of number n using fracBits.
// This can lead to loss of precision or overflow.
// It is assumed fracBits is greater than or equal to 0.
static inline fp_t ConvertFP( fp_t n, uint8_t fracBits )
{
	int diff = n.fracBits - fracBits;
	
	fp_t res = {
		.n = n.n >> diff,
		.fracBits = fracBits };
	return res;
}

//-----------------------------------------------------------------------------
//
static inline bool IsFPNegative( fp_t x )
{
	return x.n < 0;
}

//-----------------------------------------------------------------------------
// Returns the nearest integer number lower than or equal to n.
// This doesn't round n, it acts as the floor function.
// The returned number has type int32.
static inline int32_t FPToInt32( fp_t n )
{
	return n.n >> n.fracBits;
}

//-----------------------------------------------------------------------------
// Returns the nearest integer number lower than or equal to n.
// This doesn't round n, it acts as the floor function.
// The returned number has type int16.
static inline int16_t FPToInt16( fp_t n )
{
	return n.n >> n.fracBits;
}

//-----------------------------------------------------------------------------
// Ceil()
static inline int16_t FPToCeilInt16( fp_t n )
{
	int16_t result = n.n >> n.fracBits;

	if( !IsFPNegative( n ) )
	{
		int16_t mask = ( 1 << n.fracBits ) - 1;
		if( n.n & mask )
		{
			++result;
		}
	}
	return result;
}

//-----------------------------------------------------------------------------
// Returns a fixed point number representation of number n using fracBits
// fractional_bits.
static inline fp_t Int32ToFP( int32_t n, uint8_t fracBits )
{
	return CreateFP( n, 1, fracBits );
}

//-----------------------------------------------------------------------------
// Initialized an array of fp_t with the value defaultValue.
// It is assumed array points to an array of size arraySize.
static inline void InitFPArray( fp_t defaultValue, fp_t *array, size_t arraySize )
{
	for( size_t i = 0; i < arraySize; ++i )
	{
		array[i] = defaultValue;
	}
}

//-----------------------------------------------------------------------------
// Performs addition.
// op1 and op2 are assumed to have the same fixed point representation.
// Result has same fixed point representation as op1 and op2.
static inline fp_t FPAdd( fp_t op1, fp_t op2 )
{
	fp_t res = {
		.n = op1.n + op2.n,
		.fracBits = op1.fracBits };
	return res;
}

//-----------------------------------------------------------------------------
// Performs addition with max 0.
// op1 and op2 are assumed to have the same fixed point representation.
// Result has same fixed point representation as op1 and op2 and always above 0.
static inline fp_t FPAddMax0( fp_t op1, fp_t op2 )
{
    int32_t n = op1.n + op2.n;
	fp_t res = {
		.n = n > 0 ? n : 0,
		.fracBits = op1.fracBits };
	return res;
}

//-----------------------------------------------------------------------------
// Performs subtraction
// op1 and op2 are assumed to have the same fixed point representation
// Result has the same fixed point representation as op1 and op2.
static inline fp_t FPSub( fp_t op1, fp_t op2 )
{
	fp_t res = {
		.n = op1.n - op2.n,
		.fracBits = op1.fracBits };
	return res;
}

//-----------------------------------------------------------------------------
// Performs multiplication
// op1 and op2 can have different fixed point representation
// Result has the same fixed point representation as op1.
static inline fp_t FPMul( fp_t op1, fp_t op2 )
{
	int64_t n;
	// When multiplying two fixed point numbers, we're adding their fractional
	// bits. We thus need to adjust the result to go back to op1 representation
	// number of fractional bits.
	n = ( (int64_t) op1.n * (int64_t) op2.n ) >> op2.fracBits;
	fp_t res = {
		.n = n,
		.fracBits = op1.fracBits };
	return res;
}

//-----------------------------------------------------------------------------
// Performs squaring.
// Wrapper around fp_mul to make the code a bit more pretty.
static inline fp_t FPSqr( fp_t op )
{
	return FPMul( op, op );
}

//-----------------------------------------------------------------------------
// Extracts the square root of op.
// IMPORTANT : op fixed point representation is assumed to have an even number
// of fractional bits.
// Result has the same fixed point representation as op. Result will be the
// greatest representable number such that result*result <= op.
// It is assumed op is greater than or equal to 0.
static inline fp_t FPSqrt( fp_t op )
{
    assert(
        op.n >= 0, EC_NEGATIVE_VALUE,
        "FPSqrt: Can't extract the sqrt of negative number %d %d.\r\n",
        op.n, op.fracBits );

    assert(
        op.fracBits % 2 == 0, EC_NOT_EVEN,
        "op frac. bits number is not even: %d\r\n", op.fracBits );

    // If op has an even number of fractionnal bits, the square root of
    // n * 2^-fracBits is
    //    sqrt(op) = sqrt(n) * sqrt(2^-fracBits) = sqrt(n) * 2^(-frac-bits/2)
    //    sqrt(op) = sqrt(n) * 2^(-fracBits / 2)
    //    sqrt(op) = (sqrt(n) * 2^(fracBits / 2)) * 2^(-fracBits)
    // So to get the same representation as op for the result, we have to
    // multiply sqrt(n) by 2^(fracBits / 2), which is just shifting fracBits/2
    // times to the left.
    // As isqrt(n) <= sqrt(n) < isqrt(n) + 1, the best fixed point value for
    // sqrt(op) lies somewhere between 
    // (isqrt(n) * 2^(fracBits / 2)) * 2^(-fracBits)
    // and
    // ((isqrt(n) + 1) * 2^(fracBits / 2)) * 2^(-fracBits)
    // We use a dichotomic search to find the best value. Since fracBits / 2 is
    // at most 16, the dichotomic search takes at worst 16 steps.

    int n = ISqrt( op.n ) << ( op.fracBits / 2 );

    // Dichotomic search
    int left = 0;
    int right = ( 1 << ( op.fracBits / 2 ) ) - 1;
    
    int64_t inflatedOp = (int64_t) op.n * ( 1 << op.fracBits );
    // Loop invariant: (n + left) is always lesser than or equal to the true sqrt
    while( left != right )
    {
        int middle = ( right + left ) / 2;
        int candidate = n + middle;
        
        // We want to test if candidate^2 / ( 1 << op.fracBits ) (that's squaring
        // the fixed point number represented by integer candidate) is lesser than
        // or equal to op.n.
        // Since we're dealing with integers, we want to avoid this nasty
        // division that could drop some precision.
        // candidate^2 / ( 1 << op.fracBits ) <= op.n
        // candidate^2 <= op.n * ( 1 << op.fracBits )
        int64_t squaredCandidate = (int64_t) candidate * candidate;
        
        if( squaredCandidate == inflatedOp )
        {
            return InterpretIntAsFP( candidate, op.fracBits );
        }
        
        if( squaredCandidate < inflatedOp )
        {
             if( left != middle )
            {
                // Loop invariant preserved
                left = middle;
            }
            else // Only 2 values left
            {
                candidate = n + right;
                squaredCandidate = (int64_t) candidate * candidate;
                if( squaredCandidate < inflatedOp )
                {
                    left = right; // Loop invariant preserved
                }
                else
                {
                    right = left; // Loop invariant preserved
                }
            }
        }
        else
        {
            // We know n + middle is greater than the true sqrt, otherwise we
            // wouldn't be in that instruction block.
            // But n + left is less than the true sqrt (loop invariant).
            // So n + left < n + middle -->
            // left < middle -->
            // left <= middle - 1.
            // So right cannot become less than left, because that would require
            // left > middle - 1, which is not possible.
            right = middle - 1; // Loop invariant preserved
        }
    }   
    return InterpretIntAsFP( n + left, op.fracBits );
}

//-----------------------------------------------------------------------------
// Performs division.
// op1 and op2 can have different fixed point representation.
// Result has the same fixed point representation as op1.
// It is assumed op2 is different from 0.
static inline fp_t FPDiv( fp_t op1, fp_t op2 )
{
	assert( op2.n != 0, EC_ZERO_VALUE, "FPDiv: Can't divide by 0.\r\n" );
	
	// When dividing one fixed point number op1 by fixed point number op2
	// we are subtracting op2 frac bits from op1 frac bits.
	// We thus need to adjust the result to go back to op1 representation number
	// of fractional bits.
	int64_t n;
	n = ( (int64_t) op1.n << op2.fracBits ) / (int64_t) op2.n;
	fp_t res = {
		.n = n,
		.fracBits = op1.fracBits };
	return res;
}

//-----------------------------------------------------------------------------
// Performs left shift on op1.n op2 times.
// This allows for fast multiplication by powers of 2.
// Result has the same fixed point representation as op1.
static inline fp_t FPLShift( fp_t op1, int op2 )
{
	fp_t res = {
		.n = op1.n << op2,
		.fracBits = op1.fracBits };
	return res;
}

//-----------------------------------------------------------------------------
// Performs right shift on op1.n op2 times.
// This allows for fast division by powers of 2.
// Result has the same fixed point representation as op1.
static inline fp_t FPRShift( fp_t op1, int op2 )
{
	fp_t res = {
		.n = op1.n >> op2,
		.fracBits = op1.fracBits };
	return res;
}

//-----------------------------------------------------------------------------
// Returns the absolute value of op.
// Result has the same fixed point representation as op.
static inline fp_t FPAbs( fp_t op )
{
	fp_t res = {
		.n = abs( op.n ),
		.fracBits = op.fracBits };
	return res;
}

//-----------------------------------------------------------------------------
// Checks if op1 if greater than op1.
// op1 and op2 are assumed to have the same fixed point representation.
static inline bool FPGt( fp_t op1, fp_t op2 )
{
	assert(
		op1.fracBits == op2.fracBits, EC_FP_NOT_EQUIVALENT,
		"FPGt : op1 fracbits != op2 fracbits: %d %d\r\n",
		op1.fracBits, op2.fracBits );
	return op1.n > op2.n;
}

//-----------------------------------------------------------------------------
// Checks if op1 is greater than or equal to op2.
// op1 and op2 are assumed to have the same fixed point representation.
static inline bool FPGe( fp_t op1, fp_t op2 )
{
	assert(
		op1.fracBits == op2.fracBits, EC_FP_NOT_EQUIVALENT,
		"FPGe : op1 fracbits != op2 fracbits: %d %d\r\n",
		op1.fracBits, op2.fracBits );
	return op1.n >= op2.n;
}

//-----------------------------------------------------------------------------
// Checks if op1 is larger than op2.
// op1 and op2 are assumed to have the same fixed point representation.
static inline bool FPLt( fp_t op1, fp_t op2 )
{
	assert(
		op1.fracBits == op2.fracBits, EC_FP_NOT_EQUIVALENT,
		"FPLt : op1 fracbits != op2 fracbits: %d %d\r\n",
		op1.fracBits, op2.fracBits );
	return op1.n < op2.n;
}

//-----------------------------------------------------------------------------
// Checks if op1 is larger than or equal to op2.
// op1 and op2 are assumed to have the same fixed point representation.
static inline bool FPLe( fp_t op1, fp_t op2 )
{
	assert(
		op1.fracBits == op2.fracBits, EC_FP_NOT_EQUIVALENT,
		"FPLe : op1 fracbits != op2 fracbits: %d %d\r\n",
		op1.fracBits, op2.fracBits );
	return op1.n <= op2.n;
}

//-----------------------------------------------------------------------------
// Returns true if value is between lower_bound and upper_bound. 
// All values are assumed to have the same fixed point representation.
static inline bool FPBetween(fp_t value, fp_t lower_bound, fp_t upper_bound)
{
	return FPGe(value, lower_bound) && FPLe(value, upper_bound);
}

//-----------------------------------------------------------------------------
// Checks if op1 is equal to op2.
// op1 and op2 are assumed to have the same fixed point representation.
static inline bool FPEq( fp_t op1, fp_t op2 )
{
	assert(
		op1.fracBits == op2.fracBits, EC_FP_NOT_EQUIVALENT,
		"FPEq : op1 fracbits != op2 fracbits: %d %d\r\n",
		op1.fracBits, op2.fracBits );
	return op1.n == op2.n;
}

//-----------------------------------------------------------------------------
// Checks if op1 is different from op2.
// op1 and op2 are assumed to have the same fixed point representation.
static inline bool FPNe( fp_t op1, fp_t op2 )
{
	assert(
		op1.fracBits == op2.fracBits, EC_FP_NOT_EQUIVALENT,
		"FPNe : op1 fracbits != op2 fracbits: %d %d\r\n",
		op1.fracBits, op2.fracBits );
	return op1.n != op2.n;
}

//-----------------------------------------------------------------------------
// Returns the max of input values.
// op1 and op2 are assumed to have the same fixed point representation.
static inline fp_t FPMax( fp_t op1, fp_t op2 )
{
	assert(
		op1.fracBits == op2.fracBits, EC_FP_NOT_EQUIVALENT,
		"FPMax : op1 fracbits != op2 fracbits: %d %d\r\n",
		op1.fracBits, op2.fracBits );
	return FPGt( op1, op2 ) ? op1 : op2;
}

//-----------------------------------------------------------------------------
// Returns the min of input values.
// op1 and op2 are assumed to have the same fixed point representation.
static inline fp_t FPMin( fp_t op1, fp_t op2 )
{
	assert(
		op1.fracBits == op2.fracBits, EC_FP_NOT_EQUIVALENT,
		"FPMin : op1 fracbits != op2 fracbits: %d %d\r\n",
		op1.fracBits, op2.fracBits );
	return FPLt( op1, op2 ) ? op1 : op2;
}

//-----------------------------------------------------------------------------
// Returns the greatest integer that is lesser than or equal to number x.
static inline int FPFloor( fp_t x )
{
    return x.n >> x.fracBits;
}

//-----------------------------------------------------------------------------
// Returns the greatest integer that is lesser than or equal to number x.
// All arguments are assumed to have the same fixed point representation.
static inline fp_t FPClip( fp_t x, fp_t min, fp_t max )
{
    assert(
        x.fracBits == min.fracBits, EC_FP_NOT_EQUIVALENT,
        "FPMin : x fracbits != min fracbits: %d %d\r\n",
        x.fracBits, min.fracBits );
    assert(
        x.fracBits == max.fracBits, EC_FP_NOT_EQUIVALENT,
        "FPMin : x fracbits != max fracbits: %d %d\r\n",
        x.fracBits, max.fracBits );
    assert(
        min.fracBits == max.fracBits, EC_FP_NOT_EQUIVALENT,
        "FPMin : min fracbits != max fracbits: %d %d\r\n",
        min.fracBits, max.fracBits );

    return FPMax( min, FPMin( max, x ) );
}

static inline int FPRound( fp_t x )
{
    int fracMask = ( 1 << x.fracBits ) - 1;
    int halfFrac = 1 << ( x.fracBits - 1 );
    int fracPart = x.n & fracMask;
	int floor = x.n >> x.fracBits;
	if( fracPart == halfFrac )
	{
		if( floor % 2 == 0 )
		{
			return floor;
		}
		else
		{
			return floor + 1;
		}
	}
	else if( fracPart > halfFrac )
	{
		return floor + 1;
	}
	else
	{
		return floor;
	}
}

//-----------------------------------------------------------------------------
// Returns -x.
// The number of fractional bits of the result is the same as the number used 
// for x.
static inline fp_t FPMinus( fp_t x )
{
    fp_t result = x;
    result.n *= -1;
    return result;
}

//-----------------------------------------------------------------------------
// Compute the cosine of x. 
// x is expected to be in radians.
static inline fp_t FPCos( fp_t x )
{
	// This function actually computes result for 0 <= x < PI.
	// First step is to move x to this interval

    uint8_t fracBits = x.fracBits;
    fp_t zero = CreateFPInt( 0, fracBits );

	// Set x value between 0 and 2pi
    while( FPLt( x, zero ) )
    {
		// cos(x) = cox(x + 2*PI)
        x = FPAdd( x, TAU );
    }
    while( FPGe( x, TAU ) )
    {
		// cos(x) = cos(x - 2*PI)
        x = FPSub( x, TAU );
    }

	// Here, 0 <= x < 2*PI

	// PI < x < 2PI
    if( FPGt( x, PI ) )
    {
		// 0 < (x - PI) < PI
		// cos(x) = -cos(x-PI)
        return FPMinus( FPCos( FPSub( x, PI ) ) );
    }

	// If we're here, 0 <= x < PI
	// cos(PI/2) = 0
    if( FPEq( x, HALF_PI ) )
    {
        return zero;
    }

	// If we're here, 0 <= x < PI

	// PI/2 < x < PI
    if( FPGe( x, HALF_PI ) )
    {
		// 0 < (PI - x) < PI/2 
		// cos(x) = -cos(PI-x)
		return FPMinus( FPCos( FPSub( PI, x ) ) );
    }

	// if we're here, 0 <= x <= PI/2
	// Actual computation happens from there
	
	// Interpolate cos(x) value using the lookup table content
    fp_t fpIndex = FPDiv( x, HALF_PI );
    fpIndex.n *= COS_LUT_LEN - 1;

    int index = FPFloor( fpIndex );

    fp_t value1 = COS_LUT[index];
    fp_t value2 = COS_LUT[index + 1];
    fp_t distance = FPSub( value2, value1 );
    fp_t ratio = FPSub( fpIndex, CreateFPInt( index, fracBits ) );

    return FPAdd( value1, FPMul( ratio, distance ) );
}

//-----------------------------------------------------------------------------
// Compute the sine of x. 
// x is expected to be in radians.
static inline fp_t FPSin( fp_t x )
{
	// sin(x) = cos(x - PI/2)
    return FPCos( FPSub( x, HALF_PI ) );
}

//-----------------------------------------------------------------------------
// Compute the tan of x.
// x is expected to be in radians.
static inline fp_t FPTan( fp_t x )
{
	// tan(x) = sin(x) / cos(x)
    return FPDiv( FPSin( x ), FPCos( x ) );
}

//-----------------------------------------------------------------------------
// Compute the arc cosine of x. 
// x is expected to be in [-1, 1]
static inline fp_t FPAcos( fp_t x )
{
	// This function actually computes result for 0 <= x <= 1
	// First step is to move x in this interval

    uint8_t fracBits = x.fracBits;
    fp_t zero = CreateFPInt( 0, fracBits );
    fp_t one = CreateFPInt( 1, fracBits );

	assert( FPLe( FPMinus( one ), x ) && FPLe( x, one ),
		EC_OUT_OF_BOUNDS,
		"FPAcos: x value %d is not in interval [%d, %d]\r\n",
		x.n, FPMinus( one ).n, one.n );

	// x < 0
    if( FPLt( x, zero ) )
    {
		// arccos(x) = PI - arccos(-x)
        return FPSub( PI, FPAcos( FPMinus( x ) ) );
    }

	// If we're here, 0 < x < 1
	
	// arccos(1) = 0
    if( FPEq( x, one ) )
    {
        return zero;
    }

	// if we're here, 0 < x < 1
	// Actual computation happens from there

	// Interpolate arccos(x) using the lookup table content
    fp_t fpIndex = { x.n * ( ACOS_LUT_LEN - 1 ), fracBits };
    int index = FPFloor( fpIndex );

    fp_t value1 = ACOS_LUT[index];
    fp_t value2 = ACOS_LUT[index + 1];

    fp_t distance = FPSub( value2, value1 );
    fp_t ratio = FPSub( fpIndex, CreateFPInt( index, fracBits ) );
    
    return FPAdd( value1, FPMul( ratio, distance ) );
}

//-----------------------------------------------------------------------------
// Compute the arc sine of x. 
// x is expected to be in [-1 ,1]
static inline fp_t FPAsin( fp_t x )
{
	assert(
		FPLe( FPMinus( CreateFPInt( 1, x.fracBits ) ), x ) &&
		FPLe( x, CreateFPInt( 1, x.fracBits ) ),
		EC_OUT_OF_BOUNDS,
		"FPAsin: x value %d is not in interval [%d, %d]\r\n",
		x.n, CreateFPInt( -1, x.fracBits ).n, CreateFPInt( 1, x.fracBits ).n );
	// arcsin(x) = PI/2 - arccos(x)
    return FPSub( HALF_PI, FPAcos( x ) );
}

//-----------------------------------------------------------------------------
// Compute the arc tan of x. 
static inline fp_t FPAtan( fp_t x )
{
	// This function actually computes result for -1 <= x <= 1
	// The first step is to move x in this interval
    uint8_t fracBits = x.fracBits;
    fp_t one = CreateFPInt( 1, fracBits );
    fp_t zero = CreateFPInt( 0, fracBits );

	// x < 0
    if( FPLt( x, zero ) )
    {
        return FPMinus( FPAtan( FPMinus( x ) ) );
    }

	// 1 < x
    if( FPGt( x, one ) )
    {
		// arctan(x) = PI/2 - arctan(1/x)
        fp_t inv = FPDiv( one, x );
        fp_t result = FPAtan( inv );
        result = FPSub( HALF_PI, result );
        return result;
    }

	// If we're here, 0 < x < 1
	// Actual computation happens from there
    
	// Magic formula: arctan(x) = pi4/*x - x*(abs(x) - 1)*(0.2447 + 0.0663*abs(x))
	// See: Efficient Approximations for the Arctangent Function, 
	// Rajan, Wang, Inkol & Joyal, 2006
	// https://www-labs.iro.umontreal.ca/~mignotte/IFT2425/Documents/EfficientApproximationArctgFunction.pdf
    fp_t result = FPSub(
        FPMul( QUARTER_PI, x ),
        FPMul(
            x,
            FPMul(
                FPSub( FPAbs( x ), CreateFPInt( 1, fracBits ) ),
                FPAdd(
                    CreateFP(2447, 10000, fracBits),
                    FPMul(
                        CreateFP( 663, 10000, fracBits ),
                        FPAbs( x )
                    )
                )
            )
        )
    );
    return result;
}

//-----------------------------------------------------------------------------
// Compute the direct angle between vector (1, 0) and (x, y)
static inline fp_t FPAtan2( fp_t y, fp_t x )
{
    uint8_t fracBits = x.fracBits;
    fp_t zero = CreateFPInt( 0, fracBits );
    
	// Using the following definition or arctan2:
	// arctan2(y, x) =
	//   - arctan(y / x)      if x > 0
	//   - arctan(y / x) + PI if x < 0 and y > 0
	//   - arctan(y / x) - PI if x < 0 and y < 0
	//   - PI/2               if x = 0 and y > 0
	//   - -PI/2              if x = 0 and y < 0
	//   - 0                  if x = 0 and y = 0
	//
	// arctan(0, 0) should actually be undefined

    if( FPEq( x, zero ) )
    {
        if( FPEq( y, zero ) )
        {
            return zero;
        }
        else
        {
            if( FPGt( y, zero ) )
            {
                return HALF_PI;
            }
            else
            {
                return FPMinus( HALF_PI );
            }
        }
    }
    else if( FPGt( x, zero ) )
    {
        return FPAtan( FPDiv( y, x ) );
    }
    else
    {
        if( FPGe( y, zero ) )
        {
            return FPAdd( FPAtan( FPDiv( y, x ) ), PI );
        }
        else
        {
            return FPSub( FPAtan( FPDiv( y, x ) ), PI );
        }
    }
}

//-----------------------------------------------------------------------------
// Compute an approximation of 1 / (1 + e^-x)
// TODO: this is not a good approximation, we can and should probably do much 
// better.
static inline fp_t FPSigmoid( fp_t x )
{
    fp_t scale = InterpretIntAsFP( 325, 10 );
    return FPMul( FPAdd( FPAtan( x ), HALF_PI ), scale );
}

//-----------------------------------------------------------------------------
//
static inline fp_t FPSigmoidWithParams( fp_t x, fp_t a, fp_t b )
{
    fp_t exp = FPAdd( FPMul( x, a ), b );
    return FPSigmoid( exp );
}

//-----------------------------------------------------------------------------
//
static inline fp_t RadiansToDegrees( fp_t radians )
{
	return FPDiv( FPMul( radians, CreateFPInt( 360, radians.fracBits ) ), TAU );
}

//-----------------------------------------------------------------------------
//
static inline fp_t DegreesToRadians( fp_t degrees )
{
	return FPDiv( FPMul( degrees, TAU ), CreateFPInt( 360, degrees.fracBits ) );
}

//-----------------------------------------------------------------------------
//
static inline bool IsFPCloseToZero( fp_t x )
{	
	fp_t tolerance = { .n = 1, .fracBits = x.fracBits };
	return FPGe( x, FPMinus( tolerance ) ) && FPLe( x, tolerance );
}

//-----------------------------------------------------------------------------
// Compute the average value of an array.
// The array is expected to have at least one entry.
static inline fp_t FPAvg( fp_t *array, size_t arraySize )
{
    assert(arraySize > 0, EC_NEGATIVE_OR_ZERO, 
        "FPAvg: arraySize value %d is zero or negative.");
    fp_t average = CreateFPInt( 0, array[0].fracBits );
    
    for (size_t i = 0; i < arraySize; ++i)
    {
        average = FPAdd( average, array[i] );
    }
    
    return FPDiv( average, CreateFPInt( arraySize, average.fracBits ) );
}

//-----------------------------------------------------------------------------
// Identity function.
    static inline fp_t FPId( fp_t n )
{
    return n;
}

//-----------------------------------------------------------------------------
// 
static inline float FP2Float(fp_t x )
{
    return (float)x.n / (float)(1 << x.fracBits);
}

//-----------------------------------------------------------------------------
// 
// Converts a fixed point number to a string representation
// num: the fixed point number to convert
// numstr: the buffer to store the string representation
// bufSize: the size of the buffer
// Returns the number of characters written to the buffer, 
//      or returns -1 if there is not enough buffer space for the integer part
static inline int32_t FPToStr( fp_t num, char* numStr, int32_t bufSize ) {
    if ( bufSize <= 0 ) {
        return -1;
    }

    bool negative = num.n < 0;
    int32_t absNumN = negative ? -num.n : num.n;

    // Calculate the integer and fractional parts
    int32_t intPart = absNumN >> num.fracBits;
    int32_t fracBitMask = ( 1 << num.fracBits ) - 1;
    int64_t fracPart = absNumN & fracBitMask;

    // Format the integer part
    int32_t written = 0;
    if ( negative ) {
        if (bufSize > 1) {
            numStr[written++] = '-';
        }
        else {
            numStr[0] = '\0';
            return -1;
        }
    }

    // Convert integer part to string
    int32_t intStart = written;
    do {
        if (written < bufSize - 1) {
            numStr[written++] = '0' + (intPart % 10);
        } else {
            numStr[0] = '\0';
            return -1;
        }
        intPart /= 10;
    } while (intPart > 0);
    
    // Reverse the integer part string
    for (int32_t i = intStart, j = written - 1; i < j; ++i, --j) {
        char temp = numStr[i];
        numStr[i] = numStr[j];
        numStr[j] = temp;
    }

    // Convert fractional part to string
    if (fracPart > 0 && written < bufSize - 1) {
        numStr[written++] = '.';
        while (fracPart > 0 && written < bufSize - 1) {
            fracPart *= 10;
            int32_t digit = fracPart >> num.fracBits;
            numStr[written++] = '0' + digit;
            fracPart &= fracBitMask;
        }

        // Remove trailing zeros from fractional part
        while (numStr[written - 1] == '0') {
            --written;
        }
    }

    // Ensure null termination
    numStr[written] = '\0';

    return written + 1; // Count '\0' character
}

#endif
