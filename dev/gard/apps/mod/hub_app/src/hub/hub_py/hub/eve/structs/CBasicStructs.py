import ctypes
from .ctypes_enum import CtypesEnum


class EveVideoFormat(CtypesEnum):
	EVE_NONE = 0
	EVE_BGRA = 1
	EVE_YUY2 = 2
	EVE_NV12 = 3
	EVE_MJPG = 4
	EVE_BGR = 5
	EVE_GRAYSCALE = 6

class CPoint2i(ctypes.Structure):
	_fields_ = [
		("x", ctypes.c_int),
		("y", ctypes.c_int),
	]

class CPoint2f(ctypes.Structure):
	_fields_ = [
		("x", ctypes.c_float),
		("y", ctypes.c_float),
	]

class CPoint3i(ctypes.Structure):
	_fields_ = [
		("x", ctypes.c_int),
		("y", ctypes.c_int),
		("z", ctypes.c_int),
	]

class CPoint3f(ctypes.Structure):
	_fields_ = [
		("x", ctypes.c_float),
		("y", ctypes.c_float),
		("z", ctypes.c_float),
	]

class CAngles3f(ctypes.Structure):
	_fields_ = [
		("pitch", ctypes.c_float),
		("yaw", ctypes.c_float),
		("roll", ctypes.c_float),
	]

class CRect2i(ctypes.Structure):
	_fields_ = [
		("left", ctypes.c_int),
		("top", ctypes.c_int),
		("right", ctypes.c_int),
		("bottom", ctypes.c_int),
	]

class CRect2iWH(ctypes.Structure):
	_fields_ = [
		("x", ctypes.c_int),
		("y", ctypes.c_int),
		("width", ctypes.c_int),
		("height", ctypes.c_int),
	]

class CRect2fWH(ctypes.Structure):
	_fields_ = [
		("x", ctypes.c_float),
		("y", ctypes.c_float),
		("width", ctypes.c_float),
		("height", ctypes.c_float),
	]

class CResolution(ctypes.Structure):
	_fields_ = [
		("width", ctypes.c_uint),
		("height", ctypes.c_uint),
	]

