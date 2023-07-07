#include "audio_effects.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>


#define PI		(3.141592654f)

float first_order_lowpass(FirstOrderLP_t *const filter, float x) {

	filter->y = filter->a * filter->y + filter->K * (x + filter->x_prev);

	filter->x_prev = x;

	return filter->y;
}


float first_order_highpass(FirstOrderHP_t *const filter, float x) {
	
	filter->y = filter->a * filter->y + filter->K * (x - filter->x_prev);
	
	filter->x_prev = x;

	return filter->y;
}


float band_pass(BandPass_t *const filter, float x) {
	
	first_order_lowpass(&filter->lowpass, x);
	filter->y = first_order_highpass(&filter->highpass, filter->lowpass.y);
	return filter->y;
}


float band_stop(BandStop_t *const filter, float x) {
	
	first_order_lowpass(&filter->lowpass, x);
	first_order_highpass(&filter->highpass, x);

	filter->y = filter->lowpass.y + filter->highpass.y;

	return filter->y;
}


float flanger(Flanger_t *const _flanger, float x) {
	float delay;
	int32_t i, j;
	float x_i, x_j;
	float x_interpolated;
	
	/* Update index */
	_flanger->index++;
	if (_flanger->index >= _flanger->buffer_size) _flanger->index = 0;
	/* Update buffer with most recent input */
	_flanger->x_delayed[_flanger->index] = x;
	

	delay = (_flanger->buffer_size - 1) * 0.5f * (1.f + _flanger->A * sinf(2.f * PI * 
													_flanger->f * _flanger->k * _flanger->Ts));	

	_flanger->k++;
	if (_flanger->k >= _flanger->k_max) _flanger->k = 0;

	i = delay;
	j = (i + 1) % (_flanger->buffer_size);
    x_i = _flanger->x_delayed[(_flanger->index - i + _flanger->buffer_size) % _flanger->buffer_size];
	x_j = _flanger->x_delayed[(_flanger->index - j + _flanger->buffer_size) % _flanger->buffer_size];
    x_interpolated = x_i + (delay - i) * (x_j - x_i);

	_flanger->y = (1 - _flanger->delay_gain) * x + _flanger->delay_gain * x_interpolated;
	return _flanger->y;
}


float tremolo(Tremolo_t *const _tremolo, float x) {
	float M;

	M = (1.f - _tremolo->depth) + _tremolo->depth * sinf(2.f * PI * _tremolo->f * _tremolo->Ts * _tremolo->k );
	
	_tremolo->y = M * x;

	_tremolo->k++;
	if (_tremolo->k >= _tremolo->k_max) _tremolo->k = 0;

	return _tremolo->y;
}


float distortion(Distortion_t *const _distortion, const float x) {
	if (x > _distortion->max) {
		_distortion->y = _distortion->max;
	}
	else if (x < -_distortion->max) {
		_distortion->y = -_distortion->max;
	}
	else {
		_distortion->y = x;
	}
	return _distortion->y;
}


float second_order_lowpass(SecondOrderLP_t *const filter, float x) {
	
	filter->y_prev_prev = filter->y_prev;
	filter->y_prev = filter->y;

	filter->y = -filter->a * filter->y_prev - filter->b * filter->y_prev_prev + 
			    filter->K * (x + 2 * filter->x_prev + filter->x_prev_prev);
	
	filter->x_prev_prev = filter->x_prev;
	filter->x_prev = x;

	return filter->y;
}


float crossover(Crossover_t *const cross, const float x) {
	float x_mod = fabsf(x);

	if (x_mod < cross->x_threshold) cross->y = cross->gain * x;
	else cross->y = x;

	return cross->y;
}


void first_order_lowpass_init(FirstOrderLP_t *const filter, float freq_cutoff, const float Tsamp) {
	float w_cutoff, Kt;

	/* Set minimum frequency */
	if (freq_cutoff < 1) freq_cutoff = 1;

	w_cutoff = 2.f * PI * freq_cutoff;
	Kt = 2.f / Tsamp;
	
	filter->K = w_cutoff / (w_cutoff + Kt);
	filter->a = (Kt - w_cutoff) / (w_cutoff + Kt);
}


void first_order_highpass_init(FirstOrderHP_t *const filter, float freq_cutoff, const float Tsamp) {
	float w_cutoff, Kt;
	
	/* Set minimum frequency */
	if (freq_cutoff < 1) freq_cutoff = 1;

	w_cutoff = 2.f * PI * freq_cutoff;
	Kt = 2.f / Tsamp;
	
	filter->K = Kt / (Kt + w_cutoff);
	filter->a = (Kt - w_cutoff) / (Kt + w_cutoff);
}


void bandpass_init(BandPass_t *const filter, float freq_cut_1, float freq_cut_2, const float Tsamp) {
	float freq_cut_low, freq_cut_high;

	/* Set a minimum frequency */
	if (freq_cut_1 < 1) freq_cut_1 = 1;
	if (freq_cut_2 < 1) freq_cut_2 = 1;

	if (freq_cut_1 < freq_cut_2) {
		freq_cut_low = freq_cut_1;
		freq_cut_high = freq_cut_2;
	}
	else {
		freq_cut_low = freq_cut_2;
		freq_cut_high = freq_cut_1;
	}

	first_order_lowpass_init(&filter->lowpass, freq_cut_low, Tsamp);
	first_order_highpass_init(&filter->highpass, freq_cut_high, Tsamp);
}


