//=============================================================================
//
// Copyright(c) 2025 Lattice Semiconductor Corp. All rights reserved.
//
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by LSCC and are protected 
// by copyright law. They may not be disclosed to third parties or copied 
// or duplicated in any form, in whole or in part, without the prior 
// written consent of LSCC.
//
//=============================================================================

#pragma once

#define EVE_FPGA_LANDMARKS 23 /**< Maximum number of face landmarks that be detected by the FPGA */
#define EVE_FPGA_MAX_USERS 10 /**< Maximum number of users that is supported by the FPGA */
#define EVE_FPGA_MAX_PERSONS 5 /**< Maximum number of person that be detected by the FPGA */
#define EVE_FPGA_MAX_HAND_LANDMARKS 11 /**< Maximum number of hand landmarks that be detected by the FPGA */
#define EVE_FPGA_HAND_LANDMARKS 10 /**< The actual number of landmarks the FPGA outputs, different from the above so we can map our SOC to our FPGA landmarks */
#define EVE_FPGA_MAX_OBJECT_DETECTION 50 /**< Maximum number of objects that can be detected by the FPGA */

#include "CBasicStructs.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

	enum EveFpgaConnectionType
	{
		EVE_FPGA_AUTO_SELECT,
		EVE_FPGA_UART,
		EVE_FPGA_I2C,
		EVE_FPGA_HUB,
		EVE_FPGA_MANUAL
	};

	enum EveFpgaConnectionRequest
	{
		EVE_FPGA_STOP,
		EVE_FPGA_CONTINUE
	};

////////////////////////////////////////////////////////
// Copied from the generated pipeline_config.h
////////////////////////////////////////////////////////

	/*Possible values are:
		ALL = All;
		FD = FaceDetection;
		LM = LandmarksEstimation (Depends on FD);
		ELD = EyesLandmarksEstimation (Depends on FD);
		FV = FaceValidation (Depends on FD);
		DE = DepthEstimation (Depends on FD);
		BIT = Bitstream;
		FW = FirmwareRiscV;
		FID = FaceID (Depends on LM_FV);
		GALLERY = FaceIDGallery;
		LM_FV = LandmarksFaceValidation (Depends on FD);
		HD = HandDetection;
		HLMV = HandLandmarksValidation (Depends on HD);
		PD = PersonDetection;
		PIPELINE_CONFIG = PipelineConfig
	*/

	enum pipeline_config_type_t
	{
		PT_FD    =0,
		PT_LM_FV =1,
		PT_FID   =2,
		PT_PD    =3,
		PT_HD    =4,
		PT_HLMV  =5,
		PT_SIZE
	};

	enum setting_type_t
	{
		CS_ENABLED         = 0 , // 0x00
		CS_IPS             = 1 , // 0x01
		CS_RESERVED_2_7    = 2 , // 0x02
		CS_COMMAND         = 8 , // 0x08
		CS_CUSTOM          = 16, // 0x10
		CS_MAX             = 17, // 0x11
	};

	enum message_type_t
	{
		MT_NONE = 0,
		MT_SET = 1,
		MT_GET = 2,
		MT_GET_BATCH = 3,
		MT_SIZE
	};


	enum response_type_t
	{
		RT_NONE = 0,
		RT_DATA = 1,
		RT_GET = 2,
		RT_ACK = 3,
		RT_SIZE
	};

	struct pipeline_setting_t
	{
		enum setting_type_t settingType;
		uint32_t value;
	};

