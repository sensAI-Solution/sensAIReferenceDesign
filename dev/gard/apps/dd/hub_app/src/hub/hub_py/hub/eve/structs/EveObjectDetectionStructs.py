import ctypes
from .ctypes_enum import CtypesEnum
from .CDetectionStructs import *
from .EveControlOption import *
from .EveErrors import *


class EveObjectDetectionOptions(ctypes.Structure):
	_fields_ = [
		("enabled", ctypes.c_int),
		("error", ctypes.c_int),
	]

class EvePersonDetectionOptions(ctypes.Structure):
	_fields_ = [
		("enabled", ctypes.c_int),
		("error", ctypes.c_int),
	]

class EveDetectionData(ctypes.Structure):
	_fields_ = [
		("data", ctypes.POINTER(CDetectionData)),
		("error", ctypes.c_int),
	]

