import ctypes
from .ctypes_enum import CtypesEnum
from .CCameraStructs import *
from .EveErrors import *

EVE_CAMERA_FORMATS_SIZE = 20

class EveCameraFormats(ctypes.Structure):
	_fields_ = [
		("formats", CCameraFormat * EVE_CAMERA_FORMATS_SIZE),
		("formatsCount", ctypes.c_uint),
		("hadMoreFormats", ctypes.c_uint),
		("error", ctypes.c_int),
	]

class EveCamera(ctypes.Structure):
	_fields_ = [
		("data", CCamera),
		("error", ctypes.c_int),
	]

class EveNumberOfCameras(ctypes.Structure):
	_fields_ = [
		("count", ctypes.c_uint),
		("error", ctypes.c_int),
	]

