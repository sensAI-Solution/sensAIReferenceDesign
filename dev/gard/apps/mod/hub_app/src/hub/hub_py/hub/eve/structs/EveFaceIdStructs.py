import ctypes
from .ctypes_enum import CtypesEnum
from .CFaceIdStructs import *
from .EveControlOption import *
from .EveErrors import *


class EveFaceIdCalibrationPoseMode(CtypesEnum):
	EVE_FACEID_CALIBRATION_FRONTAL_ONLY = 1

class EveFaceIdOptions(ctypes.Structure):
	_fields_ = [
		("enabled", ctypes.c_int),
		("calibrationPoses", ctypes.c_int),
		("galleryPath", ctypes.c_byte * 256),
		("threshold", ctypes.c_float),
		("error", ctypes.c_int),
	]

class EveFaceIdData(ctypes.Structure):
	_fields_ = [
		("data", CFaceIdentityData),
		("error", ctypes.c_int),
	]

