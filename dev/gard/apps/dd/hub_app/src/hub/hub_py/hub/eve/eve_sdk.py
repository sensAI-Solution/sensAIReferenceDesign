import ctypes
import hub.eve.eve_sdk_structs as structs

EveProcessingCallbackFn = ctypes.CFUNCTYPE(None, ctypes.POINTER(structs.EveProcessingCallbackReturnData))

class EveSDK:
    def __init__(self, dll_path: str):
        self.cdll = ctypes.CDLL(dll_path)
        # EveCameraApi.h
        self.cdll.EveGetFormats.restype = structs.EveCameraFormats
        self.cdll.EveGetFormats.argtypes = [ctypes.c_uint, structs.CCameraFormat]
        self.cdll.EveGetCamera.restype = structs.EveCamera
        self.cdll.EveGetCamera.argtypes = [ctypes.c_uint]
        self.cdll.EveSetCamera.restype = structs.EveError
        self.cdll.EveSetCamera.argtypes = [ctypes.c_uint, structs.CCameraFormat]
        # EveControlInterface.h
        self.cdll.CreateEve.restype = structs.EveError
        self.cdll.CreateEve.argtypes = [structs.EveStartupParameters]
        self.cdll.EveRegisterDataCallback.restype = structs.EveError
        self.cdll.EveRegisterDataCallback.argtypes = [EveProcessingCallbackFn]
        self.cdll.StartEve.restype = structs.EveError
        self.cdll.StartEve.argtypes = []
        self.cdll.EveSendImageForProcessing.restype = structs.EveError
        self.cdll.EveSendImageForProcessing.argtypes = [structs.EveInputImage]
        self.cdll.EveSendFpgaDataManually.restype = structs.EveError
        self.cdll.EveSendFpgaDataManually.argtypes = [structs.EveFpgaManualData]        
        self.cdll.ShutdownEve.restype = structs.EveError
        self.cdll.ShutdownEve.argtypes = []

        # EveKarolinska.h
        self.cdll.EveConfigureKarolinska.restype = structs.EveKarolinskaOptions
        self.cdll.EveConfigureKarolinska.argtypes = [structs.EveKarolinskaOptions]

        self.cdll.EveGetKarolinskaData.restype = structs.EveKarolinskaData
        self.cdll.EveGetKarolinskaData.argtypes = []

        # EveFaceId.h
        self.cdll.EveConfigureFaceId.restype = structs.EveFaceIdOptions
        self.cdll.EveConfigureFaceId.argtypes = [structs.EveFaceIdOptions]
        self.cdll.EveFaceIdCalibrateCurrent.restype = structs.EveError
        self.cdll.EveFaceIdCalibrateCurrent.argtypes = []
        self.cdll.EveFaceIdCalibrateNew.restype = structs.EveError
        self.cdll.EveFaceIdCalibrateNew.argtypes = []
        self.cdll.EveFaceIdForceIdentify.restype = structs.EveError
        self.cdll.EveFaceIdForceIdentify.argtypes = []
        self.cdll.EveFaceIdRemoveCurrent.restype = structs.EveError
        self.cdll.EveFaceIdRemoveCurrent.argtypes = []
        self.cdll.EveFaceIdRemoveAll.restype = structs.EveError
        self.cdll.EveFaceIdRemoveAll.argtypes = []
        self.cdll.EveFaceIdReloadGallery.restype = structs.EveError
        self.cdll.EveFaceIdReloadGallery.argtypes = []
        self.cdll.EveFaceIdCommandWaiting.restype = ctypes.c_uint
        self.cdll.EveFaceIdCommandWaiting.argtypes = []
        self.cdll.EveGetFaceIdData.restype = structs.EveFaceIdData
        self.cdll.EveGetFaceIdData.argtypes = []
        # EveFaceTracker.h
        self.cdll.EveConfigureFaceTracker.restype = structs.EveError
        self.cdll.EveConfigureFaceTracker.argtypes = [structs.EveFaceTrackerOptions]        
        # EveFpga.h
        self.cdll.EveConfigureFpga.restype = structs.EveFpgaOptions
        self.cdll.EveConfigureFpga.argtypes = [structs.EveFpgaOptions]
        self.cdll.EveConfigureFpgaDebug.restype = structs.EveFpgaDebugOptions
        self.cdll.EveConfigureFpgaDebug.argtypes = [structs.EveFpgaDebugOptions]
        self.cdll.QueryFpgaSetting.restype = structs.EveError
        self.cdll.QueryFpgaSetting.argtypes = [structs.pipeline_config_t, ctypes.c_bool]
        self.cdll.QueryFpgaSettings.restype = structs.EveError
        self.cdll.QueryFpgaSettings.argtypes = [ctypes.c_uint16, ctypes.c_uint32, ctypes.c_bool]
        self.cdll.SendSetSetting.restype = structs.EveError
        self.cdll.SendSetSetting.argtypes = [structs.pipeline_config_t]
        self.cdll.PopQueuedSetting.restype = structs.CFpgaGetSetting
        self.cdll.PopQueuedSetting.argtypes = []
        self.cdll.EveGetFpgaData.restype = structs.EveFpgaData
        self.cdll.EveGetFpgaData.argtypes = []
        self.cdll.FpgaReadJson.restype = structs.EveFpgaJsonMetadata
        self.cdll.FpgaReadJson.argtypes = []
        
        # EveImage.h
        self.cdll.EveGetProcessedImage.restype = structs.EveProcessedImage
        self.cdll.EveGetProcessedImage.argtypes = []
        self.cdll.EveGetProcessedFrameTime.restype = structs.EveProcessedFrameTime
        self.cdll.EveGetProcessedFrameTime.argtypes = []
        # EveObjectDetection.h
        self.cdll.EveGetObjectDetectionData.restype = structs.EveDetectionData
        self.cdll.EveGetObjectDetectionData.argtypes = []
        self.cdll.EveCopyObjectDetectionData.restype = structs.EveDetectionData
        self.cdll.EveCopyObjectDetectionData.argtypes = []
        self.cdll.EveGetPersonDetectionData.restype = structs.EveDetectionData
        self.cdll.EveGetPersonDetectionData.argtypes = []
        self.cdll.EveCopyPersonDetectionData.restype = structs.EveDetectionData
        self.cdll.EveCopyPersonDetectionData.argtypes = []
        self.cdll.DeleteDetectionData.restype = structs.EveError
        self.cdll.DeleteDetectionData.argtypes = [structs.EveDetectionData]
        # EveROI.h
        self.cdll.EveConfigureROIs.restype = structs.EveROIOptions
        self.cdll.EveConfigureROIs.argtypes = [structs.EveROIOptions]
        self.cdll.EveGetROIScoreData.restype = structs.EveROIScoreData
        self.cdll.EveGetROIScoreData.argtypes = []
        # EveHandGesture.h
        self.cdll.EveConfigureHandGesture.restype = structs.EveHandGestureOptions
        self.cdll.EveConfigureHandGesture.argtypes = [structs.EveHandGestureOptions]
        self.cdll.EveGetHandGestureData.restype = structs.EveHandGestureData
        self.cdll.EveGetHandGestureData.argtypes = []
        self.cdll.EveCopyHandGestureData.restype = structs.EveHandGestureData
        self.cdll.EveCopyHandGestureData.argtypes = []
        self.cdll.EveDeleteHandGestureData.restype = structs.EveError
        self.cdll.EveDeleteHandGestureData.argtypes = [structs.EveHandGestureData]
        self.cdll.EveGetStaticGestureDetections.restype = structs.EveStaticGestureData
        self.cdll.EveGetStaticGestureDetections.argtypes = []
        self.cdll.EveGetDynamicGestureDetections.restype = structs.EveDynamicGestureData
        self.cdll.EveGetDynamicGestureDetections.argtypes = []


    # EveCamera.h
    def EveGetFormats(self, cameraId: ctypes.c_uint, filter: structs.CCameraFormat) -> structs.EveCameraFormats:
        return self.cdll.EveGetFormats(cameraId, filter)
    
    def EveGetCamera(self, cameraId: ctypes.c_uint) -> structs.EveCamera:
        return self.cdll.EveGetCamera(cameraId)
    
    def EveSetCamera(self, cameraId: ctypes.c_uint, filter: structs.CCameraFormat) -> structs.EveError:
        return self.cdll.EveSetCamera(cameraId, filter)
    
    # EveControlInterface.h
    def CreateEve(self, options: structs.EveStartupParameters) -> structs.EveError:
        return self.cdll.CreateEve(options)
    
    def EveRegisterDataCallback(self, callback) -> structs.EveError:
        return self.cdll.EveRegisterDataCallback(callback)
    
    def StartEve(self) -> structs.EveError:
        return self.cdll.StartEve()
    
    def EveSendImageForProcessing(self, image: structs.EveInputImage) -> structs.EveError:
        return self.cdll.EveSendImageForProcessing(image)
        
    def EveSendFpgaDataManually(self, image: structs.EveFpgaManualData) -> structs.EveError:
        return self.cdll.EveSendFpgaDataManually(image)
    
    def ShutdownEve(self) -> structs.EveError:
        return self.cdll.ShutdownEve()    

    # EveFaceId.h
    def EveConfigureFaceId(self, options: structs.EveFaceIdOptions) -> structs.EveFaceIdOptions:
        return self.cdll.EveConfigureFaceId(options)

    def EveFaceIdCalibrateCurrent(self) -> structs.EveError:
        return self.cdll.EveFaceIdCalibrateCurrent()
    
    def EveFaceIdCalibrateNew(self) -> structs.EveError:
        return self.cdll.EveFaceIdCalibrateNew()
    
    def EveFaceIdForceIdentify(self) -> structs.EveError:
        return self.cdll.EveFaceIdForceIdentify()
    
    def EveFaceIdRemoveCurrent(self) -> structs.EveError:
        return self.cdll.EveFaceIdRemoveCurrent()
    
    def EveFaceIdRemoveAll(self) -> structs.EveError:
        return self.cdll.EveFaceIdRemoveAll()
    
    def EveFaceIdReloadGallery(self) -> structs.EveError:
        return self.cdll.EveFaceIdReloadGallery()
    
    def EveFaceIdCommandWaiting(self) -> ctypes.c_uint:
        return self.cdll.EveFaceIdCommandWaiting()
    
    def EveGetFaceIdData(self) -> structs.EveFaceIdData:
        return self.cdll.EveGetFaceIdData()
    
    # EveFaceTracker.h
    def EveConfigureFaceTracker(self, mode: structs.EveFaceTrackerOptions) -> structs.EveError:
        return self.cdll.EveConfigureFaceTracker(mode)

    # EveFpga.h
    def EveGetFpgaData(self) -> structs.EveFpgaData:
        return self.cdll.EveGetFpgaData()
        
    def EveConfigureFpga(self, options: structs.EveFpgaOptions) -> structs.EveFpgaOptions:
        return self.cdll.EveConfigureFpga(options)
        
    def EveConfigureFpgaDebug(self, options: structs.EveFpgaDebugOptions) -> structs.EveFpgaDebugOptions:
        return self.cdll.EveConfigureFpgaDebug(options)
        
    def QueryFpgaSetting(self, command: structs.pipeline_config_t, notify: ctypes.c_bool) -> structs.EveError:
        return self.cdll.QueryFpgaSetting(command, notify)

    def QueryFpgaSettings(self, typeMask: ctypes.c_uint16, settingsMask: ctypes.c_uint32, notify: ctypes.c_bool) -> structs.EveError:
        return self.cdll.QueryFpgaSettings(typeMask, settingsMask, notify)
        
    def SendSetSetting(self, command: structs.pipeline_config_t) -> structs.EveError:
        return self.cdll.SendSetSetting(command)
        
    def PopQueuedSetting(self) -> structs.CFpgaGetSetting:
        return self.cdll.PopQueuedSetting()
        
    def FpgaReadJson(self) -> structs.EveFpgaJsonMetadata:
        return self.cdll.FpgaReadJson()
        
    # EveKarolinksa.h
    def EveConfigureKarolinska(self, parameters: structs.EveKarolinskaOptions) -> structs.EveKarolinskaOptions:
        return self.cdll.EveConfigureKarolinska(parameters)

    def EveGetKarolinskaData(self) -> structs.EveKarolinskaData:
        return self.cdll.EveGetKarolinskaData()

    # EveImage.h
    def EveGetProcessedImage(self) -> structs.EveProcessedImage:
        return self.cdll.EveGetProcessedImage()
    
    def EveGetProcessedFrameTime(self) -> structs.EveProcessedFrameTime:
        return self.cdll.EveGetProcessedFrameTime()
    
    # EveObjectDetection.h
    def EveConfigureObjectDetection(self, enabled: structs.EveObjectDetectionOptions) -> structs.EveObjectDetectionOptions:
        return self.cdll.EveConfigureObjectDetection(enabled)
    
    def EveConfigurePersonDetection(self, enabled: structs.EvePersonDetectionOptions) -> structs.EvePersonDetectionOptions:
        return self.cdll.EveConfigurePersonDetection(enabled)
    
    def EveGetObjectDetectionData(self) -> structs.EveDetectionData:
        return self.cdll.EveGetObjectDetectionData()
    
    def EveCopyObjectDetectionData(self) -> structs.EveDetectionData:
        return self.cdll.EveCopyObjectDetectionData()
    
    def EveGetPersonDetectionData(self) -> structs.EveDetectionData:
        return self.cdll.EveGetPersonDetectionData()
    
    def EveCopyPersonDetectionData(self) -> structs.EveDetectionData:
        return self.cdll.EveCopyPersonDetectionData()
    
    def DeleteDetectionData(self, data: structs.EveDetectionData) -> structs.EveError:
        return self.cdll.DeleteDetectionData(data)
    
    # EveROI.h
    def EveConfigureROIs(self, options: structs.EveROIOptions) -> structs.EveError:
        return self.cdll.EveConfigureROIs(options)
    
    def EveGetROIScoreData(self) -> structs.EveROIScoreData:
        return self.cdll.EveGetROIScoreData()

    # EveHandGesture.h
    def EveConfigureHandGesture(self, options: structs.EveHandGestureOptions) -> structs.EveHandGestureOptions:
        return self.cdll.EveConfigureHandGesture(options)

    def EveGetHandGestureData(self) -> structs.EveHandGestureData:
        return self.cdll.EveGetHandGestureData()

    def EveCopyHandGestureData(self) -> structs.EveHandGestureData:
        return self.cdll.EveGetHandGestureData()

    def EveDeleteHandGestureData(self, data: structs.EveHandGestureData) -> structs.EveError:
        return self.cdll.EveGetHandGestureData(data)

    def EveGetStaticGestureDetections(self) -> structs.EveStaticGestureData:
        return self.cdll.EveGetStaticGestureDetections()

    def EveGetDynamicGestureDetections(self) -> structs.EveDynamicGestureData:
        return self.cdll.EveGetDynamicGestureDetections()
