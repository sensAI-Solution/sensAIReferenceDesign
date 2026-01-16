# -----------------------------------------------------------------------------
# Copyright (c) 2025 Lattice Semiconductor Corporation
#
# SPDX-License-Identifier: UNLICENSED
#
# -----------------------------------------------------------------------------
# EVE SDK
#
# A Python library EVE, a metada parsing, reporting and drawing
#
# The eve.py is designed to provide python bindings to the
# EVE SDK C library functions, so that python applications can use EVE natively.
#
# Interfaces of EVEInterface class:
#
# 1.  isInitialized() - Returns whether or not initialization was called and working properly
# 2.  init() - Initialized EVE, loading the C code. This method can fail
# 3.  registerFaceID() - Sends a command to register a face for FaceID
# 4.  clearFaceID() - Sends a command to clear all registered faces for FaceID
# 5.  isFpgaEnabled() - Returns whether or not the FPGA processing is enabled
# 6.  isUsingMetadata() - Returns whether or not the metadata camera is used (e.g. Sensing mode)
# 7.  enableManualImageMetadata() - Enables or disables sending manual images with metadata
# 8.  configure() - Configures EVE features (local pipeline -- not available for EVE Light)
# 9.  configureFpga() - Configures FPGA features
# 10. get_frame_id() - Returns the current frame ID
# 11. get_json() - Returns the latest JSON metadata as a dictionary
# 12. get_json_str() - Returns the latest JSON metadata as a string
# 13. get_image_jpg() - Returns the latest processed image as a JPG byte array
# 14. get_image() - Returns the latest processed image
# 15. send_image() - Sends an image to EVE for processing (when not using EVE captured images)
# 16. update_image() - Updates the latest processed image from EVE and converts if necessary)
# 17. poll_setting() - Polls for a new FPGA setting update
# 18. poll_settings() - Polls for all new FPGA setting updates
# 19. readJson() - Reads the latest JSON metadata from EVE
# 20. stop() - Stops EVE processing
# 21. send_manual_buffer() - Sends a manual FPGA data buffer to EVE
# 22. isManualMetadataEnabled() - Returns whether or not we are drawing metadata on the image
#
# -----------------------------------------------------------------------------

import os
import sys
import cv2
import ctypes
import platform
import numpy as np
import json
from pathlib import Path
from .eve import eve_sdk as sdk
from subprocess import run, CalledProcessError, TimeoutExpired

EVE_CAPTURES_IMAGE = False

FACE_ID_CLEAR = sdk.structs.setting_type_t.CS_COMMAND + 0
FACE_ID_REGISTER = sdk.structs.setting_type_t.CS_COMMAND + 1
FACE_ID_CLEAR_ID = sdk.structs.setting_type_t.CS_COMMAND + 2
FACE_ID_SECONDARY_USERS = sdk.structs.setting_type_t.CS_CUSTOM
            
frames = 0
requested_state = sdk.structs.EveRequestedProcessingState.EVE_REQUESTED_PROCESSING_STATE_CONTINUE
eve_sdk = None
callback = None