void bandstop_init(BandStop_t *const filter, float freq_cut_1, float freq_cut_2, float Tsamp) {
	float freq_cut_low, freq_cut_high;

	/* Set a minimum frequency */
	if (freq_cut_1 < 1) freq_cut_1 = 1;
	if (freq_cut_2 < 1) freq_cut_2 = 1;

	if (freq_cut_1 < freq_cut_2) {
		freq_cut_low = freq_cut_1;
		freq_cut_high = freq_cut_2;
	}
	else {
		freq_cut_low = freq_cut_2;
		freq_cut_high = freq_cut_1;
	}

	first_order_lowpass_init(&filter->lowpass, freq_cut_low, Tsamp);
	first_order_highpass_init(&filter->highpass, freq_cut_high, Tsamp);
}



void flanger_init(Flanger_t *const _flanger, const uint16_t buffer_size, const float excursion, const float delay_gain, const float mod_freq, const float Tsamp) {

	_flanger->Ts = Tsamp;
	_flanger->buffer_size = buffer_size;
	_flanger->A = excursion;
	_flanger->delay_gain = delay_gain;
	_flanger->f = mod_freq;
	_flanger->k_max = 1.f / Tsamp;
	_flanger->k = 0;
	_flanger->index = 0;
	_flanger->x_delayed = malloc(buffer_size * sizeof(float));
	memset(_flanger->x_delayed, 0, buffer_size * sizeof(float));
}


void flanger_terminate(Flanger_t *const _flanger) {
	free(_flanger->x_delayed);
}

void tremolo_init(Tremolo_t *const _tremolo, float depth, float mod_freq, const float Tsamp) {
	_tremolo->Ts = Tsamp;
	
	if (depth < 0) depth = 0;
	else if (depth > 1) depth = 1;
	_tremolo->depth = depth;

	_tremolo->k_max = 1.f / Tsamp;
	_tremolo->f = mod_freq;
	_tremolo->k = 0;
}


void distortion_init(Distortion_t *const _distortion, float max) {
	if (max < 0) max = -1 * max;

	_distortion->max = max;

}


void second_order_lowpass_init(SecondOrderLP_t *const filter, float zeta, float freq, float Tsamp) {
	float wc, Kt, denominator;

	wc = 2.f * PI * freq;
	Kt = 2.f / Tsamp;

	denominator = Kt * Kt + 2.f * zeta * wc * Kt + wc * wc;

	filter->K = wc * wc / denominator;
	filter->a = 2.f * (wc * wc - Kt * Kt) / denominator;
	filter->b = (Kt * Kt - 2.f * zeta * wc * Kt + wc * wc) / denominator;

}


void crossover_init(Crossover_t *const cross, const float x_threshold, const float gain) {
	cross->x_threshold = x_threshold;
	cross->gain = gain;
	cross->y = 0.f;
}


void crossover2_init(Crossover2_t *const cross, float xp, float yp, float slope) {

    crossover2_params_update(cross, xp, yp, slope);

    cross->y = 0;
}


void crossover2_params_update(Crossover2_t *const cross, float xp, float yp, float slope) {

    if (xp > 1.f) xp = 1.f;
    else if (xp < 0.f) xp = 0.f;

    if (yp > 1.f) yp = 1.f;
    else if (yp < 0.f) yp = 0.f;

    cross->A = yp / (expf(xp) - 1.f);

    cross->xp = xp;
    cross->yp = yp;
    cross->slope = slope;
}


float crossover2(Crossover2_t *const cross, const float x) {
    float gain;
    float x_abs = fabsf(x);

    if (x_abs < cross->xp) {
        gain = cross->A * (expf(x_abs) - 1);
    }
    else {
        gain = cross->yp + cross->slope * (x_abs - cross->xp);
    }

    cross->y = gain * x;
    return cross->y;
}

void bitcrusher_init(Bitcrusher_t *const pedal, const uint16_t A, const float depth, const uint16_t decimation) {
  pedal->x_integer = 0;
  pedal->y = 0.f;
  pedal->i = 0;

  bitcrusher_params_update(pedal, A, depth, decimation);

}
void bitcrusher_params_update(Bitcrusher_t *const pedal, const uint16_t A, const float depth, const uint16_t decimation) {
  
  pedal->A = A;
  if (pedal->A == 0) {
    pedal->A = 1;
  }
  
  pedal->depth = depth;
  if (pedal->depth < 0.f) pedal->depth = 0.f;
  else if (pedal->depth > 1.f) pedal->depth = 1.f;

  pedal->decimation = decimation;
  if (pedal->decimation == 0) {
    pedal->decimation = 1;
  }
}


float bitcrusher(Bitcrusher_t *const pedal, const float x) {

  if (pedal->i >= pedal->decimation) {
    pedal->x_integer = pedal->A * x;
    pedal->i = 0;
  }
  else {
    pedal->i++;
  }

  pedal->y = (1.f - pedal->depth) * x + pedal->depth * ((float) pedal->x_integer) / (float) pedal->A; 
  return pedal->y;

}
