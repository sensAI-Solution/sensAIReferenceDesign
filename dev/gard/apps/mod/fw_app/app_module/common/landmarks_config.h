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

#ifndef LANDMARKS_CONFIG_H
#define LANDMARKS_CONFIG_H


//=============================================================================
// I N C L U D E   F I L E S   A N D   F O R W A R D   D E C L A R A T I O N S

#include <stdint.h>
#include <stddef.h>

#include "types.h"

//=============================================================================
// C O N S T A N T S

// Number of landmarks from face detection:
#define ROUGH_LANDMARKS_NB 5

#ifndef SMALL_NB_LANDMARKS_NN // 68 landmarks NN

#define DEFAULT_LANDMARKS_NB 68		// TODO: duplicated information, see ALL_INDICES_LEN
#ifdef FAST_FITTER
#define FITTER_LANDMARKS_NB 23
#define FITTER_LANDMARKS_NB_MUL_BY_2 46
#else
#define FITTER_LANDMARKS_NB 68
#define FITTER_LANDMARKS_NB_MUL_BY_2 136
#endif

static const size_t ALL_INDICES_LEN = 68;
static const uint8_t ALL_INDICES[] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
	10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
	20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
	30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
	40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
	50, 51, 52, 53, 54, 55, 56, 57, 58, 59,
	60, 61, 62, 63, 64, 65, 66, 67 };

static const bool WEIGHTED_LANDMARKS = false;
static const uint8_t LANDMARKS_WEIGHTS[DEFAULT_LANDMARKS_NB] = {0};

static const size_t CHIN_TO_NOSE_INDICES_LEN = 30;
static const uint8_t CHIN_TO_NOSE_INDICES[] = {
	3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 33, 48, 49, 50,
	51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65 };

static const size_t NOSE_INDICES_LEN = 11;
static const uint8_t NOSE_INDICES[] = {
	27, 28, 29, 30, 31, 32, 33, 34, 35, 39, 42 };

static const size_t LEFT_EYE_INDICES_LEN = 6;
static const uint8_t LEFT_EYE_INDICES[] = { 42, 43, 44, 45, 46, 47 };
static const uint8_t LEFT_EYE_INDICES_REVERSED[] = { 45, 44, 43, 42, 47, 46 };
// For now we are interested only in two middle eyelid landmarks
#define EYELID_MIDDLE_INDICES_LEN 2	
static const uint8_t LEFT_TOP_EYELID_INDICES[] = { 43, 44 };
static const uint8_t LEFT_BOTTOM_EYELID_INDICES[] = { 47, 46 };

static const size_t RIGHT_EYE_INDICES_LEN = 6;
static const uint8_t RIGHT_EYE_INDICES[] = { 36, 37, 38, 39, 40, 41 };
static const uint8_t RIGHT_TOP_EYELID_INDICES[] = { 37, 38 };
static const uint8_t RIGHT_BOTTOM_EYELID_INDICES[] = { 41, 40 };

static const size_t MOUTH_INDICES_LEN = 18;
static const uint8_t MOUTH_INDICES[] = {
	48, 49, 50, 51, 52, 53, 54, 55, 56,
	57, 58, 59, 60, 61, 62, 63, 64, 65 };

static const size_t NOSE_BRIDGE_INDICES_LEN = 1;
static const uint8_t NOSE_BRIDGE_INDICES[] = { 27 };

static const size_t NOSE_TIP_IDX = 30;

// Subset of landmarks used within headpose fitting to speed it up. 
static const size_t SPARSE_INDICE_LEN = 20;
static const uint8_t SPARSE_INDICES[] = {
	0, 3, 5, 11, 13, 16,
	17, 21, 22, 26,
	27, 30, 31, 35,
	36, 39, 42, 45,
	48, 54 };

static const size_t ROUGH_INDICES_LEN = 5;
// Note: this is the order that the detector outputs these
static const uint8_t ROUGH_INDICES[] = { 66, 67, 33, 48, 54 };

