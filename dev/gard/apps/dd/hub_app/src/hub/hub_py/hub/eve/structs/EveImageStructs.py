import ctypes
from .ctypes_enum import CtypesEnum
from .CBasicStructs import *
from .EveErrors import *


class EveInputImage(ctypes.Structure):
	_fields_ = [
		("data", ctypes.POINTER(ctypes.c_ubyte)),
		("width", ctypes.c_int),
		("height", ctypes.c_int),
		("encoding", ctypes.c_int),
	]

class EveProcessedImage(ctypes.Structure):
	_fields_ = [
		("data", ctypes.POINTER(ctypes.c_ubyte)),
		("width", ctypes.c_int),
		("height", ctypes.c_int),
		("channels", ctypes.c_int),
		("timestamp", ctypes.c_longlong),
		("error", ctypes.c_int),
	]

class EveProcessedFrameTime(ctypes.Structure):
	_fields_ = [
		("frameTime", ctypes.c_double),
		("error", ctypes.c_int),
	]

