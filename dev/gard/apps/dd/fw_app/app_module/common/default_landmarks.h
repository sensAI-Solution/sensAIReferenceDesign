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

#ifndef DEFAULT_LANDMARKS_H
#define DEFAULT_LANDMARKS_H

//=============================================================================
// I N C L U D E   F I L E S   A N D   F O R W A R D   D E C L A R A T I O N S

#include "landmarks.h"


#ifndef SMALL_NB_LANDMARKS_NN

// All 68 landmarks:
// TODO: Implementation when landmarks estimation runs with 68 landmarks 
// and face fitting algo uses 23 langmarks (fast fitting) is broken
// since landmarks_3d_facefit_t struct (with 23 landmarks) is not enough to 
// store DEFAULT_LANDMARKS_3D with 68 landmarks.
static const landmarks_3d_facefit_t DEFAULT_LANDMARKS_3D = {
    .fracBits = 10,
    .x = { -18753, -18567, -18059, -17168, -15556, -12777, -9103, -4786,
	6, 4793, 9108, 12781, 15559, 17170, 18059, 18567,
	18753, -15017, -12704, -9888, -7010, -4251, 4250, 7009,
	9887, 12703, 15015, -3, -2, 0, 0, -3378,
	-1741, 0, 1740, 3377, -11273, -9433, -7168, -5339,
	-7272, -9403, 5331, 7159, 9425, 11268, 9398, 7267,
	-6786, -4889, -2570, -1, 2567, 4886, 6785, 5002,
	2669, 1, -2668, -5002, -2664, 0, 2664, 2682,
	0, -2683, -8248, 8286 },
    .y = { -7784, -2361, 3037, 8277, 12965, 16767, 19696, 21735,
	22191, 21734, 19694, 16765, 12962, 8274, 3035, -2363,
	-7786, -14702, -16345, -16979, -16760, -15954, -15954, -16760,
	-16980, -16347, -14705, -10726, -7723, -4738, -1762, 1392,
	1873, 2049, 1873, 1391, -10011, -10968, -11001, -10052,
	-9618, -9559, -10052, -11001, -10968, -10013, -9561, -9619,
	8532, 7021, 5938, 6142, 5938, 7019, 8531, 10230,
	11315, 11628, 11316, 10231, 7902, 7898, 7901, 8747,
	8969, 8747, -10291, -10292 },
    .z = { 23198, 21724, 20225, 17768, 13647, 8878, 4213, 97,
	-2910, 99, 4226, 8894, 13653, 17761, 20211, 21707,
	23176, 2344, 425, -1489, -3306, -4947, -4940, -3302,
	-1489, 425, 2343, -6384, -8639, -10918, -13098, -8009,
	-9137, -10067, -9136, -8008, 0, -755, -1168, -1326,
	-992, -612, -1301, -1153, -751, -11, -613, -983,
	-2679, -5246, -7152, -8523, -7160, -5259, -2692, -4629,
	-6329, -7509, -6330, -4627, -6128, -7853, -6133, -5916,
	-7554, -5914, -959, -932 }
 };
 
#else // SMALL_NB_LANDMARKS_NN

// Same landmarks as above, but only using a subset of 23 landmarks.
// Corresponding indices are: ( see image: FPGAFaceTracker\model\figure_68_23_markup.jpg )
// 	    [ 0, 4, 8, 12, 16, 19, 24, 27, 29, 30,
// 		33, 37, 40, 44, 47, 48, 51, 54, 57, 61, 
//		64, 66, 67 ]
static const landmarks_3d_facefit_t DEFAULT_LANDMARKS_3D = {
    .fracBits = 10,
    .x = { -18753, -15556, 6, 15559, 18753, -9888, 9887, -3,
	0, 0, 0, -9433, -7272, 9425, 7267, -6786,
	-1, 6785, 1, 0, 0, -8248, 8286 },
    .y = { -7784, 12965, 22191, 12962, -7786, -16979, -16980, -10726,
	-4738, -1762, 2049, -10968, -9618, -10968, -9619, 8532,
	6142, 8531, 11628, 7898, 8969, -10291, -10292 },
    .z = { 23198, 13647, -2910, 13653, 23176, -1489, -1489, -6384,
	-10918, -13098, -10067, -755, -992, -751, -983, -2679,
	-8523, -2692, -7509, -7853, -7554, -959, -932 }
 };
 
#endif
 
#endif
