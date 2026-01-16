#include "stdint.h"
#include "fixed_point.h"
#include "box.h"
#include "frame_data.h"


#define FACE_DETECTION_CAP 5


int32_t FaceDetection(
	uint32_t FACE_DETECTION_NETWORK_OUTPUT_ADDR,
    fp_t *confidence,
    geometric_box_t *boxes,
	frame_data_t *frameData);