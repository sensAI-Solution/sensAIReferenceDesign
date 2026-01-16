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

#include "ideal_user.h"

#include "app_assert.h"
#include "ml_engine_config.h"

//=============================================================================
// F U N C T I O N S   C O D E   S E C T I O N

//-----------------------------------------------------------------------------
//
geometric_box_t NormalizeBoxToCommonScale( geometric_box_t box,
	fp_dim_t originalDims, const fp_t zeroValue )
{
    if( FPGe( originalDims.width, zeroValue ) )
    {
		fp_dim_t halfOriginalDims = {
            .width = FPRShift( originalDims.width, 1 ),
            .height = FPRShift( originalDims.height, 1 ),
        };

        geometric_point_2d_t center = {
			.x = FPAdd( box.left, FPRShift( GetGeometricBoxWidth(&box), 1 ) ),
			.y = FPAdd( box.top,  FPRShift( GetGeometricBoxHeight(&box), 1 ) )
        };

		geometric_box_t normBox = CreateGeometricBox(
			FPSub( center.x, halfOriginalDims.width ),
			FPSub( center.y, halfOriginalDims.height ),
			FPAdd( center.x, halfOriginalDims.width ),
			FPAdd( center.y, halfOriginalDims.height ) );
		
		return normBox;
	}
	else
	{
	    return box;
	}
}

//-----------------------------------------------------------------------------
//
void ResetIdealUser( ideal_user_manager_t *manager )
{
	manager->idealUserInSavedBoxesIdx = -1;
	manager->idealUserChangedSinceLastQuery = true;
	manager->idealUserAvailable = false;
	manager->detectionBoxSize = (fp_dim_t) {
		.width = manager->MINUS_ONE,
		.height = manager->MINUS_ONE
	};
	manager->idealUserBox = CreateGeometricBox( manager->ZERO_VALUE,
		manager->ZERO_VALUE, manager->ZERO_VALUE, manager->ZERO_VALUE );
}

//-----------------------------------------------------------------------------
ideal_user_manager_t CreateIdealUserManager(
	fp_t iouThreshold,
	fp_t similarityRatio,
	fp_t idealUserPrefFactor,
	int32_dim_t sourceImageDim )
{
	ideal_user_manager_t manager = {
		.maxBoxesNb = FACE_BOXES_MANAGER_MAX_BOXES_NB,
		.savedBoxesNb = 0,
		.iouThreshold = iouThreshold,
		.similarityRatio = similarityRatio,
		.idealUserPrefFactor = idealUserPrefFactor,
		.sourceImageDim = sourceImageDim,
		.MINUS_ONE = CreateFPInt( -1, iouThreshold.fracBits ),
		.ZERO_VALUE = CreateFPInt( 0, iouThreshold.fracBits ) };

	ResetIdealUser( &manager );
	return manager;
}

//-----------------------------------------------------------------------------
//
bool IdealUserExists( const ideal_user_manager_t *manager )
{
	return manager->idealUserAvailable;
}

//-----------------------------------------------------------------------------
//
geometric_box_t GetIdealFaceBox( const ideal_user_manager_t *manager )
{
	return manager->idealUserBox;
}

//-----------------------------------------------------------------------------
//
void UpdateIdealFaceBox( ideal_user_manager_t *manager, geometric_box_t box )
{
	if( manager->idealUserAvailable )
	{
		manager->idealUserBox = box;
	}
}

//-----------------------------------------------------------------------------
// Search for a geometric_box_t in boxes that match the geometric_box_t 
// boxToMatch.
// A box is a match if its intersection over union with boxToMatch is greater
// than the threshold.
// The function returns the index of the best match (greater intersection over
// union) or -1 if no match is found.
int MatchBox(
	const geometric_box_t *boxes, // List of candidate boxes
	int32_t boxesNb,              // Number of candidate boxes
	geometric_box_t boxToMatch,   // The box to match
	fp_t threshold )              // IoU value beyond which a candidate is kept
{
	fp_t bestIoU = CreateFP( 0, 1, boxToMatch.left.fracBits );
	int bestCandidateIdx = -1;
	for( int32_t i = 0; i < boxesNb; ++i )
	{
		fp_t iou = ComputeGeometricIoU( boxToMatch, boxes[i] );
		if( FPGt( iou, bestIoU ) && FPGt( iou, threshold ) )
		{
			bestIoU = iou;
			bestCandidateIdx = i;
		}
	}
	return bestCandidateIdx;
}

