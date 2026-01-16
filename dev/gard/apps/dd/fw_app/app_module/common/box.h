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

#ifndef BOX_H
#define BOX_H

//=============================================================================
// I N C L U D E   F I L E S   A N D   F O R W A R D   D E C L A R A T I O N S

#include <stdint.h>

#include "fixed_point.h"


//=============================================================================
// S T R U C T   D E C L A R A T I O N S

// 2D int32 dimensions, intended to represent dimensions of pixel objects.
typedef struct
{
	int32_t width;
	int32_t height;
} int32_dim_t;

// Create a literal int32_dim_t struct. This is useful to initialize static or
// global variables.
#define CreateLiteralInt32Dim( argWidth, argHeight ) \
{ \
    .width = argWidth, \
    .height = argHeight \
}

// 2D fixed point dimensions, intended to represent dimensions of geometric 
// objects.
// It is assumed that width and height use the same number of fractional bits.
// Avoid direct manipulation and prefer using functions to ensure integrity.
typedef struct
{
	fp_t width;
	fp_t height;
} fp_dim_t;

// Create a literal fp_dim_t struct. This is useful to initialize static or
// global variables. literalWidth and literalHeight have to be literal fp_t
// struct.
#define CreateLiteralFPDim( literalWidth, literalHeight ) \
{ \
    .width = literalWidth, \
    .height = literalHeight \
}

// Meant to represent boxes in math world.
// The box is assumed to be valid : left < right and top < bottom.
// It is assumed that the coordinates of the box all use the same number
// of fractional bits for their fixed point representation. 
// Avoid direct manipulation and prefer using functions to ensure integrity.
typedef struct
{
	fp_t left;
	fp_t top;
	fp_t right;
	fp_t bottom;
} geometric_box_t;

// Meant to represent boxes in pixel world.
// The box is assumed to be valid : left < right and top < bottom.
// Avoid direct manipulation and prefer using function to ensure integrity.
typedef struct
{
	int32_t left;
	int32_t top;
	int32_t right;
	int32_t bottom;
	int32_dim_t dimensions;
} pixel_box_t;

// Create a literal pixel_box_t struct. This is useful to initialize static or
// global variables.
#define CreateLiteralPixelBox( argLeft, argTop, argRight, argBottom ) \
{ \
    .left = argLeft, \
    .bottom = argBottom, \
    .right = argRight, \
    .top = argTop, \
    .dimensions = { \
        .width = argRight - argLeft, \
        .height = argBottom - argTop \
    } \
}

// Meant to represent a point in 2d geometric space.
// It is assumed that x and y use the same number of fractional bits.
// Avoid direct manipulation and prefer using function to ensure integrity.
typedef struct
{
	fp_t x;
	fp_t y;
} geometric_point_2d_t;

// Meant to represent a point in 3d geometric space.
// It is assumed that x, y and z use the same number of fractional bits.
typedef struct
{
	fp_t x;
	fp_t y;
	fp_t z;
} geometric_point_3d_t;

typedef struct box
{
	bool valid;
	geometric_point_2d_t point;
} validated_geometric_point_2d_t;

// Meant to represent a vector in 2D geometric space.
// It is assumed that x and y use the same number of fractional bits.
// Avoid direct manipulation and prefer using function to ensure integrity.
typedef struct
{
	fp_t x;
	fp_t y;
} geometric_vector_2d_t;

// Meant to represent a vector in 3D geometric space.
// It is assumed that x and y use the same number of fractional bits.
// Avoid direct manipulation and prefer using function to ensure integrity.
typedef struct
{
	fp_t x;
	fp_t y;
	fp_t z;
} geometric_vector_3d_t;

// Meant to represent a point in pixel space.
typedef struct
{
	int32_t x;
	int32_t y;
} pixel_point_t;

// Meant to represent a vector in pixel space.
typedef struct
{
	int32_t x;
	int32_t y;
} pixel_vector_t;


//=============================================================================
// F U N C T I O N S   D E C L A R A T I O N S

// Creates a geometric box. 
// It is assumed that left <= right and top <= bottom and all coordinates use
// the same number of fractional bits.
geometric_box_t CreateGeometricBox(
	fp_t left, fp_t top,       // left top point of the box
	fp_t right, fp_t bottom ); // right bottom point of the box

