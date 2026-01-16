import ctypes
from .ctypes_enum import CtypesEnum
from .CBasicStructs import *

EVE_FPGA_LANDMARKS = 23
EVE_FPGA_MAX_USERS = 10
EVE_FPGA_MAX_PERSONS = 5
EVE_FPGA_MAX_HAND_LANDMARKS = 11
EVE_FPGA_HAND_LANDMARKS = 10
EVE_FPGA_MAX_OBJECT_DETECTION = 50
PT_SIZE = 6
MT_SIZE = 4
RT_SIZE = 4

class EveFpgaConnectionType(CtypesEnum):
	EVE_FPGA_AUTO_SELECT = 0
	EVE_FPGA_UART = 1
	EVE_FPGA_I2C = 2
	EVE_FPGA_HUB = 3
	EVE_FPGA_MANUAL = 4

class EveFpgaConnectionRequest(CtypesEnum):
	EVE_FPGA_STOP = 0
	EVE_FPGA_CONTINUE = 1

class pipeline_config_type_t(CtypesEnum):
	PT_FD    =0
	PT_LM_FV =1
	PT_FID   =2
	PT_PD    =3
	PT_HD    =4
	PT_HLMV  =5
	PT_SIZE = 6

class setting_type_t(CtypesEnum):
	CS_ENABLED         = 0
	CS_IPS             = 1
	CS_RESERVED_2_7    = 2
	CS_COMMAND         = 8
	CS_CUSTOM          = 16
	CS_MAX             = 17

class message_type_t(CtypesEnum):
	MT_NONE = 0
	MT_SET = 1
	MT_GET = 2
	MT_GET_BATCH = 3
	MT_SIZE = 4

class response_type_t(CtypesEnum):
	RT_NONE = 0
	RT_DATA = 1
	RT_GET = 2
	RT_ACK = 3
	RT_SIZE = 4

class EveFpgaSerialStatus(CtypesEnum):
	EVE_FPGA_SUCCESS = 0
	EVE_FPGA_NO_DATA = 1
	EVE_FPGA_READ_START_MARKER_FAILED = 2
	EVE_FPGA_FIND_START_MARKER_FAILED = 3
	EVE_FPGA_READ_DATA_LENGTH_FAILED = 4
	EVE_FPGA_READ_DATA_FAILED = 5
	EVE_FPGA_CORRUPTED_DATA = 6
	EVE_FPGA_UNEXPECTED_RESPONSE_TYPE = 7
	EVE_FPGA_API_ERROR_START = 8
	EVE_FPGA_NO_CALLBACK = 9
	EVE_FPGA_DATA_ACCESSED_OUTSIDE_CALLBACK = 10
	EVE_FPGA_INIT_FAILED = 11
	EVE_FPGA_NOT_INIT = 12
	EVE_FPGA_API_ERROR_END = 13

class EveWakeupDetectionType(CtypesEnum):
	EVE_USER_DETECTION = 0
	EVE_STRANGER_DETECTION = 1

class EveFpgaPipelineType(CtypesEnum):
	EVE_UNKNOWN_PIPELINE = 0
	EVE_HEAD_POSE_PIPELINE = 1
	EVE_FACE_ID_PIPELINE = 2
	EVE_HAND_GESTURE_PIPELINE = 3
	EVE_COMPACT_HEAD_POSE_PIPELINE = 4
	EVE_HMI_PIPELINE = 5
	EVE_STANDALONE_HAND_GESTURE_PIPELINE = 6

class EvePersonBodyPose(CtypesEnum):
	EVE_FRONT = 0
	EVE_NOT_FRONT = 1

class EveDistanceFromCamera(CtypesEnum):
	EVE_DISTANCE_CLOSE = 0
	EVE_DISTANCE_MID = 1
	EVE_DISTANCE_FAR = 2

class EvePersonRegistrationStatus(CtypesEnum):
	EVE_REGISTERED = 0
	EVE_UNREGISTERED = 1
	EVE_UNKNOWN = 2
	EVE_REQUIREMENTS_UNMET = 3
	EVE_DISABLED = 4
	EVE_NO_GALLERY = 5

