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

#include "app_assert.h"
#include "errors.h"
#include "box.h"
#include "debug.h"
#include "quick_select.h"

//=============================================================================
// F U N C T I O N S   C O D E   S E C T I O N

//----------------------------------------------------------------------------
//
geometric_box_t CreateGeometricBox( 
	fp_t left,  fp_t top, 
	fp_t right, fp_t bottom )
{
	assert( FPLe( left, right ), EC_OUT_OF_BOUNDS,
		"Invalid geometric box coordinates - left: %d   right: %d (frac. bits %d)\r\n",
		left.n, right.n, left.fracBits );
	assert( FPLe( top, bottom ), EC_OUT_OF_BOUNDS,
		"Invalid geometric box coordinates - top: %d   bottom: %d (frac. bits %d)\r\n",
		top.n, bottom.n, top.fracBits );
	assert(
		left.fracBits == top.fracBits &&
		top.fracBits == right.fracBits &&
		right.fracBits == bottom.fracBits, EC_FP_NOT_EQUIVALENT,
		"All box coordinates must have the same nb. of frac. bits %d %d %d %d\r\n",
		left.fracBits, top.fracBits, right.fracBits, bottom.fracBits);

	geometric_box_t box = 
	{
		.left = left, .top = top, .right = right, .bottom = bottom
	};
	return box;
}

//----------------------------------------------------------------------------
//
pixel_box_t CreatePixelBox( 
	int32_t left, int32_t top,
	int32_t right, int32_t bottom )
{
	assert( left <= right, EC_OUT_OF_BOUNDS,
		"Invalid pixel box coordinates - left: %d   right: %d\r\n",
		left, right );
	assert( top <= bottom, EC_OUT_OF_BOUNDS,
		"Invalid pixel box coordinates - top: %d   bottom: %d\r\n",
		top, bottom );
	pixel_box_t box = CreateLiteralPixelBox( left, top, right, bottom );
	return box;
}

//----------------------------------------------------------------------------
//
geometric_point_2d_t CreateGeometricPoint( fp_t x, fp_t y )
{
	assert(
		x.fracBits == y.fracBits, EC_FP_NOT_EQUIVALENT,
		"Point coordinates don't use the same number of fractional bits: %d %d\r\n",
		x.fracBits, y.fracBits );

	geometric_point_2d_t point = {
		.x = x,
		.y = y };
	return point;
}

//----------------------------------------------------------------------------
//
geometric_point_3d_t CreateGeometricPoint3d( fp_t x, fp_t y, fp_t z )
{
	assert(
		x.fracBits == y.fracBits && x.fracBits == z.fracBits, EC_FP_NOT_EQUIVALENT,
		"Point coordinates don't use the same number of fractional bits: %d %d %d\r\n",
		x.fracBits, y.fracBits, z.fracBits );

	geometric_point_3d_t point = {
		.x = x,
		.y = y,
		 .z = z };
	return point;
}

//----------------------------------------------------------------------------
//
pixel_point_t CreatePixelPoint( int32_t x, int32_t y )
{
	pixel_point_t point = { .x = x, .y = y };
	return point;
}

//----------------------------------------------------------------------------
//
pixel_vector_t CreatePixelVector( int32_t x, int32_t y )
{
	pixel_vector_t vector = { .x = x, .y = y };
	return vector;
}

//----------------------------------------------------------------------------
//
geometric_point_2d_t TranslateGeometricPoint2d(
	geometric_point_2d_t point, geometric_vector_2d_t vector )
{
	geometric_point_2d_t newPoint = {
		.x = FPAdd( point.x, vector.x ),
		.y = FPAdd( point.y, vector.y )
	};
	return newPoint;
}

//----------------------------------------------------------------------------
//
void TranslateGeometricPoints2d(
	geometric_point_2d_t *points, size_t pointsNb, geometric_vector_2d_t vector )
{
	for( size_t i = 0; i < pointsNb; ++i )
	{
		points[i] = TranslateGeometricPoint2d( points[i], vector );
	}
}

