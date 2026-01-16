import ctypes
from .ctypes_enum import CtypesEnum


class EveProcessingStatus(CtypesEnum):
	EVE_PROCESSING_DISABLED = 0
	EVE_PROCESSING_ENABLED_FAILURE = 1
	EVE_PROCESSING_ENABLED_SUCCESS = 2
	EVE_SOURCED_FROM_FPGA = 3

