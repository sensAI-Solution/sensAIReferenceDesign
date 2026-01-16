import ctypes
from .ctypes_enum import CtypesEnum
from .CFpgaData import *
from .EveErrors import *


class EveFpgaOptions(ctypes.Structure):
	_fields_ = [
		("parameters", CFpgaParameters),
		("error", ctypes.c_int),
	]

class EveFpgaDebugOptions(ctypes.Structure):
	_fields_ = [
		("enableDrawingOnImage", ctypes.c_uint),
		("error", ctypes.c_int),
	]

class EveFpgaData(ctypes.Structure):
	_fields_ = [
		("data", ctypes.POINTER(CFpgaData)),
		("error", ctypes.c_int),
	]