//----------------------------------------------------------------------------
//
geometric_point_3d_t TranslateGeometricPoint3d(
	geometric_point_3d_t point, geometric_vector_3d_t vector )
{
	geometric_point_3d_t newPoint = {
		.x = FPAdd( point.x, vector.x ),
		.y = FPAdd( point.y, vector.y ),
		.z = FPAdd( point.z, vector.z )
	};
	return newPoint;
}

//----------------------------------------------------------------------------
//
void
TranslateGeometricPoints3d(
geometric_point_3d_t *points, size_t pointsNb, geometric_vector_3d_t vector )
{
	for( size_t i = 0; i < pointsNb; ++i )
	{
		points[i] = TranslateGeometricPoint3d( points[i], vector );
	}
}

//----------------------------------------------------------------------------
//
geometric_box_t ScaleAndOffsetBox(
	geometric_box_t box,
	fp_t horizontalScale, fp_t verticalScale,
	geometric_vector_2d_t offset )
{
	assert(
		FPGe( horizontalScale, CreateFPInt( 0, horizontalScale.fracBits ) ),
		EC_NEGATIVE_VALUE, "Horizontal scale is lesser than 0: %d %d",
		horizontalScale.n, horizontalScale.fracBits );
	assert(
		FPGe( verticalScale, CreateFPInt( 0, horizontalScale.fracBits ) ),
		EC_NEGATIVE_VALUE, "Vertical scale is lesser than 0: %d %d",
		verticalScale.n, verticalScale.fracBits );

	fp_t top = FPAdd( FPMul( box.top, verticalScale ), offset.y );
	fp_t left = FPAdd( FPMul( box.left, horizontalScale ), offset.x );
	fp_t bottom = FPAdd( FPMul( box.bottom, verticalScale ), offset.y );
	fp_t right = FPAdd( FPMul( box.right, horizontalScale ), offset.x );
	geometric_box_t new_box = CreateGeometricBox( left, top, right, bottom );
	return new_box;
}

//----------------------------------------------------------------------------
//
geometric_box_t OffsetAndScaleBox(
	geometric_box_t box,
	geometric_vector_2d_t offset,
	fp_t horizontalScale, fp_t verticalScale )
{
	assert(
		FPGe( horizontalScale, CreateFPInt( 0, horizontalScale.fracBits ) ),
		EC_NEGATIVE_VALUE, "Horizontal scale is lesser than 0: %d %d",
		horizontalScale.n, horizontalScale.fracBits );
	assert(
		FPGe( verticalScale, CreateFPInt( 0, horizontalScale.fracBits ) ),
		EC_NEGATIVE_VALUE, "Vertical scale is lesser than 0: %d %d",
		verticalScale.n, verticalScale.fracBits );

	fp_t top = FPMul( FPAdd( box.top, offset.y ), verticalScale );
	fp_t left = FPMul( FPAdd( box.left, offset.x ), horizontalScale );
	fp_t bottom = FPMul( FPAdd( box.bottom, offset.y ), verticalScale );
	fp_t right = FPMul( FPAdd( box.right, offset.x ), horizontalScale );

	geometric_box_t new_box = CreateGeometricBox( left, top, right, bottom );
	return new_box;
}

//----------------------------------------------------------------------------
//
geometric_point_2d_t ScaleAndOffsetPoint(
	const geometric_point_2d_t point,
	const fp_t horizontalScale, const fp_t verticalScale,
	const geometric_vector_2d_t offset )
{
	assert(
		FPGe( horizontalScale, CreateFPInt( 0, horizontalScale.fracBits ) ),
		EC_NEGATIVE_VALUE, "Horizontal scale is less than 0: %d %d",
		horizontalScale.n, horizontalScale.fracBits );
	assert(
		FPGe( verticalScale, CreateFPInt( 0, horizontalScale.fracBits ) ),
		EC_NEGATIVE_VALUE, "Vertical scale is less than 0: %d %d",
		verticalScale.n, verticalScale.fracBits );

	fp_t y = FPAdd( FPMul( point.y, verticalScale ), offset.y );
	fp_t x = FPAdd( FPMul( point.x, horizontalScale ), offset.x );
	geometric_point_2d_t newPoint = CreateGeometricPoint( x, y );
	return newPoint;
}

