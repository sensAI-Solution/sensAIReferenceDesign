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

#include "preprocessing/bounding_box.h"

//=============================================================================
// F U N C T I O N S   C O D E   S E C T I O N

//-----------------------------------------------------------------------------
//
pixel_box_t ComputeRoIFromBoundingBox(
    const geometric_box_t *box,
    int32_t referenceFrameThickness,
    const geometric_box_t *referenceBox )
{
    // Compute box / default box scale 
    fp_t widthScale = 
        FPDiv( GetGeometricBoxWidth(box), GetGeometricBoxWidth(referenceBox) );
    fp_t heightScale =
        FPDiv ( GetGeometricBoxHeight(box), GetGeometricBoxHeight(referenceBox) );
    
    // Keeping the largest ratio, since we want to add the same number of pixels
    // to the top, left, bottom and right sides.
    fp_t scale = FPGt( widthScale, heightScale ) ? widthScale : heightScale;
    
    // Number of pixels to be added
    fp_t scaledFrameThickness = 
        FPMul( scale, CreateFPInt( referenceFrameThickness, scale.fracBits ) );
    
    // New box. The centers of the original box and the new box are the same.
    pixel_box_t roi = CreatePixelBox(
        FPRound( FPSub( box->left, scaledFrameThickness ) ),
        FPRound( FPSub( box->top, scaledFrameThickness ) ),
        FPRound( FPAdd( box->right, scaledFrameThickness ) ),
        FPRound( FPAdd( box->bottom, scaledFrameThickness ) ) );
    return roi;
}
