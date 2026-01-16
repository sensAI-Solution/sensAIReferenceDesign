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

#ifndef IDEAL_USER_H
#define IDEAL_USER_H

//=============================================================================
// I N C L U D E   F I L E S   A N D   F O R W A R D   D E C L A R A T I O N S

#include <stdint.h>

#include "scene_info.h"
#include "fixed_point.h"
#include "types.h"

//=============================================================================
// S T R U C T   D E C L A R A T I O N S

// Contains variables to store face boxes information in order to choose an
// ideal user out of them.
// The following is important:
// savedBoxes contains all the bounding boxes detected by the last face detection
// step. idealUserBox cotains the current ideal user bounding box. It is 
// possible the current ideal user is NOT in the latest detected users. That
// would happen if the ideal user is still validated by face validation but not
// detected anymore by face detection. 
// When the ideal user is detected by face detection, the ideal user bounding box
// stored in idealUserBox and the one stored in savedBoxes will most likely NOT
// be the same. This is by design. We want to keep the ideal user bounding box
// as it was detected by the face detection in savedBoxes and the one computed
// from the landmarks and the face validation offsets.
typedef struct
{
    // Saved detection data
	geometric_box_t savedBoxes[FACE_BOXES_MANAGER_MAX_BOXES_NB];

	int32_t maxBoxesNb;         // Max number of saved boxes
	int32_t savedBoxesNb;       // Number of currently saved boxes
	int32_dim_t sourceImageDim; // Dimensions of the source image
	fp_t iouThreshold;          // IoU value above which two boxes are said to
	                            // be the same
	fp_t similarityRatio;       // Ratio below which two areas are said similar
	fp_t idealUserPrefFactor;   // factor to favor already selected ideal user
	bool idealUserAvailable;	// Boolean indicating ideal user state
	geometric_box_t idealUserBox; // Independently holding the ideal user box
	fp_dim_t detectionBoxSize;	// store the original size from detection
	bool idealUserChangedSinceLastQuery; // Track changes to ideal user
	int32_t idealUserInSavedBoxesIdx; // If the ideal user is in detected users,
	                                 // its index. Kept for convenience.

	fp_t MINUS_ONE;       // Constant for convenience
	fp_t ZERO_VALUE;      // Constant for convenience
} ideal_user_manager_t;

//=============================================================================
// F U N C T I O N S   D E C L A R A T I O N S

// Populates an ideal_user_manager_t.
ideal_user_manager_t CreateIdealUserManager(
	fp_t detectionThreshold,      // IoU value above which two boxes are said
	                              // to be identical
	fp_t similarityRatio,         // Ratio below which two areas are said to be
	                              // similar
	fp_t idealUserPrefFactor,     // factor to favor already selected ideal user
	int32_dim_t sourceImageDim ); // Dimensions of the source image

// Process the candidates list and stores the processed list in the manager.
// If an ideal user was previously being tracked, it will keep it in the
// new list.
// It is assumed the array boxes points to has size boxesNb.
// It is assumed boxesNb is lesser than or equal to the manager max capacity.
void UpdateIdealUserManagerCandidates(
    ideal_user_manager_t *manager,  // Ideal user manager to be updated
    geometric_box_t *boxes,         // New boxes to save.
    int32_t boxesNb );              // Number of boxes to save

// Updates the ideal user for this time step.
// The ideal user may not change or there may be no new ideal user.
// It is assumed the boxes array size is boxesNb.
void UpdateIdealUser( ideal_user_manager_t *manager );

// Removes the face box associated with the current ideal user from the list
// of saved boxes
void RemoveIdealUser( ideal_user_manager_t *manager );

// If there's currently an ideal user, returns true. Else returns false.
bool IdealUserExists( const ideal_user_manager_t *manager );

// Replace the current ideal user face box by the one passed as an argument.
// Nothing happens if no ideal user exists.
void UpdateIdealFaceBox(
	ideal_user_manager_t *manager, // Ideal user manager be updated
	geometric_box_t box );         // New face box for ideal user

// Returns the face box associated with the current ideal user.
// It is assumed that before calling that function, IdealUserExists was called
// and returned true.
geometric_box_t GetIdealFaceBox( const ideal_user_manager_t *manager );

// If the ideal user has changed since the last time this function was called,
// returns true. Else returns false.
bool IdealUserChangedSinceLastQuery( ideal_user_manager_t *manager );

//-----------------------------------------------------------------------------
// Select a box among detected that is likely to be the more relevant user in
// image. The function returns the index of the ideal user or size if none is
// found.
// It is assumed that the boxes array is of length size.
// It is assumed that sourceImageDim contains values greater than zero.
// @param currentIdealUserIdx Index of currently selected ideal user. If index equals size, ideal user index is set to 0.
// @param idealUserPreferenceCoef Coefficient that determines preference level for currently selected ideal user over new detected ones.
//      - < 1.0: Less preference for currently selected user, more openness to new detected users
//      - = 1.0: Same preference for currently selected user and new detected users
//      - > 1.0: More preference for currently selected user, less openness to new detected users 
// @param areaRatioThreshold How much bigger new detected user's box should be compared to the currently selected ideal user's box.
//      - value * 100 - % of how much more new detected user's box should be bigger.
size_t SelectIdealUser( 
    const geometric_box_t *boxes, size_t size,
    size_t currentIdealUserIdx,
    fp_t idealUserPreferenceCoef,
    fp_t areaRatioThreshold,
    const geometric_point_2d_t *focusPoint );
#endif