////////////////////////////////////////////////////////
// End of  Copied from the generated pipeline_config.h
////////////////////////////////////////////////////////

	struct pipeline_config_t
	{
		enum pipeline_config_type_t type;
		struct pipeline_setting_t setting;
	};

	/**
	 * Represents the serial status of the FPGA in EVE.
	 * Also used as an error code source for connections to the FPGA via the connection DLL
	 */
	enum EveFpgaSerialStatus
	{
		EVE_FPGA_SUCCESS, /**< EVE successfully read data from the serial port */
		EVE_FPGA_NO_DATA, /**< EVE did not receive data from the serial port */
		EVE_FPGA_READ_START_MARKER_FAILED, /**< EVE did not read the start marker from serial port */
		EVE_FPGA_FIND_START_MARKER_FAILED, /**< EVE did not find the start marker from serial port */
		EVE_FPGA_READ_DATA_LENGTH_FAILED, /**< EVE did not read correct length from serial port */
		EVE_FPGA_READ_DATA_FAILED, /**< EVE failed to read the data sent by the serial port */
		EVE_FPGA_CORRUPTED_DATA, /**< EVE received corrupted data from the serial port */
		EVE_FPGA_UNEXPECTED_RESPONSE_TYPE, /**< EVE received an unknown response type */
		EVE_FPGA_API_ERROR_START, /**< not a real error code, just numerically indicates the start of the API errors*/
		EVE_FPGA_NO_CALLBACK, /**< Tried initializing the communication object without supplying a callback*/
		EVE_FPGA_DATA_ACCESSED_OUTSIDE_CALLBACK, /**< The FPGA data must be accessed from the callback thread*/
		EVE_FPGA_INIT_FAILED, /**< Could not connect to the FPGA, most likely due to using the wrong COM port*/
		EVE_FPGA_NOT_INIT, /**< You need to call the connection function before trying to read the fpga data*/
		EVE_FPGA_API_ERROR_END /**< not a real error code, just numerically indicates the end of the API errors*/
	};

	/**
	 * Represents what event will wake up EVE.
	 */
	enum EveWakeupDetectionType
	{
		EVE_USER_DETECTION, /**< EVE will wake up when it detects a previously identified user */
		EVE_STRANGER_DETECTION /**< EVE will wake up when it detects a unidentified user */
	};

	/**
	 * Represents the FGPA pipeline.
	 */
	enum EveFpgaPipelineType
	{
		EVE_UNKNOWN_PIPELINE = 0, /**< The FPGA network used is not known yet */
		EVE_HEAD_POSE_PIPELINE = 1, /**< The FPGA will use the head pose network */
		EVE_FACE_ID_PIPELINE = 2, /**< The FPGA will use the Face ID network */
		EVE_HAND_GESTURE_PIPELINE = 3, /**< The FPGA will use the hand gesture network */
		EVE_COMPACT_HEAD_POSE_PIPELINE = 4, /**< The FPGA will use the compact head pose network */
		EVE_HMI_PIPELINE = 5, /**< The FPGA will use the HMI network */
		EVE_STANDALONE_HAND_GESTURE_PIPELINE = 6, /**< The FPGA will use the standalone hand gesture network */
	};

	/**
	 * Represents the body pose of the person.
	 */
	enum EvePersonBodyPose
	{
		EVE_FRONT, /**< The user is front facing towards the camera */
		EVE_NOT_FRONT /**< The user is not facing towards the camera */
	};

	/**
	 * Represents the user distance from the camera.
	 */
	enum EveDistanceFromCamera
	{
		EVE_DISTANCE_CLOSE, /**< The user is close to the camera */
		EVE_DISTANCE_MID, /**< The user is neither close, nor far from the camera */
		EVE_DISTANCE_FAR /**< The user is far from the camera */
	};

	/**
	 * Represents the user's registration status.
	 */
	enum EvePersonRegistrationStatus
	{
		EVE_REGISTERED, /**< The detected user is registered in the system */
		EVE_UNREGISTERED, /**< The detected user is not registered in the system */
		EVE_UNKNOWN, /**< The detected user is unknown */
		EVE_REQUIREMENTS_UNMET, /**< The FaceID requirements prevent FaceID to run (e.g. angle) */
		EVE_DISABLED, /**< The feature is disabled */
		EVE_NO_GALLERY, /**< No FaceID in the gallery */
	};

	/**
	 * Represents the hand gesture type indicated from the FPGA
	 */
	enum EveFpgaHandGesture
	{
		EVE_FPGA_HAND_GESTURE_NO_GESTURE,
		EVE_FPGA_HAND_GESTURE_CLOSE,
		EVE_FPGA_HAND_GESTURE_OPEN,
		EVE_FPGA_HAND_GESTURE_OPEN_LEFT,
		EVE_FPGA_HAND_GESTURE_OPEN_RIGHT,
		EVE_FPGA_HAND_GESTURE_INDEX_UP,
		EVE_FPGA_HAND_GESTURE_INDEX_DOWN,
		EVE_FPGA_HAND_GESTURE_TIP_LEFT,
		EVE_FPGA_HAND_GESTURE_TIP_RIGHT,
		EVE_FPGA_HAND_GESTURE_UNKNOWN,
	};

	/**
	 * Represents the hand gesture type indicated from the FPGA
	 */
	enum EveFpgaObjectClass
	{
		EVE_FPGA_OBJECT_CLASS_PERSON,
		EVE_FPGA_OBJECT_CLASS_BICYCLE,
		EVE_FPGA_OBJECT_CLASS_CAR,
		EVE_FPGA_OBJECT_CLASS_MOTORCYCLE,
		EVE_FPGA_OBJECT_CLASS_BUS,
		EVE_FPGA_OBJECT_CLASS_TRUCK,
		EVE_FPGA_OBJECT_CLASS_TRAFFIC_LIGHT,
		EVE_FPGA_OBJECT_CLASS_STOP_SIGN,
	};

	/**
	 * Represents the data of the ideal person.
	 */
	struct CFpgaIdealPersonData
	{
		unsigned int valid; /**< 1 if the ideal person is valid, 0 if not (discard all data below in this case) */
		unsigned int index; /**< Index (from 0 to EVE_FPGA_MAX_PERSONS) of which person was selected as the ideal person */
		enum EvePersonRegistrationStatus status; /**< Registration status of the ideal person */
		struct CAngles3f faceAngles; /**< Face angles of the ideal person */
		float faceLandmarksConfidence; /**< The confidence of face landmarks data */
		bool isFaceLandmarksConfidenceValid; /**< Is the value of faceLandmarksConfidence valid */
	};

	struct CFpgaImageDimensions
	{
		int width;
		int height;
		struct CRect2i cropArea;
		int reserved1;
		int reserved2;
	};

	struct CFpgaDataContent
	{
		int16_t numberOfUsers; /**< number of users */

		int16_t idealUserIndex; /**< the ideal user's idx in users */

		int16_t numberOfDetectedFaces; /**< number of faces detected */
		float numberOfFacesConfidence; /**< confidence level of numberOfDetectedFaces variable */

		int16_t numberOfDetectedPersons; /**<n umber of persons detected */
		float numberOfPersonsConfidence; /**<confidence level of numberOfPersonsConfidence variable */

		bool isIdealUserDataAvailable; /**< are the ideal user variables valid? */
		bool idealUserDetected; /**< is an ideal user selected ? */
		bool isIdealUserIndexValid; /**< is the idealUserIndex value valid? (Is ideal user among the detected users?) */
		bool isNumberOfDetectedFacesAvailable; /**< is the numberOfDetectedFaces variable value valid */
		bool isNumberOfFacesConfidenceAvailable; /**< is the numberOfFacesConfidence variable value valid */
		bool isNumberOfDetectedPersonsAvailable; /**< is the value of numberOfDetectedPersons variables valid */
		bool isNumberOfPersonsConfidenceAvailable;/**< is the value of numberOfDetectedPersons variables valid */
		bool isUsersDataAvilable; /**< is the data related to the detected users from face/person detection (from face/person box) available?  (not true if just the data from landmark box are available) */
		bool isFaceIdDataAvailable; /**< are the face ID variables values valid */
		bool isObjectDetectionAvailable; /**< is the object detection data valid */
		bool isCameraStreaming; /**< is the fpga telling the camera to stream to the host*/
		bool isHandGestureDataAvailable; /**< is hand gesture data present in this frame*/
	};

	struct CFpgaBlinkData
	{
		int32_t closingDuration;
		int32_t openingDuration;
		int32_t blinkDuration;
		int32_t closingAmplitude;
		int32_t openingAmplitude;
		float confidence;
		int8_t isBlink;
		int16_t blinksPerSec;
	};

	struct CFpgaEyeClosureData
	{
		float closure;
		float confidence;
		int32_t eyelidDistanceMM;
	};

	struct CFpgaPerCloseData
	{
		int16_t eyeState;
		float strictPerClose;
		float extendedPerClose;
		float longClosureRatio;
		struct CFpgaEyeClosureData eyeClosureData;
		bool isLongClosure;
		bool isEyeClosureDataAvailable;
	};

	struct CFpgaDrowsiness
	{
		int16_t attentionState;
		struct CFpgaBlinkData blinkData;
		struct CFpgaPerCloseData percloseData;
		bool isAttentionStateAvailable;
		bool isBlinkDataAvailable;
		bool isPercloseDataAvailable;
	};

	struct CFpgaLandmark
	{
		struct CPoint2i landmark2D;
		struct CPoint3i landmark3D;
	};

	struct CFpgaHandData
	{
		float validationScore;
		struct CRect2i handBox;
		struct CPoint3f landmarks[EVE_FPGA_MAX_HAND_LANDMARKS];
	};

	struct CFpgaHandsData
	{
		int16_t numberOfHandLandmarkPoints;
		struct CFpgaHandData handData;
		enum EveFpgaHandGesture gesture;
		bool isHandBoxAvailable;
		bool isHandLandmark3D;
	};

	struct CFpgaFaceData
	{
		float faceConfidence;
		int16_t faceDistance; /**< Distance to the camera */
		struct CPoint3i faceCenter; /**< Coordinates of the center of the face*/
		struct CAngles3f anglesICS; /**< Angles in the image coordinate system, 2D angles, ignore z */
		struct CAngles3f anglesCCS; /**< Angles in the camera coordinate system, 3D angles*/
		int16_t numberOfFaceLandmarkPoints; /**< If 0, no landmarks are available*/
		struct CPoint3i landmarks[EVE_FPGA_LANDMARKS];
		float faceLandmarksConfidence;
		struct CRect2i faceBox; /**< Face bounding box*/
		struct CFpgaDrowsiness drowsinessData;
		enum EvePersonRegistrationStatus faceIDStatus;
		int16_t faceID; /**< The saved ID in the face ID gallery if this user's face is recognized*/
		bool isFaceConfidenceAvailable;
		bool isFaceDistanceAvailable;
		bool isFacePositionAvailable;
		bool isEulerAnglesIcsAvailable;
		bool isEulerAnglesCcsAvailable;
		bool isFaceLandmark3D;
		bool isFaceLandmarksConfidenceAvailable;
		bool isFaceGeometricBoxAvailable;
		bool isStatusAvailable;
	};

	/**
	 * Represents the FPGA person's data.
	 */
	struct CFpgaPersonData
	{
		float personConfidence;
		enum EveDistanceFromCamera personDistance; /**< rough distance estimate*/
		enum EvePersonBodyPose personPosture; /**< rough orientation estimate*/
		float personFrontalPostureConfidence;
		float personNotFrontalPostureConfidence;
		struct CPoint3i position;
		int16_t numberOfPersonLandmarkPoints; /**< 0 if no landmarks are available*/
		struct CPoint3i landmarks[EVE_FPGA_LANDMARKS];
		struct CRect2i personBox; /**< Bounding box*/
		bool isPersonDataAvailable;
	};

	struct CFpgaObjectDetection
	{
		enum EveFpgaObjectClass objectClass;
		float objectConfidence;
		struct CRect2i objectBox;
	};

	struct CFpgaObjectData
	{
		int16_t numberOfObjects; /**< number of objects */
		struct CFpgaObjectDetection objects[EVE_FPGA_MAX_OBJECT_DETECTION];
	};

	struct CFpgaUserData
	{
		int16_t id;
		enum EvePersonRegistrationStatus status; /**< Face ID status of this user*/
		float scale;
		struct CFpgaFaceData faceData;
		struct CFpgaPersonData personData;
		bool isIdealUser; /**< Is this user being tracked by the FPGA for angles or landmarks*/
		bool isIdValid;
		bool isStatusAvailable;
		bool isScaleAvailable;
	};

	struct CFpgaFaceIdData
	{
		int16_t command;
		int16_t userId;
		int16_t freeEntry;
		int16_t statusCode;
		int16_t faceId;
		int16_t lastRegisteredFaceID;
		int16_t usersInGallery;
		int16_t gallerySize;
	};

	struct CFpgaPipelineData
	{
		enum EveFpgaPipelineType pipelineType;
		struct CFpgaImageDimensions imageDimensions;
		struct CFpgaDataContent dataContent;
		struct CFpgaUserData userData[EVE_FPGA_MAX_USERS];
		struct CFpgaObjectData objectData;
		struct CFpgaFaceIdData faceId;
		struct CFpgaHandsData handsData;
	};

	/**
	 * Represents the FPGA data in EVE.
	 */
	struct CFpgaMessage
	{
		enum response_type_t responseType; /**< Type of the response */ 
		uint8_t responseVersion;
		enum EveFpgaSerialStatus serialStatus; /**< Status of the serial data */
		long long serialReadTimeNano; /**< Delay to read the entire data from the serial port, in nanoseconds */
	};

	/**
	 * Represents the FPGA data in EVE.
	 */
	struct CFpgaData
	{
		struct CFpgaMessage message; /**< Message details from the FPGA */
		struct CFpgaPipelineData pipelineData; /**< Parsed data from the FPGA pipeline */
	};

	/**
	 * Represents the FPGA return when fetching a setting value with MT_GET.
	 */
	struct CFpgaGetSetting
	{
		struct CFpgaMessage message; /**< Message details from the FPGA */
		enum pipeline_config_type_t type; /**< The type of the network */
		enum setting_type_t setting; /**< The setting to get */
		uint32_t value;
	};

	/**
	 * Represents the FPGA parameters if interfacing with cameras equipped with a LSCC FPGA.
	 */
	struct CFpgaParameters
	{
		unsigned int comport; /**< The COM port the FPGA is connected to when using uART*/
		unsigned int socWakeupDelay; /**< Car Sentry feature: delay between person of interest being detected and camera feed wake up*/
		enum EveWakeupDetectionType wakeupType;
		unsigned char forceCameraOn; /**< If the FPGA shuts the camera feed off, set this to false, if the FPGA always streams the camera, set this to true*/
		unsigned char registerNewFace; /**< deprecated*/
		unsigned char clearCurrentFace; /**< deprecated*/
		unsigned char enableFaceId; /**< deprecated*/
		unsigned char allPipelinesSupported; /**< deprecated*/
		unsigned int pipelineVersion; /**< specify a pipeline version, when possible. 0 uses the latest version */
		enum EveFpgaConnectionType connection;
		unsigned int i2cAdapterNumber; /**< The I2C adapter number when using I2C */
		unsigned int i2cDeviceNumber; /**< The I2C device number when using I2C */
		unsigned int i2cIRQPin; /**< The GPIO pin number used for IRQ when using I2C */
	};

	/**
	* On every call in the callback, set the connection request status to continue if you want to keep receiving frames
	* Changing it to stop will allow everything to shutdown gracefully, and will be the last call on the callback you receive
	*/
	struct CFpgaCallbackControl
	{
		enum EveFpgaConnectionRequest request;
	};

	/**
	* Contains a pointer to the data, DO NOT DELETE this pointer
	* The data is only valid inside of the callback
	* data->message.serialStatus contains the actual serial connection state
	* errorCode is used to indicate problems with the API
	* if any errorCode is set, data will be nullptr
	* is data->message.serialStatus has an error, the actual data will be valid, it will just be replayed from a previously successful data frame
	* this means you shouldn't have to cache good results in between frames
	*/
	struct EveFpgaMetadata
	{
		struct CFpgaData *data;
		enum EveFpgaSerialStatus errorCode;
	};

	/**
	 * Send the FPGA metadata from outside EVE.
	 * This is intended to be an internal only feature
	 */
	struct EveFpgaManualData
	{
		unsigned char *data;
		int size;
	};

	/**
	* Contains a pointer to the beginning of a JSON formatted string, as well as asize
	* Do not delete this pointer
	* If any error has occurred, it will be nullptr with a size of 0
	* Similarly to the structure above, if a serial communication error occurs, the data will be replayed form the previously successful frame
	* The actual serial error will be contained in the JSON document
	*/
	struct EveFpgaJsonMetadata
	{
		char const *textStart;
		unsigned int textSize;
		enum EveFpgaSerialStatus errorCode;
	};


#ifdef __cplusplus
}
#endif // __cplusplus
