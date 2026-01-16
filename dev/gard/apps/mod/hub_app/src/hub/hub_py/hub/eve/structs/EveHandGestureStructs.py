import ctypes
from .ctypes_enum import CtypesEnum
from .CHandGesture import *
from .EveControlOption import *
from .EveErrors import *


class EveHandGestureOptions(ctypes.Structure):
	_fields_ = [
		("enabled", ctypes.c_int),
		("redetectionDelay", ctypes.c_int),
		("async", ctypes.c_uint),
		("errorCode", ctypes.c_int),
	]

class EveHandGestureData(ctypes.Structure):
	_fields_ = [
		("hands", ctypes.POINTER(EveHandDetections)),
		("errorCode", ctypes.c_int),
	]

class EveStaticGestureData(ctypes.Structure):
	_fields_ = [
		("gestures", EveStaticGestures),
		("errorCode", ctypes.c_int),
	]

class EveDynamicGestureData(ctypes.Structure):
	_fields_ = [
		("gestures", EveDynamicGestures),
		("errorCode", ctypes.c_int),
	]

class EveStaticGestureDefinitions(ctypes.Structure):
	_fields_ = [
		("count", ctypes.c_uint),
		("errorCode", ctypes.c_int),
		("definitions", EveStaticGestureDefinition * EVE_MAX_STATIC_GESTURES),
	]

class EveDynamicGestureDefinitions(ctypes.Structure):
	_fields_ = [
		("count", ctypes.c_uint),
		("errorCode", ctypes.c_int),
		("definitions", EveDynamicGestureDefinition * EVE_DYNAMIC_GESTURE_SIZE),
	]