//----------------------------------------------------------------------------
//
fp_t ComputeGeometricArea( geometric_box_t box )
{
	return FPMul( GetGeometricBoxWidth(&box), GetGeometricBoxHeight(&box) );
}

//----------------------------------------------------------------------------
//
fp_t ComputeGeometricIoU( geometric_box_t box1, geometric_box_t box2 )
{
	assert(
		box1.left.fracBits == box2.left.fracBits, EC_FP_NOT_EQUIVALENT,
		"Boxes don't use the same number of fractional bits: %d %d\r\n",
		box1.left.fracBits, box2.left.fracBits );

	fp_t zero = CreateFP( 0, 1, box1.top.fracBits );
	fp_t area_1 = ComputeGeometricArea( box1 );
	fp_t area_2 = ComputeGeometricArea( box2 );
	fp_t top = FPLt( box1.top, box2.top ) ? box2.top : box1.top;
	fp_t left = FPLt( box1.left, box2.left ) ? box2.left : box1.left;
	fp_t bottom = FPLt( box1.bottom, box2.bottom ) ?
		box1.bottom : box2.bottom;
	fp_t right = FPLt( box1.right, box2.right ) ? box1.right : box2.right;
	fp_t width =
		FPGt( FPSub( right, left ), zero ) ? FPSub( right, left ) : zero;
	fp_t height =
		FPGt( FPSub( bottom, top ), zero ) ? FPSub( bottom, top ) : zero;
	fp_t interArea = FPMul( width, height );
	fp_t unionArea = FPSub( FPAdd( area_1, area_2 ), interArea );
	fp_t iou = FPDiv( interArea, unionArea );
	return iou;
}

//----------------------------------------------------------------------------
//
geometric_box_t CropGeometricBox(
	geometric_box_t box, geometric_box_t container )
{
	DebugOutput( "Crop geometric box\r\n" );
	DebugOutput( 
		"box : %d %d %d %d\r\n", 
		box.left.n, box.top.n, box.right.n, box.bottom.n );
	DebugOutput( 
		"container : %d %d %d %d\r\n",  container.left.n, container.top.n, 
		container.right.n, container.bottom.n );
	assert(
		box.left.fracBits == container.left.fracBits, EC_FP_NOT_EQUIVALENT,
		"Box and container don't use the same number of fractional bits: %d %d\r\n",
		box.left.fracBits, container.left.fracBits );

	fp_t left = box.left;
	fp_t right = box.right;
	fp_t top = box.top;
	fp_t bottom = box.bottom;

	// If intersection between box and container is empty
	if( FPLt( right, container.left ) ||
		FPLt( bottom, container.top ) ||
		FPGt( left, container.right ) ||
		FPGt( top, container.bottom ) )
	{
		fp_t zero = CreateFPInt( 0, container.top.fracBits );
		return CreateGeometricBox( zero, zero, zero, zero );
	}

	if( FPLt( left, container.left ) )
	{
		left = container.left;
	}

	if( FPLt( top, container.top ) )
	{
		top = container.top;
	}

	if( FPGt( right, container.right ) )
	{
		right = container.right;
	}

	if( FPGt( bottom, container.bottom ) )
	{
		bottom = container.bottom;
	}

	return CreateGeometricBox( left, top, right, bottom );
}

//----------------------------------------------------------------------------
//
geometric_vector_2d_t CreateGeometricVector2d(
	geometric_point_2d_t src, geometric_point_2d_t dst )
{
	assert(
		src.x.fracBits == dst.x.fracBits, EC_FP_NOT_EQUIVALENT,
		"Src and dst points don't use the same number of fractional bits: %d %d\r\n",
		src.x.fracBits, dst.x.fracBits );

	geometric_vector_2d_t vector = {
		.x = FPSub( dst.x, src.x ),
		.y = FPSub( dst.y, src.y )
	};
	return vector;
}

