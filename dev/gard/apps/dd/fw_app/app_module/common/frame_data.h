//=============================================================================
//
// Copyright(c) 2023 Mirametrix Inc. All rights reserved.
//
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by MMX and
// are protected by copyright law. They may not be disclosed
// to third parties or copied or duplicated in any form, in whole or
// in part, without the prior written consent of MMX.
//
//=============================================================================

#ifndef FRAME_DATA_H
#define FRAME_DATA_H

//=============================================================================
// I N C L U D E   F I L E S   A N D   F O R W A R D   D E C L A R A T I O N S

#include "fixed_point.h"
#include "ideal_user.h"


//=============================================================================
// S T R U C T   D E C L A R A T I O N S

// The different types of motion to be detected
typedef enum
{
	MT_UNKNOWN = 0,
	MT_SMALL,
	MT_LARGE
} MotionType;

// Contains frame results
typedef struct
{
    // For optimization some data are shared between all networks and retained from previous frame
    fp_t noUserConfidence;
    granular_user_info_t idealUser;
    rough_user_info_t detectedUsers[FACE_BOXES_MANAGER_MAX_BOXES_NB];
    size_t detectedUsersNb;
 
    // Other data is specified for each network and recalculated every frame
    // FD results
    bool newFaceDetectionSuccessful;
    // - idealUserChanged: true if ideal user is changed since last ideal user selection. If user wasn't selected previously - false
    bool idealUserChanged;

    // LM results
    bool newLandmarksReady;
   
    // FV results
    validated_geometric_point_2d_t fvCenter;

    // MD results
    MotionType motionType;     // Type of last motion detected

   
    // Frame results
    bool dataReady;

    // frame per seconf
    fp_t fps;
} frame_data_t;


//=============================================================================
// F U N C T I O N S   D E C L A R A T I O N S

frame_data_t InitFrameData( void );

/**
* @brief Reset frame data
* 
* Reset frame data, which is recalculated every frame 
* and is not retained from the previous frame.
* @param frameData 
*/
void ResetFrameData( frame_data_t *frameData );

#endif