//=============================================================================
//
// Copyright(c) 2025 Mirametrix Inc. All rights reserved.
//
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by MMX and
// are protected by copyright law. They may not be disclosed
// to third parties or copied or duplicated in any form, in whole or
// in part, without the prior written consent of MMX.
//
//=============================================================================


//=============================================================================
// I N C L U D E   F I L E S   A N D   F O R W A R D   D E C L A R A T I O N S

#include "defect_detection_module.h"
#include "app_module.h"
#include "memmap.h"
#include "network_info.h"
#include "fw_core.h"
#include "ospi_support.h"
#include "fw_globals.h"
#include "assert.h"
#include "camera_config.h"
#include "utils.h"


//=============================================================================
// S T R U C T   D E C L A R A T I O N S

#define APP_MODULE_OUTPUT_SIZE    34  // bytes

typedef struct {
    defect_detection_t defectDetection;
    struct network_info defectDetectionNetworkInfo;
    defect_detection_result_t defectDetectionResult;
    unsigned char output[APP_MODULE_OUTPUT_SIZE + 3]; // +3 for start flag and length
    uint8_t streamComplete;
    uint32_t skipOutputNb;
} app_module_context_t;


const uint32_t DEFECT_DETECTION_NETWORK_INPUT_ADDR              = ML_APP_MOD_INPUT_START_ADDRESS;     // Use in SensAI compiler
const uint32_t DEFECT_DETECTION_NETWORK_OUTPUT_ADDR             = ML_APP_MOD_INPUT_START_ADDRESS;     // Use in SensAI compiler
const uint32_t DEFECT_DETECTION_NETWORK_REF_VECTOR_OFFSET       = 0;

const pixel_box_t SOURCE_IMAGE_ROI = CreateLiteralPixelBox( 0, 0, 3280, 2464 );

struct networks NETWORK_LISTS = {
    .count_of_networks = 0,
    .networks = NULL,
};


//=============================================================================
// F U N C T I O N S   C O D E   S E C T I O N

//-----------------------------------------------------------------------------
//
app_handle_t app_preinit(void)
{
    static app_module_context_t appCtxt;

    appCtxt.defectDetection = InitDefectDetection();

    return (app_handle_t)&appCtxt;
}


//-----------------------------------------------------------------------------
//
app_handle_t app_init(app_handle_t appContext)
{
    app_module_context_t *appCtxt = (app_module_context_t *)appContext;
    appCtxt->streamComplete = true;
    appCtxt->skipOutputNb = 0;

    // Add reference vectors
    int16_t __attribute__((aligned(4))) rawRefVector[DEFECT_DETECTION_VECTOR_SIZE_16B];
    for(uint8_t i=0; i < DEFECT_DETECTION_NB_REF_IMAGE; ++i)
    {
        uint32_t bytesNb = read_module_data( 
            0x5001, 
            DEFECT_DETECTION_NETWORK_REF_VECTOR_OFFSET + i*DEFECT_DETECTION_VECTOR_SIZE_16B*2, 
            DEFECT_DETECTION_VECTOR_SIZE_16B*2, 
            (uint8_t *)rawRefVector);
        if(bytesNb == DEFECT_DETECTION_VECTOR_SIZE_16B*2)
        { 
            AddReferenceVector( &appCtxt->defectDetection, rawRefVector );
        }
    }
    
    // Read threshold
    int32_t __attribute__((aligned(4))) rawThreshold;
    uint32_t bytesNb = read_module_data( 
        0x5001, 
        DEFECT_DETECTION_NETWORK_REF_VECTOR_OFFSET + DEFECT_DETECTION_NB_REF_IMAGE*DEFECT_DETECTION_VECTOR_SIZE_16B*2, 
        4, 
        (uint8_t *)&rawThreshold);
    
    if(bytesNb == 4)
    {
        UpdateThreshold( &appCtxt->defectDetection, rawThreshold );
    }

    GARD__ASSERT(appCtxt->defectDetection.output.nbRegisteredVectors > 0, "No reference vectors registered");

    // Add network
    struct network_info network =
    {        
        .network       = 0x2001,
        .inout_offset  = DEFECT_DETECTION_NETWORK_INPUT_ADDR,
        .inout_size    = 
            appCtxt->defectDetection.input.scalerConfig.roi.dimensions.width 
            * appCtxt->defectDetection.input.scalerConfig.roi.dimensions.height 
            * appCtxt->defectDetection.input.scalerConfig.nbChannels,
    };
    appCtxt->defectDetectionNetworkInfo = network;

    // Register network
    NETWORK_LISTS.count_of_networks = 1;
    NETWORK_LISTS.networks = &appCtxt->defectDetectionNetworkInfo;
    register_networks(&NETWORK_LISTS);

    // Schedule network once
    schedule_network_to_run(appCtxt->defectDetectionNetworkInfo.network);

    start_camera_streaming();

    capture_image_async();

    return appContext;
}