//-----------------------------------------------------------------------------
// Select a box among detected that is likely to be the more relevant user in
// image. The function returns the index of the ideal user or size if none is
// found.
// It is assumed that the boxes array is of length size.
// It is assumed that sourceImageDim contains values greater than zero.
size_t SelectIdealUser( 
    const geometric_box_t *boxes, size_t size,
    size_t currentIdealUserIdx,
    fp_t idealUserPreferenceCoef,
    fp_t areaRatioThreshold,
    const geometric_point_2d_t *focusPoint )
{
	if( size == 0 )
    {
        return size;
    }
    
    // if we have a previous ideal user id, we start with it as reference    
    size_t idealIdx = currentIdealUserIdx < size ? currentIdealUserIdx : 0;
    fp_t idealArea = ComputeGeometricArea( boxes[idealIdx] );
    geometric_point_2d_t currentCenter =
        GetGeometricBoxCenter( &boxes[idealIdx] );
    fp_t idealDistToCenter = VectorNorm2d(
        CreateGeometricVector2d( currentCenter, *focusPoint ) );        

    for( size_t i = 0; i < size ; ++i )
    {
        if ( idealIdx == i )
        {
            continue;
        }
        else
        {
            fp_t currentArea = ComputeGeometricArea( boxes[i] );
            geometric_point_2d_t currentCenter = GetGeometricBoxCenter( &boxes[i] );
            fp_t currentDistToCenter = VectorNorm2d(
                CreateGeometricVector2d( currentCenter, *focusPoint ) );
            
            // these values are scaled to favor against the new candidate
            fp_t idealAreaFavored = 
                FPMul(idealArea, idealUserPreferenceCoef);
            fp_t idealDistToCenterFavored =
                FPDiv( idealDistToCenter, idealUserPreferenceCoef) ;
            fp_t currentAreaToIdealRatio = FPDiv(
                FPSub( currentArea, idealAreaFavored ), idealAreaFavored );
            bool areaSimilar = FPLt(
                FPAbs( currentAreaToIdealRatio ), areaRatioThreshold );
            bool closestToIdeal =
                FPLt( currentDistToCenter, idealDistToCenterFavored );
            bool currentAreaIsBigger =
                FPGt( currentAreaToIdealRatio, areaRatioThreshold );
            bool betterCandidate =
                ( areaSimilar && closestToIdeal ) || currentAreaIsBigger;
            if ( betterCandidate)
            {
                idealIdx = i;
                idealArea = currentArea;
                idealDistToCenter = currentDistToCenter;
            }
        }
    }
    return idealIdx;
}

//-----------------------------------------------------------------------------
// Checks if the boxes at indices user1 and user2 in arrays boxes1 and boxes2
// bound or not the same user.
// If the IoU of the two boxes is greater than threshold, the functions returns
// true, otherwise it returns false.
// If user1 or user2 is negative, the functions returns false.
// It is assumed user1 and user2 values are lessert than the size of array1 and
// array2.
bool AreSameUser(
	const geometric_box_t *box1, // Box of user 1
	const geometric_box_t *box2, // Box of user 2
	fp_t threshold )             // Value over which the two users are
	                             // said to be identical
{
    fp_t iou = ComputeGeometricIoU( *box1, *box2 );
    return FPGe( iou, threshold );
}

//-----------------------------------------------------------------------------
// Merges the ideal user box with the current detected boxes, considering the
// det-to-lnd scaling
int MergeBoxes( ideal_user_manager_t *manager, geometric_box_t *boxes,
	int *idealIdx )
{
	assert( boxes != NULL, EC_VALUE_NULL, "Boxes is null, cannot proceed!\r\n" );
	int boxCount = manager->savedBoxesNb;
	*idealIdx = -1;

	for( int32_t i = 0; i < boxCount; ++i )
	{
		boxes[i] = manager->savedBoxes[i];
	}
	// To merge, we need to have an ideal user already
	if( manager->idealUserAvailable )
	{

		// previous box sized might be smaller (replaced with landmarks box)
		// this will scale it to the original (larger) detected size
		// keeping the smaller size would lower the likelihood of selection
		geometric_box_t scaledBox = NormalizeBoxToCommonScale(
			manager->idealUserBox,
			manager->detectionBoxSize,
			manager->ZERO_VALUE );
		
		// try to determine which detected user is the same as prev ideal user
		// There's always space in boxes since it's 1 larger than face detection cap
		// We will either overwrite a detection it matches or add the ideal to
		// the list
		int matchIdx = MatchBox( boxes, boxCount, scaledBox, manager->iouThreshold );
		if( matchIdx >= 0 )
		{
			*idealIdx = matchIdx;
		}
		else
		{
			*idealIdx = boxCount;
			boxCount++;
			boxes[*idealIdx] = scaledBox;
		}
	}
	return boxCount;
}

