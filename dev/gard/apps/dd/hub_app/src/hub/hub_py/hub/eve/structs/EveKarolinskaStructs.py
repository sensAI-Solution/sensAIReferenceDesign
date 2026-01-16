import ctypes
from .ctypes_enum import CtypesEnum
from .CKarolinska import *
from .EveControlOption import *
from .EveErrors import *


class EveKarolinskaOptions(ctypes.Structure):
	_fields_ = [
		("enabled", ctypes.c_int),
		("error", ctypes.c_int),
	]

class EveKarolinskaData(ctypes.Structure):
	_fields_ = [
		("data", CKarolinskaData),
		("error", ctypes.c_int),
	]

