#ifndef AUDIO_EFFECTS_H_
#define AUDIO_EFFECTS_H_


#include <stdint.h>
#include "common.h"



typedef struct {
	float x_prev;
	float y;
	float K, a;
} FirstOrderLP_t;


typedef struct {
	float K, a, b;
	float x_prev, x_prev_prev;
	float y_prev, y_prev_prev;
	float y;
} SecondOrderLP_t;


typedef struct {
	float x_prev;
	float y;
	float K, a;
} FirstOrderHP_t;


typedef struct {
	FirstOrderLP_t lowpass;
	FirstOrderHP_t highpass;
	float y;
} BandPass_t;


typedef struct {
	FirstOrderLP_t lowpass;
	FirstOrderHP_t highpass;
	float y;
} BandStop_t;


typedef struct {
	uint16_t buffer_size;
	uint16_t index;
	float A;				// Excursion = maximum delay swing in interval [0, 1]
	float delay_gain;		// in interval [0, 1]
	float Ts;
	float f;				// rate or speed
	uint32_t k;
	uint32_t k_max;
	float *x_delayed;	// Circular buffer
	float y;
} Flanger_t;


typedef struct {
	float Ts;
	float f;
	uint32_t k;
	uint32_t k_max;
	float y;
	float depth;
} Tremolo_t;


typedef struct {
	float max;
	float y;
} Distortion_t;


typedef struct {
	float x_threshold;
	float gain;
	float y;
} Crossover_t;


typedef struct {
    float A, slope;
    float xp, yp;
    float y;
} Crossover2_t;


typedef struct {
  int32_t x_integer;
  uint16_t A;
  float depth;
  float y;
  uint16_t decimation;
  uint16_t i;
} Bitcrusher_t;


void first_order_lowpass_init(FirstOrderLP_t *const filter, float freq_cutoff, const float Tsamp);
float first_order_lowpass(FirstOrderLP_t *const filter, float x);

void first_order_highpass_init(FirstOrderHP_t *const filter, float freq_cutoff, const float Tsamp);
float first_order_highpass(FirstOrderHP_t *const filter, float x); 

void bandpass_init(BandPass_t *const filter, float freq_center, float bandwidth, const float Tsamp);
float band_pass(BandPass_t *const filter, float x); 

void bandstop_init(BandStop_t *const filter, float freq_center, float bandwidth, const float Tsamp);
float band_stop(BandStop_t *const filter, float x); 

void flanger_init(Flanger_t *const _flanger, const uint16_t buffer_size, const float excursion, const float delay_gain, const float mod_freq, const float Ts);
float flanger(Flanger_t *const flanger, float x);
void flanger_terminate(Flanger_t *const _flanger);

void tremolo_init(Tremolo_t *const _tremolo, float depth, float mod_freq, const float Tsamp);
float tremolo(Tremolo_t *const _tremolo, float x);

void distortion_init(Distortion_t *const _distortion, float max);
float distortion(Distortion_t *const _distortion, const float x);

void second_order_lowpass_init(SecondOrderLP_t *const filter, float zeta, float freq, float Tsamp);
float second_order_lowpass(SecondOrderLP_t *const filter, float x);

void crossover_init(Crossover_t *const cross, const float x_threshold, const float gain);
float crossover(Crossover_t *const cross, const float x);

void crossover2_init(Crossover2_t *const cross, float xp, float yp, float slope);
void crossover2_params_update(Crossover2_t *const cross, float xp, float yp, float slope); 
float crossover2(Crossover2_t *const cross, const float x);

void bitcrusher_init(Bitcrusher_t *const pedal, const uint16_t A, const float depth, const uint16_t decimation);
void bitcrusher_params_update(Bitcrusher_t *const pedal, const uint16_t A, const float depth, const uint16_t decimation);
float bitcrusher(Bitcrusher_t *const pedal, const float x);



#endif /* AUDIO_EFFECTS_H_ */
