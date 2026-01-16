import ctypes
from .ctypes_enum import CtypesEnum
from .CScreenLocation import *
from .EveErrors import *


class EveScreenLocationOptions(ctypes.Structure):
	_fields_ = [
		("topLeftX", ctypes.c_float),
		("topLeftY", ctypes.c_float),
	]

class EveScreenLocationData(ctypes.Structure):
	_fields_ = [
		("data", CScreenLocation),
		("error", ctypes.c_int),
	]

