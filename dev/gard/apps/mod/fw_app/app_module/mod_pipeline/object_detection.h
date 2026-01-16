/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#ifndef OBJECT_DETECTION_H
#define OBJECT_DETECTION_H

#include <stdint.h>

#include "box.h"
#include "fixed_point.h"

typedef enum{
    Person = 0,
    Bicycle,
    Car,
    Motorcycle,
    Bus,
    Truck,
    TrafficLight,
    StopSign
} ObjectClass;

static const int32_t OBJECT_DETECTION_CLASSES_NB = StopSign + 1;

static const int32_t RAW_DATA_OUTPUT_COORDS_ADDRESSES[] = {
    0x807ff500,
    0x807f1420,
    0x807d1800
    // 0x807ff508,
    // 0x807f1420,
    // 0x807d1800
};

static const int32_t RAW_DATA_OUTPUT_CONFIDENCE_ADDRESSES[] = {
    0x807ff860,
    0x807f21a0,
    0x8079be00
    // 0x807ff868,
    // 0x807f21a0,
    // 0x8079be00
};

// 48x36 resolution not used because the network is apparently not compiled properly
static const size_t RESOLUTION_LAYERS_NB = 3;

struct ObjectDetectionPostprocessingConfig {
    int32_t maxBoxes; // Max number of boxes to select before deduplication
    fp_t confidenceThreshold; // Value above which an object will be selected
    fp_t IoUThreshold; // IoU value above which two objects will be deemed the same
};

struct ObjectDetectionOutput{
    fp_t *confidences;      // Detected objects confidence
    geometric_box_t *boxes; // Detected objects bounding boxes
    ObjectClass *classes;   // Detected object classes
};

struct ObjectDetectionData
{
    int32_t objectClass;
    int32_t confidence;
    int16_t left;
    int16_t top;
    int16_t right;
    int16_t bottom;
};

// Postprocess object detection network output
int32_t PostprocessObjectDetection(
    struct ObjectDetectionPostprocessingConfig *config,
    struct ObjectDetectionOutput *results );

#endif
