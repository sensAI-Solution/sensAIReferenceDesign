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

#include "landmarks.h"

#include "app_assert.h"
#include "errors.h"
#include "ml_engine_config.h"

//=============================================================================
// F U N C T I O N S   C O D E   S E C T I O N

//-----------------------------------------------------------------------------
//
geometric_point_2d_t LandmarksMean2dFaceFit(
	const landmarks_2d_facefit_t *landmarks,	// landmarks
	const uint8_t *indices,				// indices of the relevant landmarks
	size_t indicesNb )					// number of indices
{
	geometric_point_2d_t mean = GetLandmark2dFaceFit( landmarks, indices[0] );

	for( size_t i = 1; i < indicesNb; ++i )
	{
		geometric_point_2d_t landmark = GetLandmark2dFaceFit( landmarks, indices[i] );
		mean.x = FPAdd( mean.x, landmark.x );
		mean.y = FPAdd( mean.y, landmark.y );
	}

	mean.x.n /= (int) indicesNb;
	mean.y.n /= (int) indicesNb;
	return mean;
}

//-----------------------------------------------------------------------------
//
geometric_point_2d_t LandmarksMean2dFaceDet(
	const landmarks_2d_facedet_t *landmarks, 	// landmarks
	const uint8_t *indices,             // indices of the relevant landmarks
	size_t indicesNb )                  // number of indices
{
	geometric_point_2d_t mean = GetLandmark2dFaceDet( landmarks, indices[0] );

	for( size_t i = 1; i < indicesNb; ++i )
	{
		geometric_point_2d_t landmark = GetLandmark2dFaceDet( landmarks, indices[i] );
		mean.x = FPAdd( mean.x, landmark.x );
		mean.y = FPAdd( mean.y, landmark.y );
	}

	mean.x.n /= (int) indicesNb;
	mean.y.n /= (int) indicesNb;
	return mean;
}

//-----------------------------------------------------------------------------
//
geometric_point_3d_t LandmarksMean3dFaceFit(
	const landmarks_3d_facefit_t *landmarks,	// landmarks
	const uint8_t *indices,				// indices of the relevant landmarks
	size_t indicesNb )					// number of indices
{
	geometric_point_3d_t mean = GetLandmark3dFaceFit( landmarks, indices[0] );

	for( size_t i = 1; i < indicesNb; ++i )
	{
		geometric_point_3d_t landmark = GetLandmark3dFaceFit( landmarks, indices[i] );
		mean.x = FPAdd( mean.x, landmark.x );
		mean.y = FPAdd( mean.y, landmark.y );
		mean.z = FPAdd( mean.z, landmark.z );
	}

	mean.x.n /= (int) indicesNb;
	mean.y.n /= (int) indicesNb;
	mean.z.n /= (int) indicesNb;
	return mean;
}

//-----------------------------------------------------------------------------
//
geometric_point_3d_t LandmarksMean3dEyelid(
	const landmarks_3d_eyelid_t *landmarks,
	size_t indicesNb )
{
	fp_t zero = CreateFPInt( 0, ML_ENGINE_OUTPUT_FRAC_BITS );    
    geometric_point_3d_t mean = CreateGeometricPoint3d( zero, zero, zero );

	for( size_t i = 0; i < indicesNb; ++i )
	{
		geometric_point_3d_t landmark = GetLandmark3dEyelid( landmarks, i );
		mean.x = FPAdd( mean.x, landmark.x );
		mean.y = FPAdd( mean.y, landmark.y );
		mean.z = FPAdd( mean.z, landmark.z );
	}

	mean.x.n /= (int) indicesNb;
	mean.y.n /= (int) indicesNb;
	mean.z.n /= (int) indicesNb;
	return mean;	
}

//-----------------------------------------------------------------------------
//
void PopulateLandmarks2d( landmarks_2d_facefit_t *landmarks, size_t landmarksNb,
	landmarks_2d_facedet_t *fillLandmarks, const uint8_t *indices,
	size_t fillLandmarksNb, geometric_point_2d_t zeroValue )
{
	// TODO: Perhaps this could be more optimized.

	// First, fill all values
	for( size_t l = 0; l < landmarksNb; ++l )
	{
		SetLandmark2dFaceFit( landmarks, l, zeroValue );
	}

	// Then, fill the indices of interest
	for( size_t l = 0; l < fillLandmarksNb; ++l )
	{
		SetLandmark2dFaceFit( landmarks, indices[l], GetLandmark2dFaceDet( fillLandmarks, l ) );
	}
}

