import ctypes
from .ctypes_enum import CtypesEnum
from .CROIStructs import *
from .EveControlOption import *
from .EveErrors import *

EVE_ROI_MAX_COUNT = 20


class EveROI(ctypes.Structure):
	_fields_ = [
		("id", ctypes.c_uint),
		("x", ctypes.c_int),
		("y", ctypes.c_int),
		("width", ctypes.c_int),
		("height", ctypes.c_int),
		("scoreThresholdForInactive", ctypes.c_double),
	]

class EveROIOptions(ctypes.Structure):
	_fields_ = [
		("enabled", ctypes.c_int),
		("roiSelectionResponseTime", ctypes.c_double),
		("roiCount", ctypes.c_uint),
		("rois", EveROI * EVE_ROI_MAX_COUNT),
		("error", ctypes.c_int),
		("headVectorOnly", ctypes.c_uint),
	]

class EveROIScoreData(ctypes.Structure):
	_fields_ = [
		("data", CROIScoreData),
		("error", ctypes.c_int),
	]

