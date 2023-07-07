#include "utils.h"


void stereo_to_mono(void *const mono, const void *const stereo,
				    const size_t number_of_samples) {
	float *mono_ptr   = (float *) mono;
	int16_t *stereo_ptr = (int16_t *) stereo;

	for (size_t item = 0; item < number_of_samples; item++) {
		mono_ptr[item] = 2 * 0.5f * (stereo_ptr[item * 2] + stereo_ptr[item * 2 + 1]) / 32767.f;
	}
}



float* pass_buffer_to_output(float *buffer, uint8_t *buffer_half) {
	if (!*buffer_half) {	// First half
		*buffer_half = 1;
		return &buffer[0];
	}
	*buffer_half = 0;
	return &buffer[N_SAMPLES];
}