// Creates a pixel box.
// It is assumed that left <= right and top <= bottom.
pixel_box_t CreatePixelBox( 
	int32_t left, int32_t top,       // left top point of the box
	int32_t right, int32_t bottom ); // right bottom point of the box

// Creates a geometric point.
// it is assumed that x and y use the same number of fractional bits.
geometric_point_2d_t CreateGeometricPoint(
	fp_t x, fp_t y ); // Point coordinates

// Creates a 3d geometric point.
// it is assumed that x, y, and z use the same number of fractional bits.
geometric_point_3d_t CreateGeometricPoint3d(
	fp_t x, fp_t y, fp_t z ); // Point coordinates

// Creates a vector from src to dst.
// It is assumed src and dst use the same number of fractional bits.
// Fixed point representation of the resulting vector uses the same number of
// fractional bits as the points.
geometric_vector_2d_t CreateGeometricVector2d(
	geometric_point_2d_t src,  // Source point of the vector
	geometric_point_2d_t dst ); // Destination point of the vector

geometric_vector_3d_t CreateGeometricVector3d(
	geometric_point_3d_t src,  // Source point of the vector
	geometric_point_3d_t dst ); // Destination point of the vector

// Create a pixel_point_t struct.
pixel_point_t CreatePixelPoint(
	int32_t x, int32_t y ); // pixel coordinates

// Create a pixel_vector_t struct.
pixel_vector_t CreatePixelVector(
	int32_t x, int32_t y ); // vector coordinates

// Moves a point along a vector (2D)
geometric_point_2d_t TranslateGeometricPoint2d(
	geometric_point_2d_t point,     // Point to offset
	geometric_vector_2d_t vector ); // Offset

// Moves a point along a vector (3D)
geometric_point_3d_t TranslateGeometricPoint3d(
	geometric_point_3d_t point,     // Point to offset
	geometric_vector_3d_t vector ); // Offset

// Move each point in an array along a vector (2D)
// It is assumed the number of points in the array is equal to or lesser then
// pointsNb.
void TranslateGeometricPoints2d(
	geometric_point_2d_t *points,   // array of points to offset
	size_t pointsNb,             // number of points in the array
	geometric_vector_2d_t vector ); // offset

// Move each point in an array along a vector (3D)
// It is assumed the number of points in the array is equal to or lesser then
// pointsNb.
void TranslateGeometricPoints3d(
	geometric_point_3d_t *points,   // array of points to offset
	size_t pointsNb,             // number of points in the array
	geometric_vector_3d_t vector ); // offset

// Transforms coordinates of the box by scaling then offsetting.
// It is assumed that horizontalScale and verticalScale are > 0 and use the
// same number of fractional bits as box. It is assumed that offset uses the
// same number of fractional bits as box.
geometric_box_t ScaleAndOffsetBox(
	geometric_box_t box,         // Box to transform
	fp_t horizontalScale,        // Scale value for the horizontal coordinates
	fp_t verticalScale,          // Scale value for the vertical coordinates
	geometric_vector_2d_t offset ); // Vector to add to coordinates

// Transforms coordinates of the box by offsetting then scaling.
// It is assumed that horizontalScale and verticalScale are > 0 and use the
// same number of fractional bits as box. It is assumed that offset uses the
// same number of fractional bits as box.
geometric_box_t OffsetAndScaleBox(
	geometric_box_t box,          // Box to transform
	geometric_vector_2d_t offset, // Vector to add to coordinates
	fp_t horizontalScale,         // Scale value for the horizontal coordinates
	fp_t verticalScale );         // Scale value for the vertical coordinates

// Transforms coordinates of the point by scaling then offsetting.
// It is assumed that horizontalScale and verticalScale are > 0 and use the
// same number of fractional bits as box. It is assumed that offset uses the
// same number of fractional bits as box.
geometric_point_2d_t ScaleAndOffsetPoint(
	geometric_point_2d_t point,     // Point to transform
	fp_t horizontalScale,        // Scale value for the horizontal coordinates
	fp_t verticalScale,          // Scale value for the vertical coordinates
	geometric_vector_2d_t offset ); // Vector to add to coordinates

// Computes area of the box.
// Fixed point representation of the result uses the same number of fractional
// bits as the coordinates of the box.
fp_t ComputeGeometricArea(
	geometric_box_t box ); // Box to compute area from

// Returns the width of a geometric box (right - left).
fp_t GetGeometricBoxWidth(const geometric_box_t *box);