//-----------------------------------------------------------------------------
//
void TranslateLandmarks2dFaceFit(
	landmarks_2d_facefit_t *landmarks, size_t pointsNb, geometric_vector_2d_t vector )
{
	for( size_t i = 0; i < pointsNb; ++i )
	{
		geometric_point_2d_t landmark = GetLandmark2dFaceFit( landmarks, i);
		landmark = TranslateGeometricPoint2d( landmark, vector );
		SetLandmark2dFaceFit( landmarks, i, landmark );
	}
}

//-----------------------------------------------------------------------------
//
void TranslateLandmarks3dFaceFit(
	landmarks_3d_facefit_t *landmarks, size_t pointsNb, geometric_vector_3d_t vector )
{
	for( size_t i = 0; i < pointsNb; ++i )
	{
		geometric_point_3d_t landmark = GetLandmark3dFaceFit( landmarks, i );
		landmark = TranslateGeometricPoint3d( landmark, vector );
		SetLandmark3dFaceFit( landmarks, i, landmark );
	}
}

//----------------------------------------------------------------------------
//
geometric_box_t BoxFromLandmarks(
	const landmarks_2d_facefit_t *landmarks,
	size_t landmarksNb)
{
	assert( landmarksNb > 0, EC_NEGATIVE_OR_ZERO,
	    "landmarks nb. is less or equal to 0\r\n" );
	
	geometric_point_2d_t landmark = GetLandmark2dFaceFit( landmarks, 0);

	fp_t left = landmark.x;
	fp_t top = landmark.y;
	fp_t right = landmark.x;
	fp_t bottom = landmark.y;

	for( size_t i = 1; i < landmarksNb; ++i )
	{
		landmark = GetLandmark2dFaceFit( landmarks, i );
		left = FPLt( left, landmark.x ) ?
			left : landmark.x;
		right = FPLt( right, landmark.x ) ?
			landmark.x : right;
		top = FPLt( top, landmark.y ) ?
			top : landmark.y;
		bottom = FPLt( bottom, landmark.y ) ?
			landmark.y : bottom;
	}
	geometric_box_t bbox = CreateGeometricBox( left, top, right, bottom );
	return bbox;
}

//----------------------------------------------------------------------------
//
geometric_box_t BoxFromSubLandmarks(
	const uint8_t indices[],
	const landmarks_2d_facefit_t *landmarks,
	size_t indicesNb )
{
	assert( indicesNb > 0, EC_NEGATIVE_OR_ZERO,
		"Indices nb. is less or equal to 0\r\n" );
	
	geometric_point_2d_t landmark = GetLandmark2dFaceFit( landmarks, indices[0] );

	fp_t left = landmark.x;
	fp_t top = landmark.y;
	fp_t right = landmark.x;
	fp_t bottom = landmark.y;

	for( size_t i = 0; i < indicesNb; ++i )
	{
		landmark = GetLandmark2dFaceFit( landmarks, indices[i] );
		left = FPLt( left, landmark.x ) ?
			left : landmark.x;
		top = FPLt( top, landmark.y ) ?
			top : landmark.y;
		right = FPLt( right, landmark.x ) ?
			landmark.x : right;
		bottom = FPLt( bottom, landmark.y ) ?
			landmark.y : bottom;
	}
	geometric_box_t bbox = CreateGeometricBox( left, top, right, bottom );
	return bbox;
}

//-----------------------------------------------------------------------------
//
void DebugOutputLandmarks2D( landmarks_2d_facefit_t *landmarks, size_t landmarksNb )
{
	// DebugOutput( "===== Landmarks 2D in int =====\r\n" );
	// for( uint8_t i = 0; i < landmarksNb; ++i )
	// {
	// 	DebugOutput( "%u) x: %d  y: %d \r\n",
	// 		i,
	// 		FPToInt32( InterpretIntAsFP( landmarks->x[i], landmarks->fracBits ) ),
	// 		FPToInt32( InterpretIntAsFP( landmarks->y[i], landmarks->fracBits ) ) );
	// }
	// DebugOutput( "===============================\r\n" );
}

//-----------------------------------------------------------------------------
//
void DebugOutputLandmarks3D( landmarks_3d_facefit_t *landmarks, size_t landmarksNb )
{
	// DebugOutput( "===== Landmarks 3D in int =====\r\n" );
	// for( uint8_t i = 0; i < landmarksNb; ++i )
	// {
	// 	DebugOutput( "%u) x: %d  y: %d z: %d \r\n",
	// 		i,
	// 		FPToInt32( InterpretIntAsFP( landmarks->x[i], landmarks->fracBits ) ),
	// 		FPToInt32( InterpretIntAsFP( landmarks->y[i], landmarks->fracBits ) ),
	// 		FPToInt32( InterpretIntAsFP( landmarks->z[i], landmarks->fracBits ) ) );
	// }
	// DebugOutput( "===============================\r\n" );
}