//-----------------------------------------------------------------------------
//
enum app_ret_code app_preprocess(app_handle_t appContext, void *imageData)
{
    start_ml_engine();
    return APP_CODE__SUCCESS;
}


//-----------------------------------------------------------------------------
//
static void AppendAppData(uint8_t *outputPointer, size_t *indexPointer, const void *data, size_t dataSize) {
    memcpy(&outputPointer[*indexPointer], data, dataSize);
    *indexPointer += dataSize;
}


//-----------------------------------------------------------------------------
//
static void SendAppData(app_module_context_t *ctxt) {
	size_t index = 0;
    ctxt->output[index] = 0x7e;                             // start flag
    index++;

    ctxt->output[index] = APP_MODULE_OUTPUT_SIZE;
    index++;
    ctxt->output[index] = 0;
    index++;

    ctxt->output[index] = 1;                                // RT_DATA
    index++;
    ctxt->output[index] = 0x64;                             // RT_DATA version
    index++;

    AppendAppData(ctxt->output, &index, &SOURCE_IMAGE_ROI.dimensions.width, sizeof(SOURCE_IMAGE_ROI.dimensions.width));
    AppendAppData(ctxt->output, &index, &SOURCE_IMAGE_ROI.dimensions.height, sizeof(SOURCE_IMAGE_ROI.dimensions.height));
    AppendAppData(ctxt->output, &index, &SOURCE_IMAGE_ROI.left, sizeof(SOURCE_IMAGE_ROI.left));
    AppendAppData(ctxt->output, &index, &SOURCE_IMAGE_ROI.top, sizeof(SOURCE_IMAGE_ROI.top));
    AppendAppData(ctxt->output, &index, &SOURCE_IMAGE_ROI.right, sizeof(SOURCE_IMAGE_ROI.right));
    AppendAppData(ctxt->output, &index, &SOURCE_IMAGE_ROI.bottom, sizeof(SOURCE_IMAGE_ROI.bottom));

    AppendAppData(ctxt->output, &index, &ctxt->defectDetectionResult.score.n, sizeof(ctxt->defectDetectionResult.score.n));
    uint32_t isDefective = ctxt->defectDetectionResult.isDefective ? 1 : 0;  // bool as 4 bytes
    AppendAppData(ctxt->output, &index, &isDefective, sizeof(isDefective));

    uint8_t MAX_SKIP_OUTPUT_NB = 9;
    if( ctxt->streamComplete || ctxt->skipOutputNb > MAX_SKIP_OUTPUT_NB)
    {
        ctxt->streamComplete = false;
        ctxt->skipOutputNb = 0;
        stream_data_to_host_async(ctxt->output, index, 100, &ctxt->streamComplete);
    }
    else
    {
        ctxt->skipOutputNb += 1;
    }
}


//-----------------------------------------------------------------------------
//
enum app_ret_code app_ml_done(app_handle_t appContext, void *mlResults)
{
    app_module_context_t *ctxt = (app_module_context_t *)appContext;

    int16_t *outputVector = (int16_t *)mlResults;
    ctxt->defectDetectionResult = FinishDefectDetection(&ctxt->defectDetection, outputVector);

    capture_image_async();

    SendAppData(ctxt);

    return APP_CODE__SUCCESS;
}


//-----------------------------------------------------------------------------
//
enum app_ret_code app_image_processing_done(app_handle_t appContext)
{
    return APP_CODE__SUCCESS;
}


//-----------------------------------------------------------------------------
//
enum app_ret_code app_rescale_done(app_handle_t app_context)
{
    return APP_CODE__SUCCESS;
}

//-----------------------------------------------------------------------------
//
DEFINE_APP_MODULE_CALLBACKS( 
    app_preinit,
    app_init,
    app_preprocess,
    app_ml_done,
    app_image_processing_done,
    app_rescale_done );
