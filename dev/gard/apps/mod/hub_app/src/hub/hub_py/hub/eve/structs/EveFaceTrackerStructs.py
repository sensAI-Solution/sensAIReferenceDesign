import ctypes
from .ctypes_enum import CtypesEnum
from .CFaceData import *
from .EveErrors import *


class EveFaceTrackerMinimumMode(CtypesEnum):
	EVE_FACETRACKER_MINIMUM_MODE_OFF = 0
	EVE_FACETRACKER_MINIMUM_MODE_MINIMAL = 1
	EVE_FACETRACKER_MINIMUM_MODE_AVERAGE = 2
	EVE_FACETRACKER_MINIMUM_MODE_MAXIMAL = 3

class EveFaceTrackerOptions(ctypes.Structure):
	_fields_ = [
		("faceTrackerMode", ctypes.c_int),
		("enableEyeLandmarks", ctypes.c_uint),
		("error", ctypes.c_int),
	]

class EveEyes(ctypes.Structure):
	_fields_ = [
		("data", CEyeLandmarks),
		("errorCode", ctypes.c_int),
	]

class EvePupils(ctypes.Structure):
	_fields_ = [
		("data", CPupilLandmarks),
		("errorCode", ctypes.c_int),
	]