static const size_t ROUGH_NOSE_TIP_IDX = 33;
static const size_t LEFT_PUPIL_IDX = 67;
static const size_t RIGHT_PUPIL_IDX = 66;
static const uint8_t PUPILS_INDICES[] = { LEFT_PUPIL_IDX, RIGHT_PUPIL_IDX };
static const size_t PUPILS_INDICES_LEN = 2;
static const size_t ALL_INDICES_NO_PUPILS = ALL_INDICES_LEN - PUPILS_INDICES_LEN;


static const size_t MOUTH_CORNERS_INDICES_LEN = 2;
static const uint8_t MOUTH_CORNERS_INDICES[] = { 48, 54 };

static const size_t FAST_FITTER_LEN = 23;
static const uint8_t FAST_FITTER_INDICES[] = {
	0, 4, 8, 12, 16, 19, 24, 27, 29, 30,
	33, 37, 40, 44, 47, 48, 51, 54, 57, 61, 
	64, 66, 67 };

#else // 23 landmarks NN

#define DEFAULT_LANDMARKS_NB 23
#define FITTER_LANDMARKS_NB 23
#define FITTER_LANDMARKS_NB_MUL_BY_2 46

static const size_t ALL_INDICES_LEN = 23;
static const uint8_t ALL_INDICES[] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
	10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
	20, 21, 22 };

static const bool WEIGHTED_LANDMARKS = true;
static const uint8_t LANDMARKS_WEIGHTS[] = {
    0, 0, 1, 0, 0, 2, 2, 1, 3, 2,
    2, 3, 1, 3, 1, 3, 1, 3, 3, 1, 
    3, 2, 2 };

static const uint8_t MAPPING_23_TO_68_LANDMARKS[] = {
	0, 4, 8, 12, 16, 19, 24, 27, 29, 30,
	33, 37, 40, 44, 47, 48, 51, 54, 57, 61, 
	64, 66, 67 };
	
static const size_t LEFT_EYE_INDICES_LEN = 2;
static const uint8_t LEFT_EYE_INDICES[] = { 13, 14 };
static const uint8_t LEFT_EYE_INDICES_REVERSED[] = { 14, 13 };
#define EYELID_MIDDLE_INDICES_LEN 1
static const uint8_t LEFT_TOP_EYELID_INDICES[] = { 13 };
static const uint8_t LEFT_BOTTOM_EYELID_INDICES[] = { 14 };

static const size_t RIGHT_EYE_INDICES_LEN = 2;
static const uint8_t RIGHT_EYE_INDICES[] = { 11, 12 };
static const uint8_t RIGHT_TOP_EYELID_INDICES[] = { 11 };
static const uint8_t RIGHT_BOTTOM_EYELID_INDICES[] = { 12 };

static const size_t MOUTH_INDICES_LEN = 6;
static const uint8_t MOUTH_INDICES[] = {
	15, 16, 17, 18, 19, 20 };
	
static const size_t NOSE_BRIDGE_INDICES_LEN = 1;
static const uint8_t NOSE_BRIDGE_INDICES[] = { 7 };

static const size_t NOSE_TIP_IDX = 9;

static const size_t ROUGH_INDICES_LEN = 5;
// Note: this is the order that the detector outputs these
static const uint8_t ROUGH_INDICES[] = { 21, 22, 10, 15, 17 };

static const size_t ROUGH_NOSE_TIP_IDX = 10;
static const size_t LEFT_PUPIL_IDX = 22;
static const size_t RIGHT_PUPIL_IDX = 21;
static const uint8_t PUPILS_INDICES[] = { LEFT_PUPIL_IDX, RIGHT_PUPIL_IDX };
static const size_t PUPILS_INDICES_LEN = 2;

static const size_t MOUTH_CORNERS_INDICES_LEN = 2;
static const uint8_t MOUTH_CORNERS_INDICES[] = { 15, 17 };

#endif
 
#endif