// Returns the height of a geometric box (bottom - top).
fp_t GetGeometricBoxHeight(const geometric_box_t *box);

// Computes intersection over union of two boxes.
// It is assumed the two boxes use the same number of fractional bits for their
// components.
// Fixed point representation of the result uses the same number of fractional
// bits as the coordinates of the box.
fp_t ComputeGeometricIoU(
	geometric_box_t box1,
	geometric_box_t box2 );

fp_t ComputeGeometricIoURef(
	geometric_box_t *box1,
	geometric_box_t *box2 );

// Crops the box coordinates so that they fit in the container.
// It is assumed that box and container use the same number of fraction bits.
// Fixed point representation of the resulting box uses the same number of
// fractional bits as the landmarks
geometric_box_t CropGeometricBox(
	geometric_box_t box,         // Box to crop
	geometric_box_t container ); // Box defining the values to crop to

// Computes the norm of a vector.
// The norm uses the same number of fractional bits as vector.
fp_t VectorNorm2d( geometric_vector_2d_t vector );

// Compute norm vector using 64 bits for temp fixed point representation to avoid overflow
fp_t VectorNorm2d64bits( geometric_vector_2d_t vector );

// Computes the norm of a vector.
// The norm uses the same number of fractional bits as vector.
fp_t VectorNorm3d( geometric_vector_3d_t vector );


// calculate the distance from center of a geometric box to idealPoint
fp_t BoxDistToIdealPoint2D(
	geometric_box_t box,
	geometric_point_2d_t idealPoint
);

//----------------------------------------------------------------------------
// Computes the center of a geometric box.
geometric_point_2d_t GetGeometricBoxCenter( const geometric_box_t *box );

//----------------------------------------------------------------------------
// Converts a geometric box to a pixel box. Uses FPRound to convert real number
// coordinates to integer.
pixel_box_t GeometricToPixelBox( const geometric_box_t *box );

//----------------------------------------------------------------------------
// Converts a pixel box to a geometric box.
geometric_box_t PixelToGeometricBox( const pixel_box_t *box, int32_t fracBits );

// Returns true if the point is inside the box.
bool PointInsideBox(geometric_point_2d_t point, geometric_box_t box);

//----------------------------------------------------------------------------
// In geometric boxes list candidates, find the box such that:
// - IoU( toMatch, box ) >= iouThreshold
// - for all other boxes in candidates,
//     IoU( toMatch, box ) >= IoU (toMatch, otherBox )
// This function returns the index of the matched box in the candidates array.
// If no match was found (because the len(array) == 0 or because the IoU
// threshold was not met for any candidate, the function returns candidatesNb.
size_t MatchGeometricBox(
    const geometric_box_t *toMatch,    // pointer to the box to match
    fp_t iouThreshold,                 // IoU threshold value
    const geometric_box_t *candidates, // array of candidates boxes
    size_t candidatesNb );             // size of candidates

//-----------------------------------------------------------------------------
// Compute the coordinates of the smallest rectangle enclosing all points.
// It is assumed the size of the points list is greater than 0.
geometric_box_t ComputeRectangleEnvelope( 
    const geometric_point_2d_t *points,
    size_t size );

//-----------------------------------------------------------------------------
// Compute angle in radians between the y-axis and a vector.
// It is assumed the vector norm is not zero.
// The function will return 0 if the vector norm is zero, even if it's not
// mathematically sound.
fp_t AngleWithOrdinate( const geometric_vector_2d_t *vector );


//-----------------------------------------------------------------------------
// Scale width and height of box using scaling factor
pixel_box_t ScaleBox(
    const geometric_box_t *box,
    fp_t boxScale );

//-----------------------------------------------------------------------------
// Make square box using max side of original box and scale
pixel_box_t SquareAndScaleBox(    
    const geometric_box_t *box,
    fp_t boxScale );
    

// Given two boxes, return their match value
typedef int16_t (*box_matcher_t)( const geometric_box_t *, const geometric_box_t *);

// Given a comparison function, return the best pairs between two lists of boxes
// such that if the best match of list1[i] is list2[j], assignment[i] = j.
// If no match is found, then assignment[i] = -1.
// It is assumed assignment points to an array at least as long as sizeList1
void Match(const geometric_box_t *list1,
           int32_t sizeList1,
           const geometric_box_t *list2,
           int32_t sizeList2,
           box_matcher_t matcher,
           int32_t matchThreshold,
           int32_t *assignment);
#endif
