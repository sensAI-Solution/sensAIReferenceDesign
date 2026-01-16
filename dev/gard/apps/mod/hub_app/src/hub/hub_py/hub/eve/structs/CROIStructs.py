import ctypes
from .ctypes_enum import CtypesEnum
from .EveProcessingStatus import *

EVE_ROI_MAX_SCORE_COUNT = 20


class EveROIState(CtypesEnum):
	EVE_ROI_STATE_INACTIVE = 0
	EVE_ROI_STATE_ENTERING = 1
	EVE_ROI_STATE_LEAVING = 2
	EVE_ROI_STATE_SELECTED = 3

class CROIScore(ctypes.Structure):
	_fields_ = [
		("id", ctypes.c_uint),
		("intersectionScore", ctypes.c_double),
		("filteredScore", ctypes.c_double),
		("state", ctypes.c_int),
	]

class CROIScoreData(ctypes.Structure):
	_fields_ = [
		("processingStatus", ctypes.c_int),
		("fusedRoiScoresCount", ctypes.c_uint),
		("fusedRoiScores", CROIScore * EVE_ROI_MAX_SCORE_COUNT),
		("faceRoiScoresCount", ctypes.c_uint),
		("faceRoiScores", CROIScore * EVE_ROI_MAX_SCORE_COUNT),
	]

