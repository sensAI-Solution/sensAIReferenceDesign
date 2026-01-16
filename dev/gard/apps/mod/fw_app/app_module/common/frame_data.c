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

//=============================================================================
// I N C L U D E   F I L E S   A N D   F O R W A R D   D E C L A R A T I O N S

#include "frame_data.h"


//=============================================================================
// C O N S T R U C T O R (S) / D E S T R U C T O R   C O D E   S E C T I O N

//-----------------------------------------------------------------------------
//
frame_data_t InitFrameData( void )
{
    frame_data_t frameData;
    ResetFrameData( &frameData );

    /// Ideal User
    frameData.idealUser = InitGranularUser();

    /// Rough Users
    for( uint8_t i = 0; i < FACE_BOXES_MANAGER_MAX_BOXES_NB; ++i)
    {
        frameData.detectedUsers[i] = InitRoughUserInfo();
    }
    frameData.detectedUsersNb = 0;

    return frameData;
}


//=============================================================================
// F U N C T I O N S   C O D E   S E C T I O N

//-----------------------------------------------------------------------------
//
void ResetFrameData( frame_data_t *frameData )
{    
    // Reset FD results
    frameData->newFaceDetectionSuccessful = false;
    frameData->idealUserChanged = false;
    
    // Reset LM results
    frameData->newLandmarksReady = false;

    // Reset ELE results
    // frameData->eyesLandmarksUpdated = EYES_LANDMARKS_NOT_UPDATED;

    // Reset FV results
    frameData->fvCenter.valid = false;

    // Reset MD results
    frameData->motionType = MT_UNKNOWN;

    // Reset Drowsiness results
    // frameData->drowsinessResult.attentionState = AS_UNKNOWN;
    // frameData->drowsinessResult.resultReady = false;

    // Reset Data ready
    frameData->dataReady = false;
}
