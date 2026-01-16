import ctypes
from .ctypes_enum import CtypesEnum


class EveKarolinskaSleepiness(CtypesEnum):
	EVE_KAROLINSKA_DISABLED = 0
	EVE_KAROLINSKA_1 = 1
	EVE_KAROLINSKA_2 = 2
	EVE_KAROLINSKA_3 = 3
	EVE_KAROLINSKA_4 = 4
	EVE_KAROLINSKA_5 = 5
	EVE_KAROLINSKA_6 = 6
	EVE_KAROLINSKA_7 = 7
	EVE_KAROLINSKA_8 = 8
	EVE_KAROLINSKA_9 = 9
	EVE_KAROLINSKA_MAX = 10

class EveEyeClosureState(CtypesEnum):
	EVE_EYE_STATE_UNKNOWN = 0
	EVE_EYE_OPEN = 1
	EVE_EYE_CLOSED = 2

class EveKarolinskaStatus(CtypesEnum):
	EVE_KAROLINSKA_OFF = 0
	EVE_KAROLINSKA_NO_FACE = 1
	EVE_KAROLINSKA_BLINKS_ONLY = 2
	EVE_KAROLINSKA_ON = 3

class CEyeState(ctypes.Structure):
	_fields_ = [
		("state", ctypes.c_int),
		("closure", ctypes.c_float),
		("confidence", ctypes.c_float),
		("eyelidDistanceMM", ctypes.c_float),
	]

class CEyeStates(ctypes.Structure):
	_fields_ = [
		("left", CEyeState),
		("right", CEyeState),
		("fused", CEyeState),
		("blinkCount", ctypes.c_uint),
	]

class CKarolinskaData(ctypes.Structure):
	_fields_ = [
		("status", ctypes.c_int),
		("scale", ctypes.c_int),
		("headPitchScale", ctypes.c_int),
		("yawnScale", ctypes.c_int),
		("blinkDurationScale", ctypes.c_int),
		("yawn", ctypes.c_float),
		("yawnConfidence", ctypes.c_float),
		("yawnCount", ctypes.c_uint),
		("eyes", CEyeStates),
	]