//-----------------------------------------------------------------------------
// Updates the ideal user for this time step.
// The ideal user may not change or there may be no new ideal user.
// It is assumed the boxes array size is boxesNb.
void UpdateIdealUser( ideal_user_manager_t *manager )
{
    // Create boxes normalized to a common scale for evaluation purposes
	// The array can contain one element more than the number of detected users
	// since the ideal user may not have been detected during latest face detection.
	// In that case, it is added to the array.
    geometric_box_t mergedBoxes[FACE_BOXES_MANAGER_MAX_BOXES_NB + 1];
    int oldIdealUserIdx;
	// Find if the current ideal user is in the detected face boxes
    int boxCount = MergeBoxes( manager, mergedBoxes, &oldIdealUserIdx );

	// Find the new ideal user
    geometric_box_t imageBox = CreateGeometricBox(
        CreateFPInt( 0, ML_ENGINE_OUTPUT_FRAC_BITS ),
        CreateFPInt( 0, ML_ENGINE_OUTPUT_FRAC_BITS ),
        CreateFPInt( manager->sourceImageDim.width, ML_ENGINE_OUTPUT_FRAC_BITS ),
        CreateFPInt( manager->sourceImageDim.height, ML_ENGINE_OUTPUT_FRAC_BITS ) );
    
    geometric_point_2d_t imageCenter = GetGeometricBoxCenter( &imageBox );
    
	int32_t newIdealUserIdx = SelectIdealUser(
		mergedBoxes, boxCount, oldIdealUserIdx,
		manager->idealUserPrefFactor, manager->similarityRatio,
        &imageCenter );

	// Determine if the ideal user changed
    if( oldIdealUserIdx < boxCount && newIdealUserIdx < boxCount )
    {
        // The new ideal user does not match our saved one -> user changed
        manager->idealUserChangedSinceLastQuery = oldIdealUserIdx != newIdealUserIdx;
    }
    else if( oldIdealUserIdx == boxCount && newIdealUserIdx == boxCount )
    {
        // No old user, no new user -> user did not change
        manager->idealUserChangedSinceLastQuery = false;
    }
    else
    {
        // Either we dropped or just detected an ideal user
        manager->idealUserChangedSinceLastQuery = true;
    }

	// If the ideal user changed, update its data
	if( manager->idealUserChangedSinceLastQuery )
	{
		if( newIdealUserIdx == boxCount )
		{
			ResetIdealUser( manager );
		}
		else
		{
			manager->idealUserBox = mergedBoxes[newIdealUserIdx];
		}
	}
	
	// we keep updating the detection size
	if( newIdealUserIdx < boxCount )
	{
		manager->detectionBoxSize = (fp_dim_t) {
			.width = GetGeometricBoxWidth( &mergedBoxes[newIdealUserIdx] ),
			.height = GetGeometricBoxHeight( &mergedBoxes[newIdealUserIdx] )
		};
	}

	manager->idealUserAvailable = newIdealUserIdx < boxCount;

	// There can be an ideal user that is not among the users that were detected
	// by the lastest face detection step. It would mean it was detected before 
	// the latest face detection and is still valid. 
	// In this situation, idealUserAvailable == true and newIdealUserIdx == -1.
    // TODO: We updated SelectIdealUser function to return the index of the ideal user 
    // or size instead of -1 if none is found, but we still use -1 here.
	manager->idealUserInSavedBoxesIdx = newIdealUserIdx < manager->savedBoxesNb ?
		newIdealUserIdx : -1;
}

//-----------------------------------------------------------------------------
// Stores a new list of candidates in the manager. The previous list is dropped.
// It is assumed the boxes array size is boxesNb.
// It is assumed landmarks is NULL or an array of size [boxesNb][NB_ROUGH_LANDMARKS]
// It is assumd angles is NULL or an array of size [boxesNb][NB_ANGLES]
// It is assumed the manager max storing capacity is greater than or equal to
// boxesNb.
void StoreCandidates(
	ideal_user_manager_t *manager, // The manager to update
	const geometric_box_t *boxes,  // The new boxes list
	int32_t boxesNb )
{
	assert(
		boxesNb <= manager->maxBoxesNb, EC_OUT_OF_BOUNDS,
		"Too many boxes: %d / %d\r\n",
		boxesNb, manager->maxBoxesNb );

	manager->savedBoxesNb = boxesNb;
    for( int32_t i = 0; i < boxesNb; ++i )
	{
		manager->savedBoxes[i] = boxes[i];
	}
}

//-----------------------------------------------------------------------------
//
void UpdateIdealUserManagerCandidates(
	ideal_user_manager_t *manager,
	geometric_box_t *boxes,
	int32_t boxesNb )
{
	StoreCandidates( manager, boxes, boxesNb );
}

//-----------------------------------------------------------------------------
//
bool IdealUserChangedSinceLastQuery( ideal_user_manager_t *manager )
{
	bool idealUserChangedSinceLastQuery = 
		manager->idealUserChangedSinceLastQuery;
	manager->idealUserChangedSinceLastQuery = false;
	return idealUserChangedSinceLastQuery;
}

//-----------------------------------------------------------------------------
//
void RemoveIdealUser( ideal_user_manager_t *manager )
{
	ResetIdealUser( manager );
}