class EVEInterface():    
    def __init__(self, evePath, maxWidth, toJpg, copyImage, comport=0, i2cAdapter=0, i2cDevice=0x30, i2cIRQ=26, pipelineVersion=0):
        self._image = None
        self._imageClone = None
        self._json = None
        self._jsonStr = ""
        self._frame_id = 0
        self._fpga_enabled = False
        self._comport = comport
        self._i2cAdapter = i2cAdapter
        self._i2cDevice = i2cDevice
        self._i2cIRQ = i2cIRQ
        self._pipelineVersion = pipelineVersion
        self._evePath = evePath
        self._is_windows = platform.system() == "Windows"
        self._toJpg = toJpg
        self._copyImage = copyImage
        self._maxWidth = maxWidth
        self._fpgaState = {}
        self._fpgaCameraId = -1
        self._metaDataFpgaCameraId = -1
        self._usedCameraId = -1
        self._showManualImageMetadata = True

    def isInitialized(self):
        return eve_sdk != None

    def init(self, useMetadataCamera: bool):    
        print("Initializing EVE")
        if self._is_windows:
            import pythoncom
            pythoncom.CoInitializeEx(pythoncom.COINIT_MULTITHREADED)
            
        global eve_sdk
        global callback
        
        root = Path(os.path.abspath(__file__)).parent
        backup_cwd = os.getcwd()
        os.chdir(self._evePath)
            
        if self._is_windows:
            eve_sdk_path = os.path.join(self._evePath,"EveSDK.dll")
            if not os.path.isfile(eve_sdk_path):
                eve_sdk_path = os.path.join(root.parent.parent,self._evePath,"EveSDK.dll")
            eve_sdk = sdk.EveSDK(eve_sdk_path)
        else:
            eve_sdk_path = os.path.join(self._evePath,"libEveSDK.so")
            if not os.path.isfile(eve_sdk_path):
                eve_sdk_path = os.path.join(self._evePath,"..","lib","libEveSDK.so")
            eve_sdk = sdk.EveSDK(eve_sdk_path)
                
            
        ByteArray512 = ctypes.c_byte * 512
        encoded = os.path.dirname(eve_sdk_path).encode('utf-8')
        pathOverride = ByteArray512(*encoded, *([0] * (512 - len(encoded))))  # zero-pad to 512

        imageProvider = sdk.structs.EveImageProvider.EVE_CAMERA if EVE_CAPTURES_IMAGE else sdk.structs.EveImageProvider.EVE_CLIENT_PROVIDED
        startup_options = sdk.structs.EveStartupParameters(pathOverride=pathOverride, gpuPreference=sdk.structs.EveGpuPreference.EVE_NO_GPU, imageProvider=imageProvider)
        err = eve_sdk.CreateEve(startup_options)
        if (err != sdk.structs.EveError.EVE_ERROR_NO_ERROR):
            print(f"CreateEve error code: {err}")
            sys.exit(err)
            
        
                    
        # From EdgeVisionEngine\AutoSentrySample\AutoSentrySample.cpp            
        i = 0
        while EVE_CAPTURES_IMAGE:
            cameraInfo = eve_sdk.EveGetCamera( i )
            if cameraInfo.error == sdk.structs.EveError.EVE_INVALID_CAMERA_ID or cameraInfo.error == sdk.structs.EveError.EVE_NO_MORE_DATA:
                break
        
            pid = ctypes.cast(cameraInfo.data.pid, ctypes.c_char_p).value
            vid = ctypes.cast(cameraInfo.data.vid, ctypes.c_char_p).value
            if cameraInfo.data.isFpgaCamera == 1:
                if self._metaDataFpgaCameraId == -1 and vid == b'META' and pid == b'DATA':
                    self._metaDataFpgaCameraId = i
                elif self._fpgaCameraId == -1:
                    self._fpgaCameraId = i
            print(i, self._fpgaCameraId, self._metaDataFpgaCameraId, cameraInfo.error, pid, vid)
            if self._fpgaCameraId >= 0 and self._metaDataFpgaCameraId >= 0:
                break
            i += 1
        if self._fpgaCameraId == -1 and self._metaDataFpgaCameraId == -1:
            if EVE_CAPTURES_IMAGE:
                raise RuntimeError("No FPGA camera found")
        elif not EVE_CAPTURES_IMAGE:
            raise RuntimeError("EVE shouldn't find cameras with !EVE_CAPTURES_IMAGE! (Maybe remove libCamera or something..)")

        if EVE_CAPTURES_IMAGE:
            print(f" \n\t\t *** FPGA camera found: {self._fpgaCameraId}, metadata {self._metaDataFpgaCameraId}\n" )
        
        if useMetadataCamera:
            self._usedCameraId = self._metaDataFpgaCameraId
        else:
            self._usedCameraId = self._fpgaCameraId
        
        if EVE_CAPTURES_IMAGE:
            # Default:
            cameraFormat = sdk.structs.CCameraFormat()
            # Note that putting higher values here fails on the RPI,
            # It takes another resolution, aspect ratio is screwed, there's lines added at the wrong place, etc.
            # Since the lowest in EveGetFormats() is 720, using 640 works for Windows, but doesn't fail on linux 
            # (using index 0, which is at least not crashing on startup)
            if self._is_windows:
                cameraFormat.resolution.width = 640
                cameraFormat.resolution.height = 360
            else:
                cameraFormat.resolution.width = 3200
                cameraFormat.resolution.height = 2400
                
            cameraFormat.compareResolution = sdk.structs.EveCompare.EVE_AT_MOST
            cameraFormat.compareFps = sdk.structs.EveCompare.EVE_AT_LEAST
            formats = eve_sdk.EveGetFormats(self._usedCameraId, cameraFormat)
            
            
            filter = None
            if formats.formatsCount > 0:
                for i in range(formats.formatsCount):
                    f = formats.formats[i]
                    print(f"{i}: {f.resolution.width}x{f.resolution.height}")
                    # Doing "at most" here
                    if f.resolution.width <= cameraFormat.resolution.width and f.resolution.height <= cameraFormat.resolution.height:
                        if not filter or (f.resolution.width >= filter.resolution.width and f.resolution.height >= filter.resolution.height):
                            filter = f
                            print(f"\t Switching to {f.resolution.width}x{f.resolution.height}")
                if not filter: 
                    filter = formats.formats[0]
                        
            if not filter:            
                filter = sdk.structs.CCameraFormat()
                filter.resolution.width = cameraFormat.resolution.width
                filter.resolution.height = cameraFormat.resolution.height
            filter.compareResolution = cameraFormat.compareResolution
            filter.compareFps = cameraFormat.compareFps

            print(f"camera selected: ID#{self._usedCameraId}: {filter.resolution.width}x{filter.resolution.height}, Format: {filter.format} @ {filter.fps}FPS")
            
            
            errorCode = eve_sdk.EveSetCamera( self._usedCameraId, filter )
            if errorCode != sdk.structs.EveError.EVE_ERROR_NO_ERROR:
                raise RuntimeError(f"Could't set camera {errorCode}")
                
                
        self.__initFpga(useMetadataCamera=useMetadataCamera)
           
        if EVE_CAPTURES_IMAGE:
            callback = sdk.EveProcessingCallbackFn(self.__eve_callback)
            err = eve_sdk.EveRegisterDataCallback(callback)
            if (err != sdk.structs.EveError.EVE_ERROR_NO_ERROR):
                print(f"EveRegisterDataCallback error code: {err}")
                sys.exit(err)
                
        err = eve_sdk.StartEve()
        if (err != sdk.structs.EveError.EVE_ERROR_NO_ERROR):
            print(f"StartEve error code: {err}")
            sys.exit(err)            
            
        os.chdir(backup_cwd)
        print("EVE initialized")
        
        
    def __initFpga(self, useMetadataCamera: bool):
        fpgaParameters = sdk.structs.CFpgaParameters()
        fpgaParameters.comport = self._comport
        fpgaParameters.forceCameraOn = 1
        fpgaParameters.pipelineVersion = self._pipelineVersion
        
        fpgaParameters.connection = sdk.structs.EveFpgaConnectionType.EVE_FPGA_MANUAL
        
        fpgaParameters.i2cAdapterNumber = self._i2cAdapter
        fpgaParameters.i2cDeviceNumber = self._i2cDevice
        fpgaParameters.i2cIRQPin = self._i2cIRQ
                   
        options = sdk.structs.EveFpgaOptions()
        options.parameters = fpgaParameters
        options = eve_sdk.EveConfigureFpga( options );
        if options.error != sdk.structs.EveError.EVE_ERROR_NO_ERROR:
            raise RuntimeError(f"Could't configure FPGA {options.error}")
            
        self.__enableFpga(True, useMetadataCamera=useMetadataCamera)
        
        typeMask = 0
        settingsMask = 0
        for pt in [sdk.structs.pipeline_config_type_t.PT_FD, sdk.structs.pipeline_config_type_t.PT_LM_FV, sdk.structs.pipeline_config_type_t.PT_FID, sdk.structs.pipeline_config_type_t.PT_PD]:
            typeMask |= ( 1 << pt )
        for st in [sdk.structs.setting_type_t.CS_ENABLED, sdk.structs.setting_type_t.CS_IPS, sdk.structs.setting_type_t.CS_CUSTOM]:
            settingsMask |= ( 1 << st )
        if EVE_CAPTURES_IMAGE:
            print("\t\tTYPE", typeMask, "SETTINGS", settingsMask)
        eve_sdk.QueryFpgaSettings(typeMask, settingsMask, notify=True)
        
            
    def __enableFpga(self, activate: bool, useMetadataCamera: bool):
        if not self.isInitialized():
            self.init(useMetadataCamera)
            
        self._fpga_enabled = activate
        
        fpgaDebugOptions = sdk.structs.EveFpgaDebugOptions()
        fpgaDebugOptions.enableDrawingOnImage = 1 if self._fpga_enabled else 0
        print(f"fpgaDebugOptions.enableDrawingOnImage: {fpgaDebugOptions.enableDrawingOnImage}")
        options = eve_sdk.EveConfigureFpgaDebug( fpgaDebugOptions );
        if options.error != sdk.structs.EveError.EVE_ERROR_NO_ERROR:
            raise RuntimeError(f"Could't configure FPGA {options.error}")
            
    def registerFaceID(self):
        if not eve_sdk:
            raise RuntimeError(f"Eve SDK not initialized")
        
        command = sdk.structs.pipeline_config_t(type=sdk.structs.pipeline_config_type_t.PT_FID, 
            setting=sdk.structs.pipeline_setting_t(
                settingType=FACE_ID_REGISTER,
                value=1))
        eve_sdk.SendSetSetting(command)
    
    def clearFaceID(self):
        if not eve_sdk:
            raise RuntimeError(f"Eve SDK not initialized")
        
        command = sdk.structs.pipeline_config_t(type=sdk.structs.pipeline_config_type_t.PT_FID, 
            setting=sdk.structs.pipeline_setting_t(
                settingType=FACE_ID_CLEAR,
                value=1))
        eve_sdk.SendSetSetting(command)

    def isFpgaEnabled(self):
        if not eve_sdk:
            return False
        return self._fpga_enabled
    
    def isUsingMetadata(self):
        if not eve_sdk:
            return False
        return self._metaDataFpgaCameraId == self._usedCameraId
        
    def enableManualImageMetadata(self, enabled):
        self._showManualImageMetadata = int(enabled)==1
        
    def isManualMetadataEnabled(self):
        return self._showManualImageMetadata
           
    def configure(self, feats):              
        if not eve_sdk:
            raise RuntimeError(f"Eve SDK not initialized")              
        for featureName in feats:
            f = feats[featureName]
            if "enabled" in f:
                enabled = 1 if f["enabled"] else 0
                print(f"{featureName}: {enabled}")
                
                if featureName == "hand_landmarks":
                    eve_sdk.EveConfigureHandGesture(sdk.structs.EveHandGestureOptions(enabled=enabled))
                elif featureName == "person_detection":
                    eve_sdk.EveConfigurePersonDetection(sdk.structs.EvePersonDetectionOptions(enabled=enabled))
                elif featureName == "face_detection":
                    mode = sdk.structs.EveFaceTrackerMinimumMode.EVE_FACETRACKER_MINIMUM_MODE_AVERAGE if enabled else sdk.structs.EveFaceTrackerMinimumMode.EVE_FACETRACKER_MINIMUM_MODE_OFF
                    eve_sdk.EveConfigureFaceTracker(sdk.structs.EveFaceTrackerOptions(faceTrackerMode=mode))
                elif featureName == "face_id":
                    # TODO: GalleryPath?
                    eve_sdk.EveConfigureFaceId(sdk.structs.EveFaceIdOptions(enabled=enabled))
                elif featureName == "object_detection":
                    eve_sdk.EveConfigureObjectDetection(sdk.structs.EveObjectDetectionOptions(enabled=enabled))

        return True
    def configureFpga(self, feats):
        if not eve_sdk:
            raise RuntimeError(f"Eve SDK not initialized")
        for featureName in feats:
    
            if featureName == "hand_landmarks":
                featureType = sdk.structs.pipeline_config_type_t.PT_HD
            elif featureName == "person_detection":
                featureType = sdk.structs.pipeline_config_type_t.PT_PD
            elif featureName == "face_detection":
                featureType = sdk.structs.pipeline_config_type_t.PT_FD
            elif featureName == "face_validation":
                featureType = sdk.structs.pipeline_config_type_t.PT_LM_FV
            elif featureName == "face_id":
                featureType = sdk.structs.pipeline_config_type_t.PT_FID
            elif featureName == "face_id_multi":
                featureType = sdk.structs.pipeline_config_type_t.PT_FID                    
                f = feats[featureName]
                
                FACE_ID_SECONDARY_USERS = sdk.structs.setting_type_t.CS_CUSTOM + 0
                if "enabled" in f:
                    enabled = 1 if f["enabled"] else 0
                    print(f"{featureName}: {enabled}")
                    
                    # TODO: Not for all features all the time, only the ones that changed.
                    command = sdk.structs.pipeline_config_t(type=featureType, 
                        setting=sdk.structs.pipeline_setting_t(
                            settingType=FACE_ID_SECONDARY_USERS,
                            value=1 if enabled else 0))
                    eve_sdk.SendSetSetting(command)
                continue
            else:
                print(f"Unknown feature name: '{featureName}'")
                return False
            
            f = feats[featureName]
            if "enabled" in f:
                enabled = 1 if f["enabled"] else 0
                print(f"{featureName}: {enabled}")
                
                # TODO: Not for all features all the time, only the ones that changed.
                command = sdk.structs.pipeline_config_t(type=featureType, 
                    setting=sdk.structs.pipeline_setting_t(
                        settingType=sdk.structs.setting_type_t.CS_ENABLED,
                        value=1 if enabled else 0))
                eve_sdk.SendSetSetting(command)
            if "max_ips" in f:
                ips = int(f["max_ips"])
                print(f"{featureName} IPS: {ips}")
                
                command = sdk.structs.pipeline_config_t(type=featureType, 
                    setting=sdk.structs.pipeline_setting_t(
                        settingType=sdk.structs.setting_type_t.CS_IPS,
                        value=ips))
                eve_sdk.SendSetSetting(command)
                
        return True        

    def get_frame_id(self):  
        return self._frame_id
        
    def get_json(self):
        return self._json
        
    def get_json_str(self):
        return self._jsonStr
        
    def get_image_jpg(self):
        return self._image
        
    def get_image(self):
        return self._imageClone
        
    def send_image(self, frame):
        if not self._showManualImageMetadata:
            return frame
        if EVE_CAPTURES_IMAGE:
            raise Exception("Cannot send an image manually when EVE captures the images")
        input_image = sdk.structs.EveInputImage()
        input_image.data = frame.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte))
        input_image.width = frame.shape[1]
        input_image.height = frame.shape[0]
        input_image.encoding = sdk.structs.EveVideoFormat.EVE_BGR
    
        # call Eve on the image
        err = eve_sdk.EveSendImageForProcessing(input_image)
        if err != sdk.structs.EveError.EVE_ERROR_NO_ERROR:
            LOGGER.info(f"EveSendImageForProcessing() error code: {err}")
            sys.exit(err)
        self.update_image()
        
        return self._image
        
    def update_image(self):
        processed_image = eve_sdk.EveGetProcessedImage()
        if processed_image.error != sdk.structs.EveError.EVE_ERROR_NO_ERROR:
            print(f"EveGetProcessedImage() error code: {processed_image.error}")
        else:
            img = np.ctypeslib.as_array(processed_image.data, shape=(processed_image.height, processed_image.width, processed_image.channels)).astype(np.uint8)
            
            if processed_image.channels == 2:
                img = cv2.cvtColor(img, cv2.COLOR_YUV2BGR_YUYV)
                
            if img.shape[2] == 1 or img.shape[2] == 3:
                # Rescale to self._maxWidth max width
                if self._maxWidth > 0:
                    scaleFactor=self._maxWidth/processed_image.width
                    if scaleFactor < 1:
                        img = cv2.resize(img, (0,0), fx=scaleFactor, fy=scaleFactor, interpolation=cv2.INTER_AREA)
                    
                if self._toJpg:
                    self._image = cv2.imencode('.jpg', img)[1].tobytes()
                else:
                    self._image = img
                if self._copyImage:
                    self._imageClone = img.copy()
                self._frame_id += 1
            
        
    def __eve_callback(self, return_data):
        self.readJson()
        self.update_image()
        return_data.contents.requestedState = requested_state

    def poll_setting(self):
        if not eve_sdk:
            raise RuntimeError(f"Eve SDK not initialized")
        setting = eve_sdk.PopQueuedSetting()
        if setting.message.serialStatus != sdk.structs.EveFpgaSerialStatus.EVE_FPGA_SUCCESS or setting.message.responseType == sdk.structs.response_type_t.RT_NONE:
            return None
        
        return setting
        
    def poll_settings(self):
        if not eve_sdk:
            raise RuntimeError(f"Eve SDK not initialized")
        setting = self.poll_setting()
        while setting:            
            print(f"\t\t--------setting response type: -- {setting.message.responseType} -- pipeline  {setting.type}, setting {setting.setting}, value {setting.value}")            
            
            if not setting.type in self._fpgaState and setting.type < sdk.structs.pipeline_config_type_t.PT_SIZE:
                self._fpgaState[sdk.structs.pipeline_config_type_t(setting.type)] = {}
            feature = self._fpgaState[setting.type]
            #if not setting.setting in feature:
            if setting.setting < sdk.structs.setting_type_t.CS_MAX:
                feature[sdk.structs.setting_type_t(setting.setting)] = setting.value
            setting = self.poll_setting()
        
    def readJson(self):
        if not eve_sdk:
            raise RuntimeError(f"Eve SDK not initialized")
        fpgaJson = eve_sdk.FpgaReadJson()
        if fpgaJson.textStart:
            jsonStr = ctypes.string_at(fpgaJson.textStart, fpgaJson.textSize)
            self._jsonStr = jsonStr
            dataJson = json.loads(jsonStr)
            self._json = dataJson

    def stop(self):
        if not eve_sdk:
            raise RuntimeError(f"Eve SDK not initialized")
        print("Stopping EVE")
        if self._is_windows:
            import pythoncom
            pythoncom.CoUninitialize()
        global requested_state
        requested_state = sdk.structs.EveRequestedProcessingState.EVE_REQUESTED_PROCESSING_STATE_STOP

    def send_manual_buffer(self, buf):
        if buf:            
            c_array = (ctypes.c_ubyte * len(buf))(*buf)
            if self._fpga_enabled:
                fpgaManualData = sdk.structs.EveFpgaManualData(c_array, len(buf))
                error = eve_sdk.EveSendFpgaDataManually(fpgaManualData)                
                if error != sdk.structs.EveError.EVE_ERROR_NO_ERROR:
                    print(f"EveSendFpgaDataManually error code: {error}")