class EveFpgaHandGesture(CtypesEnum):
	EVE_FPGA_HAND_GESTURE_NO_GESTURE = 0
	EVE_FPGA_HAND_GESTURE_CLOSE = 1
	EVE_FPGA_HAND_GESTURE_OPEN = 2
	EVE_FPGA_HAND_GESTURE_OPEN_LEFT = 3
	EVE_FPGA_HAND_GESTURE_OPEN_RIGHT = 4
	EVE_FPGA_HAND_GESTURE_INDEX_UP = 5
	EVE_FPGA_HAND_GESTURE_INDEX_DOWN = 6
	EVE_FPGA_HAND_GESTURE_TIP_LEFT = 7
	EVE_FPGA_HAND_GESTURE_TIP_RIGHT = 8
	EVE_FPGA_HAND_GESTURE_UNKNOWN = 9

class EveFpgaObjectClass(CtypesEnum):
	EVE_FPGA_OBJECT_CLASS_PERSON = 0
	EVE_FPGA_OBJECT_CLASS_BICYCLE = 1
	EVE_FPGA_OBJECT_CLASS_CAR = 2
	EVE_FPGA_OBJECT_CLASS_MOTORCYCLE = 3
	EVE_FPGA_OBJECT_CLASS_BUS = 4
	EVE_FPGA_OBJECT_CLASS_TRUCK = 5
	EVE_FPGA_OBJECT_CLASS_TRAFFIC_LIGHT = 6
	EVE_FPGA_OBJECT_CLASS_STOP_SIGN = 7

class pipeline_setting_t(ctypes.Structure):
	_fields_ = [
		("settingType", ctypes.c_int),
		("value", ctypes.c_uint32),
	]

class pipeline_config_t(ctypes.Structure):
	_fields_ = [
		("type", ctypes.c_int),
		("setting", pipeline_setting_t),
	]

class CFpgaIdealPersonData(ctypes.Structure):
	_fields_ = [
		("valid", ctypes.c_uint),
		("index", ctypes.c_uint),
		("status", ctypes.c_int),
		("faceAngles", CAngles3f),
		("faceLandmarksConfidence", ctypes.c_float),
		("isFaceLandmarksConfidenceValid", ctypes.c_bool),
	]

class CFpgaImageDimensions(ctypes.Structure):
	_fields_ = [
		("width", ctypes.c_int16),
		("height", ctypes.c_int16),
	]

class CFpgaDataContent(ctypes.Structure):
	_fields_ = [
		("numberOfUsers", ctypes.c_int16),
		("idealUserIndex", ctypes.c_int16),
		("numberOfDetectedFaces", ctypes.c_int16),
		("numberOfFacesConfidence", ctypes.c_float),
		("numberOfDetectedPersons", ctypes.c_int16),
		("numberOfPersonsConfidence", ctypes.c_float),
		("isIdealUserDataAvailable", ctypes.c_bool),
		("idealUserDetected", ctypes.c_bool),
		("isIdealUserIndexValid", ctypes.c_bool),
		("isNumberOfDetectedFacesAvailable", ctypes.c_bool),
		("isNumberOfFacesConfidenceAvailable", ctypes.c_bool),
		("isNumberOfDetectedPersonsAvailable", ctypes.c_bool),
		("isNumberOfPersonsConfidenceAvailable", ctypes.c_bool),
		("isUsersDataAvilable", ctypes.c_bool),
		("isFaceIdDataAvailable", ctypes.c_bool),
		("isObjectDetectionAvailable", ctypes.c_bool),
		("isCameraStreaming", ctypes.c_bool),
		("isHandGestureDataAvailable", ctypes.c_bool),
	]

class CFpgaBlinkData(ctypes.Structure):
	_fields_ = [
		("closingDuration", ctypes.c_int32),
		("openingDuration", ctypes.c_int32),
		("blinkDuration", ctypes.c_int32),
		("closingAmplitude", ctypes.c_int32),
		("openingAmplitude", ctypes.c_int32),
		("confidence", ctypes.c_float),
		("8_t isBlink", ctypes.c_int),
		("blinksPerSec", ctypes.c_int16),
	]

