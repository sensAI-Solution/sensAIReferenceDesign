import ctypes
from .ctypes_enum import CtypesEnum
from .EveProcessingStatus import *

EVE_DETECTIONS_SIZE = 256

EVE_CLASS_ID_NAME_SIZE = 64


class EveActionStatus(CtypesEnum):
	EVE_IDLE = 0
	EVE_INTERPOLATED = 1
	EVE_COMPUTED = 2
	EVE_NO_OUTPUT = 3

class CSingleDetectionData(ctypes.Structure):
	_fields_ = [
		("topLeftX", ctypes.c_int),
		("topLeftY", ctypes.c_int),
		("bottomRightX", ctypes.c_int),
		("bottomRightY", ctypes.c_int),
		("classScore", ctypes.c_float),
		("classId", ctypes.c_int),
		("classIdName", ctypes.c_byte * EVE_CLASS_ID_NAME_SIZE),
	]

class CDetectionData(ctypes.Structure):
	_fields_ = [
		("processingStatus", ctypes.c_int),
		("actionStatus", ctypes.c_int),
		("numberOfDetections", ctypes.c_int),
		("detections", CSingleDetectionData * EVE_DETECTIONS_SIZE),
	]

