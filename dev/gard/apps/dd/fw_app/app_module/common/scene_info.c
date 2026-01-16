//=============================================================================
//
// Copyright(c) 2024 Mirametrix Inc. All rights reserved.
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

#include "scene_info.h"



//=============================================================================
// F U N C T I O N S   C O D E   S E C T I O N

//-----------------------------------------------------------------------------
//
static headpose_ccs_t InitHeadPoseCcs( void )
{
	headpose_ccs_t headPoseCcs;
	headPoseCcs.valid = false;
	return headPoseCcs;
}

//-----------------------------------------------------------------------------
//
static headpose_ics_t InitHeadPoseIcs( void )
{
	headpose_ics_t headPoseIcs;
	headPoseIcs.valid = false;
	return headPoseIcs;
}

//-----------------------------------------------------------------------------
//
granular_user_info_t InitGranularUser( void )
{
	granular_user_info_t granularUserInfo;
	granularUserInfo.landmarksAvailable = false;
	granularUserInfo.validity = UV_INVALID;
	granularUserInfo.headPoseCCS = InitHeadPoseCcs();
	granularUserInfo.headPoseICS = InitHeadPoseIcs();
	return granularUserInfo;
}

//-----------------------------------------------------------------------------
//
rough_user_info_t InitRoughUserInfo( void )
{
	rough_user_info_t roughUserInfo;
	roughUserInfo.ccsDataValid = false;
	return roughUserInfo;
}