class CFpgaEyeClosureData(ctypes.Structure):
	_fields_ = [
		("closure", ctypes.c_float),
		("confidence", ctypes.c_float),
		("eyelidDistanceMM", ctypes.c_int32),
	]

class CFpgaPerCloseData(ctypes.Structure):
	_fields_ = [
		("eyeState", ctypes.c_int16),
		("strictPerClose", ctypes.c_float),
		("extendedPerClose", ctypes.c_float),
		("longClosureRatio", ctypes.c_float),
		("eyeClosureData", CFpgaEyeClosureData),
		("isLongClosure", ctypes.c_bool),
		("isEyeClosureDataAvailable", ctypes.c_bool),
	]

class CFpgaDrowsiness(ctypes.Structure):
	_fields_ = [
		("attentionState", ctypes.c_int16),
		("blinkData", CFpgaBlinkData),
		("percloseData", CFpgaPerCloseData),
		("isAttentionStateAvailable", ctypes.c_bool),
		("isBlinkDataAvailable", ctypes.c_bool),
		("isPercloseDataAvailable", ctypes.c_bool),
	]

class CFpgaLandmark(ctypes.Structure):
	_fields_ = [
		("landmark2D", CPoint2i),
		("landmark3D", CPoint3i),
	]

class CFpgaHandData(ctypes.Structure):
	_fields_ = [
		("validationScore", ctypes.c_float),
		("handBox", CRect2i),
		("landmarks", CPoint3f * EVE_FPGA_MAX_HAND_LANDMARKS),
	]

class CFpgaHandsData(ctypes.Structure):
	_fields_ = [
		("numberOfHandLandmarkPoints", ctypes.c_int16),
		("handData", CFpgaHandData),
		("gesture", ctypes.c_int),
		("isHandBoxAvailable", ctypes.c_bool),
		("isHandLandmark3D", ctypes.c_bool),
	]

class CFpgaFaceData(ctypes.Structure):
	_fields_ = [
		("faceConfidence", ctypes.c_float),
		("faceDistance", ctypes.c_int16),
		("faceCenter", CPoint3i),
		("anglesICS", CAngles3f),
		("anglesCCS", CAngles3f),
		("numberOfFaceLandmarkPoints", ctypes.c_int16),
		("landmarks", CPoint3i * EVE_FPGA_LANDMARKS),
		("faceLandmarksConfidence", ctypes.c_float),
		("faceBox", CRect2i),
		("drowsinessData", CFpgaDrowsiness),
		("faceIDStatus", ctypes.c_int),
		("faceID", ctypes.c_int16),
		("isFaceConfidenceAvailable", ctypes.c_bool),
		("isFaceDistanceAvailable", ctypes.c_bool),
		("isFacePositionAvailable", ctypes.c_bool),
		("isEulerAnglesIcsAvailable", ctypes.c_bool),
		("isEulerAnglesCcsAvailable", ctypes.c_bool),
		("isFaceLandmark3D", ctypes.c_bool),
		("isFaceLandmarksConfidenceAvailable", ctypes.c_bool),
		("isFaceGeometricBoxAvailable", ctypes.c_bool),
		("isStatusAvailable", ctypes.c_bool),
	]

class CFpgaPersonData(ctypes.Structure):
	_fields_ = [
		("personConfidence", ctypes.c_float),
		("personDistance", ctypes.c_int),
		("personPosture", ctypes.c_int),
		("personFrontalPostureConfidence", ctypes.c_float),
		("personNotFrontalPostureConfidence", ctypes.c_float),
		("position", CPoint3i),
		("numberOfPersonLandmarkPoints", ctypes.c_int16),
		("landmarks", CPoint3i * EVE_FPGA_LANDMARKS),
		("personBox", CRect2i),
		("isPersonDataAvailable", ctypes.c_bool),
	]