//----------------------------------------------------------------------------
//
geometric_vector_3d_t CreateGeometricVector3d(
	geometric_point_3d_t src, geometric_point_3d_t dst )
{
	assert(
		src.x.fracBits == dst.x.fracBits, EC_FP_NOT_EQUIVALENT,
		"Src and dst points don't use the same number of fractional bits: %d %d\r\n",
		src.x.fracBits, dst.x.fracBits );

	geometric_vector_3d_t vector = {
		.x = FPSub( dst.x, src.x ),
		.y = FPSub( dst.y, src.y ),
		.z = FPSub( dst.z, src.z ),
	};
	return vector;
}

//----------------------------------------------------------------------------
//
fp_t VectorNorm2d( geometric_vector_2d_t vector )
{
	return FPSqrt( FPAdd( FPSqr( vector.x ), FPSqr( vector.y ) ) );
}

//----------------------------------------------------------------------------
//
fp_t VectorNorm2d64bits( geometric_vector_2d_t vector )
{
	uint8_t fractBits = vector.x.fracBits;
	// x^2
	int64_t x;
	x = ( (int64_t) vector.x.n * (int64_t) vector.x.n ) >> fractBits;
	
	// y^2
	int64_t y;
	y = ( (int64_t) vector.y.n * (int64_t) vector.y.n ) >> fractBits;
	
	// x^2 + y^2
	int64_t sum = x + y;

	// sqrt(x^2 + y^2)
	// Babylonian Method with initial guess computed by Bresenham's algorithm
	int64_t guess = ( 
		FPAdd( 
			FPMul( 
				( FPSub( 
					FPSqrt( CreateFPInt( 2, fractBits ) ) , 
					CreateFPInt( 1, fractBits ) ) ), 
				vector.y ), 
			vector.x ) ).n;
	int64_t prevGuess;

	do {
		prevGuess = guess;
		guess = ( ( ( guess * guess ) + ( sum << fractBits ) ) / ( guess << 1 ) );
		DebugOutput( "Guess: %lld. Prev guess: %lld\r\n", guess, prevGuess );
	} while( guess != prevGuess );

	assert(
		guess <= INT32_MAX, EC_OUT_OF_BOUNDS,
		"Fixed point number overflow when calculating vector norm."
		"Vector: x.n = %ld, y.n = %ld, fracBits = %u"
		"Result: n = %lld, fractBits = %u\r\n",
		guess, fractBits );

	fp_t vectorNorm = { .n = (int32_t)guess, .fracBits = fractBits };
	return vectorNorm;
}

//----------------------------------------------------------------------------
//
fp_t VectorNorm3d( geometric_vector_3d_t vector )
{
	return FPSqrt( FPAdd( FPAdd( FPSqr( vector.x ), FPSqr( vector.y ) ),
		FPSqr( vector.z ) ) );
}

//----------------------------------------------------------------------------
//
fp_t BoxDistToIdealPoint2D(
	geometric_box_t box, geometric_point_2d_t idealPoint )
{
	geometric_point_2d_t center = CreateGeometricPoint(
		FPAdd(box.left, FPRShift( FPSub( box.right, box.left ), 1 )),
		FPAdd(box.top,FPRShift( FPSub( box.bottom, box.top ), 1 )) );
		
	geometric_vector_2d_t toCenterVector =
		CreateGeometricVector2d( center, idealPoint );
	return VectorNorm2d( toCenterVector );
}

//----------------------------------------------------------------------------
//
geometric_point_2d_t GetGeometricBoxCenter( const geometric_box_t *box )
{
    fp_t halfWidth = FPRShift( FPSub( box->right, box->left ), 1 );
    fp_t halfHeight = FPRShift( FPSub( box->bottom, box->top ), 1 );
    geometric_point_2d_t center = CreateGeometricPoint(
		FPAdd( box->left, halfWidth ),
		FPAdd( box->top, halfHeight ) );
    return center;
}

