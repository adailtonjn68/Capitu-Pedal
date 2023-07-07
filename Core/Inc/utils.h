#ifndef UTILS_H_
#define UTILS_H_


#include <stdint.h>
#include <stddef.h>
#include "common.h"


void stereo_to_mono(void *const mono, const void *const stereo,
				    const size_t number_of_samples);
float* pass_buffer_to_output(float *buffer, uint8_t *buffer_half); 

#endif /* UTILS_H_ */
