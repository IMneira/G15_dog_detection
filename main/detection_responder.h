

// Provides an interface to take an action based on the output from the person
// detection model.

#ifndef TENSORFLOW_LITE_MICRO_EXAMPLES_PERSON_DETECTION_DETECTION_RESPONDER_H_
#define TENSORFLOW_LITE_MICRO_EXAMPLES_PERSON_DETECTION_DETECTION_RESPONDER_H_

#include "tensorflow/lite/c/common.h"

void RespondToDetection(float no_dog_score);

void setup_ledc_servo();


#endif  // TENSORFLOW_LITE_MICRO_EXAMPLES_PERSON_DETECTION_DETECTION_RESPONDER_H_