// Returns width (right - left) of a geometric box
fp_t GetGeometricBoxWidth(const geometric_box_t *box)
{
	return FPSub(box->right, box->left);
}

// Returns height (bottom - top) of a geometric box
fp_t GetGeometricBoxHeight(const geometric_box_t *box)
{
	return FPSub(box->bottom, box->top);
}

//----------------------------------------------------------------------------
// 
bool PointInsideBox(geometric_point_2d_t point, geometric_box_t box)
{
    return FPGt(point.x, box.left) &&
           FPLt(point.x, box.right) &&
           FPGt(point.y, box.top) &&
           FPLt(point.y, box.bottom);
}

//----------------------------------------------------------------------------
//
size_t MatchGeometricBox(
    const geometric_box_t *toMatch,
    fp_t iouThreshold,
    const geometric_box_t *candidates,
    size_t candidatesNb )
{
    if( candidatesNb == 0 )
    {
        return 0;
    }
    
    size_t bestCandidate = 0;
    fp_t bestIoU = ComputeGeometricIoU( *toMatch, candidates[0] );
    for( size_t i = 1; i < candidatesNb; ++i )
    {
        fp_t currentIoU = ComputeGeometricIoU( *toMatch, candidates[i] );
        if( FPGe( currentIoU, bestIoU ) )
        {
            bestIoU = currentIoU;
            bestCandidate = i;
        }
    }
    
    if( FPGt( bestIoU, iouThreshold ) )
    {
        return bestCandidate;
    }
    else
    {
        return candidatesNb;
    }
}

//-----------------------------------------------------------------------------
// 
geometric_box_t ComputeRectangleEnvelope( 
    const geometric_point_2d_t *points,
    size_t size )
{
    assert( size > 0, EC_ZERO_VALUE,
        "ComputeEnclosingBoundingBox: nbLandmarks is zero.\r\n");
  
    geometric_point_2d_t point = points[0];

	fp_t left = point.x;
	fp_t top = point.y;
	fp_t right = point.x;
	fp_t bottom = point.y;

	for( size_t i = 1; i < size; ++i )
	{
		point = points[i];
		left = FPLt( left, point.x ) ?
			left : point.x;
		right = FPLt( right, point.x ) ?
			point.x : right;
		top = FPLt( top, point.y ) ?
			top : point.y;
		bottom = FPLt( bottom, point.y ) ?
			point.y : bottom;
	}
    
	geometric_box_t envelope = CreateGeometricBox( left, top, right, bottom );
	return envelope;
}

//-----------------------------------------------------------------------------
// 
pixel_box_t GeometricToPixelBox( const geometric_box_t *box )
{
    return CreatePixelBox(
        FPRound( box->left ),
        FPRound( box->top ),
        FPRound( box->right ),
        FPRound( box->bottom ) );
}

//-----------------------------------------------------------------------------
// 
geometric_box_t PixelToGeometricBox( const pixel_box_t *box, int32_t fracBits )
{
    return CreateGeometricBox(
        CreateFPInt( box->left, fracBits ),
        CreateFPInt( box->top, fracBits ),
        CreateFPInt( box->right, fracBits ),
        CreateFPInt( box->bottom, fracBits ) );
}


//-----------------------------------------------------------------------------
// 
fp_t AngleWithOrdinate( const geometric_vector_2d_t *vector )
{
    fp_t zero = CreateFPInt( 0, vector->x.fracBits );
    assert( FPNe( vector->x, zero ) || FPNe( vector->y, zero ),
        EC_VECTOR_NORM_IS_ZERO,
        "AgnleWithOrdinate: vector norm is 0.\r\n" );

    if ( FPEq( vector->x, zero ) && FPEq( vector->y, zero ) )
    {
        return zero;
    }
    
    return FPSub(
        FPAtan2( vector->y, vector->x),
        HALF_PI );
}

