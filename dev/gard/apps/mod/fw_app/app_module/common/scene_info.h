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

#ifndef SCENE_INFO_H
#define SCENE_INFO_H

//=============================================================================
// I N C L U D E   F I L E S   A N D   F O R W A R D   D E C L A R A T I O N S

#include "angles.h"
#include "box.h"
#include "default_landmarks.h"


//=============================================================================
// E N U M S

typedef enum 
{
	UV_INVALID,
	UV_UNKNOWN,
	UV_VALID
} UserValidity;


//=============================================================================
// S T R U C T   D E C L A R A T I O N S

#define FACE_BOXES_MANAGER_MAX_BOXES_NB 10
#define ANGLES_NB 3

// Contains camera coordinate system-based head pose information
typedef struct
{
	bool valid; // Whether the provided data is valid (default false)
	euler_angles_t rotEuler; // Euler angles of the face
	geometric_point_3d_t translation; // Middle point of the face
} headpose_ccs_t;

// Contains image coordinate system-based head pose information
typedef struct
{
	bool valid; // Whether the provided data is valid (default false)
    fp_t scale; // Scale of the face (compared to default landmarks)
    euler_angles_t rotEuler; // Euler angles of the face
    fp_t rotAxisAngle[3];    // Rotation axis of the face
    geometric_vector_2d_t translation; // Middle point of the face
    landmarks_3d_facefit_t projectPoints3D; // landmarks with estimated depth
} headpose_ics_t;

// Holds our full user information, on which we have performed
// landmarks estimation and validation + tracking
typedef struct
{
	headpose_ccs_t headPoseCCS;
	headpose_ics_t headPoseICS;

	geometric_box_t roi;

	fp_t confidence;
	bool landmarksAvailable; // landmarks have been predicted 
                             // and not invalidated yet
	UserValidity validity;
} granular_user_info_t;

// Holds rough user information, obtained only from detector output
typedef struct
{
	geometric_box_t *roiPtr; // Will point to the data elsewhere
	landmarks_2d_facedet_t landmarks;
	euler_angles_t eulerAnglesICS;

	fp_t scale;

	// 3D estimated info
	bool ccsDataValid;
	euler_angles_t eulerAnglesCCS;
	geometric_point_3d_t faceCenter3D;

	// Confidence level
	fp_t confidence;
} rough_user_info_t;

//=============================================================================
// F U N C T I O N S   D E C L A R A T I O N S
granular_user_info_t InitGranularUser( void );

rough_user_info_t InitRoughUserInfo( void );


#endif