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

#ifndef LANDMARKS_H
#define LANDMARKS_H

//=============================================================================
// I N C L U D E   F I L E S

#include "landmarks_config.h"
#include "box.h"

//=============================================================================
// M A C R O S

// Structs for 2D and 3D landmarks.
// It should be possible to cast 3D landmarks to 2D.
#define FP_LANDMARKS( nbLandmarks, suffix ) \
    typedef struct \
    { \
        uint8_t fracBits; \
		int32_t x[nbLandmarks]; \
		int32_t y[nbLandmarks]; \
    } landmarks_2d_##suffix##_t; \
	\
    typedef struct \
    { \
        uint8_t fracBits; \
		int32_t x[nbLandmarks]; \
		int32_t y[nbLandmarks]; \
		int32_t z[nbLandmarks]; \
    } landmarks_3d_##suffix##_t; \

FP_LANDMARKS( DEFAULT_LANDMARKS_NB, facefit ); // struct with all landmarks for face fitting
FP_LANDMARKS( ROUGH_LANDMARKS_NB, facedet ); // struct with a reduced number of landmarks for face detection
FP_LANDMARKS( EYELID_MIDDLE_INDICES_LEN, eyelid ); // struct with eyelid landmarks


//-----------------------------------------------------------------------------
// Computes the mean of multiple landmarks.
// It is assumed that the array indices points to is of size at least indicesNb.
// It is assumed that all values in the array indices points to are lesser than
// the size of the array landmarks points to.
geometric_point_2d_t LandmarksMean2dFaceFit(
	const landmarks_2d_facefit_t *landmarks,	// landmarks
	const uint8_t *indices,				// indices of the relevant landmarks
	size_t indicesNb );					// number of indices

geometric_point_2d_t LandmarksMean2dFaceDet(
	const landmarks_2d_facedet_t *landmarks,	// landmarks
	const uint8_t *indices,				// indices of the relevant landmarks
	size_t indicesNb );					// number of indices

// Same as LandmarksMean, but with 3d points.
geometric_point_3d_t LandmarksMean3dFaceFit(
	const landmarks_3d_facefit_t *landmarks,	// landmarks
	const uint8_t *indices,				// indices of the relevant landmarks
	size_t indicesNb );					// number of indices

geometric_point_3d_t LandmarksMean3dEyelid(
	const landmarks_3d_eyelid_t *landmarks,		// landmarks
	size_t indicesNb );							// number of indices

// Fills in a provided landmarks array with zeroValue, except for the provided
// fillLandmarks (which are set at the indices provided by indices).
// Basically, a helper to allow populating a larger array of landmarks with a
// particular subset.
void PopulateLandmarks2d(
	landmarks_2d_facefit_t *landmarks,		// landmarks to populate, of size landmarksNb
	size_t landmarksNb,					// number of total landmarks in array
	landmarks_2d_facedet_t *fillLandmarks,	// landmarks to fill, of size fillLandmarksNb
	const uint8_t *indices,				// indices of relevant landmarks,  of size fillLandmarksNb
	size_t fillLandmarksNb,				// number of indices
	geometric_point_2d_t zeroValue );	// 'zero' value to set the other landmarks

void TranslateLandmarks2dFaceFit(
	landmarks_2d_facefit_t *landmarks,
	size_t pointsNb,
	geometric_vector_2d_t vector );

void TranslateLandmarks3dFaceFit(
	landmarks_3d_facefit_t *landmarks,
	size_t pointsNb,
	geometric_vector_3d_t vector );

// Computes the bounding box of a landmarks list.
// Fixed point representation of the resulting box uses the same number of
// fractional bits as the landmarks.
// It is assumed that all landmarks use the same number of fractional bits.
geometric_box_t BoxFromLandmarks(
	const landmarks_2d_facefit_t *landmarks, // Landmarks to enclose in the box
	size_t landmarksNb);                 // Number of landmarks

// Computes the bounding box of some landmarks in a list.
// It is assumed all landmarks use the same number of fractional bits.
// Fixed point representation of the resulting box uses the same number of
// fractional bits as the landmarks.
// It is assumed that all landmarks use the same number of fractional bits.
geometric_box_t BoxFromSubLandmarks(
	const uint8_t indices[],              // Indices of landmarks to enclose
	const landmarks_2d_facefit_t *landmarks, // List of landmarks
	size_t indicesNb);                   // Number of landmarks to enclose

