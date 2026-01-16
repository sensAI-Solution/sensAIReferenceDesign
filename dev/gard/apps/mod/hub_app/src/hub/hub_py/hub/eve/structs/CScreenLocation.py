import ctypes
from .ctypes_enum import CtypesEnum


class CScreenLocation(ctypes.Structure):
	_fields_ = [
		("topLeftXInMM", ctypes.c_float),
		("topLeftYInMM", ctypes.c_float),
	]

