import ctypes
from .ctypes_enum import CtypesEnum


class EveImageProvider(CtypesEnum):
	EVE_CAMERA = 0
	EVE_CLIENT_PROVIDED = 1

class EveGpuPreference(CtypesEnum):
	EVE_GPU_LOW_POWER = 0
	EVE_GPU_HIGH_PERFORMANCE = 1
	EVE_NO_GPU = 2

class EveStartupParameters(ctypes.Structure):
	_fields_ = [
		("gpuPreference", ctypes.c_int),
		("imageProvider", ctypes.c_int),
		("pathOverride", ctypes.c_byte * 512),
	]

