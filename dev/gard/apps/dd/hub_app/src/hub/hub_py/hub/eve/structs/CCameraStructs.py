import ctypes
from .ctypes_enum import CtypesEnum
from .CBasicStructs import *

CAMERA_PID_VID_SIZE = 8
CAMERA_NAME_SIZE = 64

class EveCompare(CtypesEnum):
	EVE_EQUAL = 0
	EVE_AT_MOST = 1
	EVE_AT_LEAST = 2

class CCameraFormat(ctypes.Structure):
	_fields_ = [
		("resolution", CResolution),
		("format", ctypes.c_int),
		("fps", ctypes.c_float),
		("compareResolution", ctypes.c_int),
		("compareFps", ctypes.c_int),
	]

class CCamera(ctypes.Structure):
	_fields_ = [
		("id", ctypes.c_int),
		("pid", ctypes.c_byte * CAMERA_PID_VID_SIZE),
		("vid", ctypes.c_byte * CAMERA_PID_VID_SIZE),
		("name", ctypes.c_byte * CAMERA_NAME_SIZE),
		("isHardwareCamera", ctypes.c_uint),
		("isFpgaCamera", ctypes.c_uint),
		("isIrCamera", ctypes.c_uint),
	]

class CCameraParameters(ctypes.Structure):
	_fields_ = [
		("width", ctypes.c_int),
		("height", ctypes.c_int),
		("focalLength", ctypes.c_double),
		("pixelSizeX", ctypes.c_double),
		("pixelSizeY", ctypes.c_double),
		("principalPointX", ctypes.c_double),
		("principalPointY", ctypes.c_double),
		("depthMin", ctypes.c_double),
		("depthMax", ctypes.c_double),
		("screenLocationXinMM", ctypes.c_float),
		("screenLocationYinMM", ctypes.c_float),
		("isInfrared", ctypes.c_int),
	]

