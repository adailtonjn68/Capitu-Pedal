#include "common.h"
#include <stdint.h>
#include <stddef.h>



_Atomic int play_flag;
_Atomic int window_flag;

float buffer[N_SAMPLES * 2];		// Double circular buffer will be used
uint8_t buffer_half = 0;		// 0: to stream first half, and copy second;
								// 1: to stream second hald, and copy first.
float *buffer_ptr;
size_t audio_file_index = 0;


float buffer_plot_input[N_SAMPLES * PLOT_DECIMATION];
float buffer_plot_output[N_SAMPLES * PLOT_DECIMATION];
_Atomic int plotting = 0;
_Atomic int writing_buffer_plot = 0;
_Atomic int buffer_ready_to_plot = 0;

struct Effects_config tremolo_conf = {.status = 0, .height = 2,
                                      .values = {.2, 10.}, .n_args = 2};
struct Effects_config distortion_conf = {.status = 0, .height = 2,
                                         .values={.3}, .n_args = 1 };
struct Effects_config bitcrusher_conf = {.status = 0, . height = 2,
                                         .values = {127, .5, 10}, .n_args = 3};
struct Effects_config flanger_conf = {.status = 0, . height = 2,
                                      .values = {700, .2, .25, .2}, .n_args = 4};
