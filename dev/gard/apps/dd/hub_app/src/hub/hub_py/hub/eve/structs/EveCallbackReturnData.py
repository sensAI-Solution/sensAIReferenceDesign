import ctypes
from .ctypes_enum import CtypesEnum


class EveRequestedProcessingState(CtypesEnum):
	EVE_REQUESTED_PROCESSING_STATE_CONTINUE = 0
	EVE_REQUESTED_PROCESSING_STATE_STOP = 1

class EveProcessingCallbackReturnData(ctypes.Structure):
	_fields_ = [
		("requestedState", ctypes.c_int),
	]