//-----------------------------------------------------------------------------
//
pixel_box_t ScaleBox(
    const geometric_box_t *box,
    fp_t boxScale )
{
    fp_t frameScale = 
        FPDiv(
            FPSub( boxScale, CreateFPInt( 1, boxScale.fracBits ) ),
            CreateFPInt( 2, boxScale.fracBits )
            );
    
	fp_t frameX = FPMul( GetGeometricBoxWidth(box), frameScale );
	fp_t frameY = FPMul( GetGeometricBoxHeight(box), frameScale );
    
    // New box. The centers of the original box and the new box are the same.
    pixel_box_t roi = CreatePixelBox(
        FPRound( FPSub( box->left, frameX ) ),
        FPRound( FPSub( box->top, frameY ) ),
        FPRound( FPAdd( box->right, frameX ) ),
        FPRound( FPAdd( box->bottom, frameY ) ) );

    return roi;
}

//-----------------------------------------------------------------------------
//
pixel_box_t SquareAndScaleBox(    
    const geometric_box_t *box,
    fp_t boxScale )
{
    fp_t frameScale = 
    FPDiv(
        FPSub( boxScale, CreateFPInt( 1, boxScale.fracBits ) ),
        CreateFPInt( 2, boxScale.fracBits )
        );
    
	fp_t width = GetGeometricBoxWidth(box);
	fp_t height = GetGeometricBoxHeight(box);
	
	fp_t maxDim = FPGt(width, height) ? width : height;
	fp_t frame = FPMul( maxDim, frameScale );

	fp_t widthOffset = FPAdd(FPRShift(FPSub(maxDim, width), 1), frame);
	fp_t heightOffset = FPAdd(FPRShift(FPSub(maxDim, height), 1), frame);    

    // New box. The centers of the original box and the new box are the same.
    pixel_box_t roi = CreatePixelBox(
        FPRound( FPSub( box->left, widthOffset ) ),
        FPRound( FPSub( box->top, heightOffset ) ),
        FPRound( FPAdd( box->right, widthOffset ) ),
        FPRound( FPAdd( box->bottom, heightOffset ) ) );

    return roi;
}

typedef struct
{
    int32_t boxA;
    int32_t boxB;
} box_pair_t;

void Match(const geometric_box_t *list1,
           int32_t sizeList1,
           const geometric_box_t *list2,
           int32_t sizeList2,
           box_matcher_t matcher,
           int32_t matchThreshold,
           int32_t *assignment)

{

    int32_t numberOfMatches = sizeList1 * sizeList2;
    box_pair_t matches[numberOfMatches];
    int16_t matchValues[numberOfMatches];
    size_t indices[numberOfMatches];

    bool usedBoxA[sizeList1];
    bool usedBoxB[sizeList2];

    // Make sure assignment[0] is -1, if sizeList1 is 0 then none of the initialization happens
    assignment[0] = -1;

    // Get match values for each box pair
    // (and set lists to 0)
    int32_t index = 0;
    for (int32_t i = 0; i < sizeList1; ++i)
    {
        usedBoxA[i] = false;
        assignment[i] = -1;
        for (int32_t j = 0; j < sizeList2; ++j)
        {
            if (i == 0)
            {
                usedBoxB[j] = false;
            }
            indices[index] = index;
            matches[index] = (box_pair_t){
                .boxA = i, .boxB = j};
            matchValues[index] = matcher(&list1[i], &list2[j]);
            ++index;
        }
    }
    // Sort pairs by match value
    QuickSort(indices, matchValues, numberOfMatches);

    // Iterate and assign best matches
    for (int32_t i = 0; i < numberOfMatches; ++i)
    {
        // Check if remaining matches exceed the threshold
        if (matchValues[indices[i]] > matchThreshold)
        {
            break;
        }
        // Check if we've assigned either box already
        if (usedBoxA[matches[indices[i]].boxA] == false &&
            usedBoxB[matches[indices[i]].boxB] == false)
        {
            // Assign best match and mark the boxes as used
            assignment[matches[indices[i]].boxA] = matches[indices[i]].boxB;
            usedBoxA[matches[indices[i]].boxA] = true;
            usedBoxB[matches[indices[i]].boxB] = true;
        }
    }
}
