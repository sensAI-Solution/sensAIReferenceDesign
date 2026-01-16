import ctypes
from .ctypes_enum import CtypesEnum
from .EveProcessingStatus import *

EVE_FACE_ID_MAX_MISSING_CALIBRATION_POSES = 5

class EveFaceIdActionStatus(CtypesEnum):
	EVE_FACE_ID_ACTION_IDLE = 0
	EVE_FACE_ID_ACTION_CALIBRATING = 1
	EVE_FACE_ID_ACTION_CALIBRATED = 2
	EVE_FACE_ID_ACTION_IDENTIFIED = 3

class EveFaceIdCalibrationStatus(CtypesEnum):
	EVE_FACE_ID_CALIB_NONE = 0
	EVE_FACE_ID_CALIB_RUNNING = 1
	EVE_FACE_ID_CALIB_SUCCESS = 2
	EVE_FACE_ID_CALIB_FAILURE_LACK_POSE_MOTION = 3
	EVE_FACE_ID_CALIB_FAILURE_OTHER = 4

class EveFaceIdIdentificationStatus(CtypesEnum):
	EVE_FACE_ID_NONE = 0
	EVE_FACE_ID_SUCCESS = 1
	EVE_FACE_ID_FAILURE_VERIFICATION = 2
	EVE_FACE_ID_FAILURE_ANGLE_PITCH = 3
	EVE_FACE_ID_FAILURE_ANGLE_YAW = 4
	EVE_FACE_ID_FAILURE_ANGLE_BOTH = 5
	EVE_FACE_ID_FAILURE_NO_GALLERY = 6
	EVE_FACE_ID_FAILURE_EXP_SMILE = 7
	EVE_FACE_ID_FAILURE_EXP_SQUINT = 8
	EVE_FACE_ID_FAILURE_EXP_EYES_CLOSED = 9
	EVE_FACE_ID_FAILURE_OTHER = 10

class EveFaceIdPose(CtypesEnum):
	EVE_FACE_ID_POSE_FRONTAL = 0
	EVE_FACE_ID_POSE_LEFT = 1
	EVE_FACE_ID_POSE_RIGHT = 2
	EVE_FACE_ID_POSE_UP = 3
	EVE_FACE_ID_POSE_DOWN = 4

class CFaceIdentity(ctypes.Structure):
	_fields_ = [
		("id", ctypes.c_longlong),
		("confidence", ctypes.c_float),
		("similarity", ctypes.c_float),
	]

class CFaceIdentityData(ctypes.Structure):
	_fields_ = [
		("processingStatus", ctypes.c_int),
		("actionStatus", ctypes.c_int),
		("calibrationStatus", ctypes.c_int),
		("identificationStatus", ctypes.c_int),
		("faceIdentity", CFaceIdentity),
		("missingCalibrationPosesCount", ctypes.c_uint),
		("missingCalibrationPoses", ctypes.c_int * EVE_FACE_ID_MAX_MISSING_CALIBRATION_POSES),
	]