void DebugOutputLandmarks2D( landmarks_2d_facefit_t *landmarks, size_t landmarksNb );

void DebugOutputLandmarks3D( landmarks_3d_facefit_t *landmarks, size_t landmarksNb );


//=============================================================================
// I N L I N E   F U N C T I O N S   C O D E   S E C T I O N

static inline void SetLandmark2dFaceFit( landmarks_2d_facefit_t *landmarks, size_t i, geometric_point_2d_t landmark )
{
	// TODO: assert fracBits
	landmarks->x[i] = landmark.x.n;
	landmarks->y[i] = landmark.y.n;
	
	// TODO: this is so bad. Change it.
	landmarks->fracBits = landmark.x.fracBits;
}

static inline geometric_point_2d_t GetLandmark2dFaceFit(
	const landmarks_2d_facefit_t *landmarks, size_t i )
{
	geometric_point_2d_t landmark = {
		.x = InterpretIntAsFP( landmarks->x[i], landmarks->fracBits ),
		.y = InterpretIntAsFP( landmarks->y[i], landmarks->fracBits )
	};
	return landmark;
}

static inline void SetLandmark2dFaceDet( landmarks_2d_facedet_t *landmarks, size_t i, geometric_point_2d_t landmark )
{
	// TODO: assert fracbits
	landmarks->x[i] = landmark.x.n;
	landmarks->y[i] = landmark.y.n;
	// TODO: bad, change that
	landmarks->fracBits = landmark.x.fracBits;
}

static inline geometric_point_2d_t GetLandmark2dFaceDet(
	const landmarks_2d_facedet_t *landmarks, size_t i )
{
	geometric_point_2d_t landmark = {
		.x = InterpretIntAsFP( landmarks->x[i], landmarks->fracBits ),
		.y = InterpretIntAsFP( landmarks->y[i], landmarks->fracBits )
	};
	return landmark;
}

static inline void CopyLandmarks2dFaceDet( 
	const landmarks_2d_facedet_t *src, 
	landmarks_2d_facedet_t *dst )
{
	if( src != NULL && dst != NULL )
	{
		for( int l = 0; l < ROUGH_LANDMARKS_NB; ++l )
		{
			SetLandmark2dFaceDet( dst, l, GetLandmark2dFaceDet( src, l ) );
		}
	}
}

static inline geometric_point_3d_t GetLandmark3dFaceFit( 
	const landmarks_3d_facefit_t *landmarks, size_t i )
{
	geometric_point_3d_t landmark = {
		.x = InterpretIntAsFP( landmarks->x[i], landmarks->fracBits ),
		.y = InterpretIntAsFP( landmarks->y[i], landmarks->fracBits ),
		.z = InterpretIntAsFP( landmarks->z[i], landmarks->fracBits ),
	};
	return landmark;
}

static inline void SetLandmark3dFaceFit( landmarks_3d_facefit_t *landmarks, size_t i, geometric_point_3d_t landmark )
{
	// TODO: add assert regarding fracBits
	landmarks->x[i] = landmark.x.n;
	landmarks->y[i] = landmark.y.n;
	landmarks->z[i] = landmark.z.n;
	// TODO: So bad, change it.
	landmarks->fracBits = landmark.x.fracBits;
}

//----------------------------------------------------------------------------
//
static inline geometric_point_3d_t GetLandmark3dEyelid( 
	const landmarks_3d_eyelid_t *landmarks, size_t i )
{
	geometric_point_3d_t landmark = {
		.x = InterpretIntAsFP( landmarks->x[i], landmarks->fracBits ),
		.y = InterpretIntAsFP( landmarks->y[i], landmarks->fracBits ),
		.z = InterpretIntAsFP( landmarks->z[i], landmarks->fracBits ),
	};
	return landmark;
}

//----------------------------------------------------------------------------
//
static inline void SetLandmark3dEyelid( landmarks_3d_eyelid_t *landmarks, uint8_t idx, geometric_point_3d_t landmark )
{
	// TODO: add assert regarding fracBits
	landmarks->x[idx] = landmark.x.n;
	landmarks->y[idx] = landmark.y.n;
	landmarks->z[idx] = landmark.z.n;
	// TODO: So bad, change it.
	landmarks->fracBits = landmark.x.fracBits;
}

#endif