class CFpgaObjectDetection(ctypes.Structure):
	_fields_ = [
		("objectClass", ctypes.c_int),
		("objectConfidence", ctypes.c_float),
		("objectBox", CRect2i),
	]

class CFpgaObjectData(ctypes.Structure):
	_fields_ = [
		("numberOfObjects", ctypes.c_int16),
		("objects", CFpgaObjectDetection * EVE_FPGA_MAX_OBJECT_DETECTION),
	]

class CFpgaUserData(ctypes.Structure):
	_fields_ = [
		("id", ctypes.c_int16),
		("status", ctypes.c_int),
		("scale", ctypes.c_float),
		("faceData", CFpgaFaceData),
		("personData", CFpgaPersonData),
		("isIdealUser", ctypes.c_bool),
		("isIdValid", ctypes.c_bool),
		("isStatusAvailable", ctypes.c_bool),
		("isScaleAvailable", ctypes.c_bool),
	]

class CFpgaFaceIdData(ctypes.Structure):
	_fields_ = [
		("command", ctypes.c_int16),
		("userId", ctypes.c_int16),
		("freeEntry", ctypes.c_int16),
		("statusCode", ctypes.c_int16),
		("faceId", ctypes.c_int16),
		("lastRegisteredFaceID", ctypes.c_int16),
		("usersInGallery", ctypes.c_int16),
		("gallerySize", ctypes.c_int16),
	]

class CFpgaPipelineData(ctypes.Structure):
	_fields_ = [
		("pipelineType", ctypes.c_int),
		("imageDimensions", CFpgaImageDimensions),
		("dataContent", CFpgaDataContent),
		("userData", CFpgaUserData * EVE_FPGA_MAX_USERS),
		("objectData", CFpgaObjectData),
		("faceId", CFpgaFaceIdData),
		("handsData", CFpgaHandsData),
	]

class CFpgaMessage(ctypes.Structure):
	_fields_ = [
		("responseType", ctypes.c_int),
		("responseVersion", ctypes.c_uint8),
		("serialStatus", ctypes.c_int),
		("serialReadTimeNano", ctypes.c_longlong),
	]

class CFpgaData(ctypes.Structure):
	_fields_ = [
		("message", CFpgaMessage),
		("pipelineData", CFpgaPipelineData),
	]

class CFpgaGetSetting(ctypes.Structure):
	_fields_ = [
		("message", CFpgaMessage),
		("type", ctypes.c_int),
		("setting", ctypes.c_int),
		("value", ctypes.c_uint32),
	]

class CFpgaParameters(ctypes.Structure):
	_fields_ = [
		("comport", ctypes.c_uint),
		("socWakeupDelay", ctypes.c_uint),
		("wakeupType", ctypes.c_int),
		("forceCameraOn", ctypes.c_ubyte),
		("registerNewFace", ctypes.c_ubyte),
		("clearCurrentFace", ctypes.c_ubyte),
		("enableFaceId", ctypes.c_ubyte),
		("allPipelinesSupported", ctypes.c_ubyte),
		("pipelineVersion", ctypes.c_uint),
		("connection", ctypes.c_int),
		("i2cAdapterNumber", ctypes.c_uint),
		("i2cDeviceNumber", ctypes.c_uint),
		("i2cIRQPin", ctypes.c_uint),
	]

class CFpgaCallbackControl(ctypes.Structure):
	_fields_ = [
		("request", ctypes.c_int),
	]

class EveFpgaMetadata(ctypes.Structure):
	_fields_ = [
		("data", ctypes.POINTER(CFpgaData)),
		("errorCode", ctypes.c_int),
	]

class EveFpgaManualData(ctypes.Structure):
	_fields_ = [
		("data", ctypes.POINTER(ctypes.c_ubyte)),
		("size", ctypes.c_int),
	]

class EveFpgaJsonMetadata(ctypes.Structure):
	_fields_ = [
		("textStart", ctypes.POINTER(ctypes.c_byte)),
		("textSize", ctypes.c_uint),
		("errorCode", ctypes.c_int),
	]

