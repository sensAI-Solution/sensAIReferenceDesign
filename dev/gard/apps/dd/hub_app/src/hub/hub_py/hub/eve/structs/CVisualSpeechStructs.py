import ctypes
from .ctypes_enum import CtypesEnum
from .EveProcessingStatus import *


class EveVisualSpeechState(CtypesEnum):
	EVE_NOT_SET = 0
	EVE_NOT_SPEAKING = 1
	EVE_SPEAKING = 2

class CVisualSpeechData(ctypes.Structure):
	_fields_ = [
		("processingStatus", ctypes.c_int),
		("speechState", ctypes.c_int),
		("notSpeakingProbability", ctypes.c_double),
		("speakingProbability", ctypes.c_double),
	]

