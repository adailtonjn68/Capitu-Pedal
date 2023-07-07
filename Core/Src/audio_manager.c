#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sndfile.h>
#include "audio_manager.h"
#include "audio_effects.h"
#include "common.h"
#include "utils.h"


extern int play_flag;
extern int window_flag;

extern float buffer[N_SAMPLES *2];		// Double circular buffer till be used
extern uint8_t buffer_half;		// 0: to stream first half, and copy second;
								// 1: to stream second hald, and copy first.
extern float *buffer_ptr;
extern size_t audio_file_index;

extern enum Effects_type effects_order[NUMBER_OF_EFFECTS];

extern struct Effects_config tremolo_conf;
extern struct Effects_config distortion_conf;
extern struct Effects_config bitcrusher_conf ;
extern struct Effects_config flanger_conf;

extern float buffer_plot_input[N_SAMPLES * PLOT_DECIMATION];
extern float buffer_plot_output[N_SAMPLES * PLOT_DECIMATION];
extern _Atomic int plotting;
extern _Atomic int buffer_ready_to_plot;


const float Ts = 1.f / 48000.f;
FirstOrderLP_t lp;
FirstOrderHP_t hp;
BandPass_t bp;
BandStop_t bs;
SecondOrderLP_t lp2;
Flanger_t flgr;
Tremolo_t trml;
Distortion_t distort;
Crossover_t cross;
Crossover2_t cross2;
Bitcrusher_t bcrush;


void apply_effects(float *const buffer_to_process) {
	float *sample_ptr;

	for (uint32_t i = 0; i < N_SAMPLES; i++) {
		sample_ptr = buffer_to_process + i;
		for (size_t effect_index = 0; effect_index < NUMBER_OF_EFFECTS; effect_index++) {
            if (effects_order[effect_index] == TREMOLO && tremolo_conf.status) {
		        *sample_ptr = tremolo(&trml, *sample_ptr);
            }
            else if(effects_order[effect_index] == DISTORTION && distortion_conf.status) {
                *sample_ptr = distortion(&distort, *sample_ptr);
            }
            else if (effects_order[effect_index] == FLANGER && flanger_conf.status) {
                *sample_ptr = flanger(&flgr, *sample_ptr);
            }
            else if (effects_order[effect_index] == BITCRUSHER && bitcrusher_conf.status) {
                *sample_ptr = bitcrusher(&bcrush, *sample_ptr);
            }
        }
    }

}



void process_buffer(size_t audio_index) {
    static int plot_index = 0;
    float *buffer_to_process, *buffer_to_copy_to;
	/* Process first half and copy data to second half */
	if (!buffer_half) {
		/* Process first */
		buffer_to_process = &buffer[0];
		/* Copy second half */
		buffer_to_copy_to = &buffer[N_SAMPLES];
	}
	else {
		/* Process second half */
		buffer_to_process = &buffer[N_SAMPLES];
		/* Copy first half */
		buffer_to_copy_to = &buffer[0];
	}

	/* Copy input to double buffer */
	memcpy(buffer_to_copy_to, buffer_ptr + audio_index, N_SAMPLES * sizeof(float));

    /* Process other half of double buffer */  
    if (plotting) {
        apply_effects(buffer_to_process);
    }
    else {

        buffer_ready_to_plot = 0;
        memcpy(&buffer_plot_input[plot_index], buffer_to_process,
               N_SAMPLES * sizeof(buffer_to_process[0]));

        apply_effects(buffer_to_process);
        
        memcpy(&buffer_plot_output[plot_index], buffer_to_process,
               N_SAMPLES * sizeof(buffer_to_process[0]));
        plot_index += N_SAMPLES;
        if (plot_index >= N_SAMPLES * PLOT_DECIMATION) {
            buffer_ready_to_plot = 1;
            plot_index = 0;
        }
    }
}


int stream_callback(const void *input_buffer, void *output_buffer,
					unsigned long frames_per_buffer,
					const PaStreamCallbackTimeInfo *timeinfo,
					PaStreamCallbackFlags status_flags, void *user_data) {

	(void) input_buffer;
	(void) timeinfo;
	(void) status_flags;
	(void) user_data;

	float *out = (float *) output_buffer;

	process_buffer(audio_file_index);
	float *buffer_to_pass = pass_buffer_to_output(buffer, &buffer_half);

	for (size_t i = 0; i < frames_per_buffer; i++) {
		if (audio_file_index == 9021438) {
			return paAbort;
		}
		*out++ = *buffer_to_pass++;
		audio_file_index++;
	}
	return paContinue;
}



int audio_device_manager(void *arg) {
	(void) arg;
	first_order_lowpass_init(&lp, 500, Ts);
	first_order_highpass_init(&hp, 500, Ts);
	bandpass_init(&bp, 500, 900, Ts);
	bandstop_init(&bs, 500, 900, Ts);
	flanger_init(&flgr, 700, .2, .4, .2, Ts); 
	tremolo_init(&trml, .2, 10., Ts);
	distortion_init(&distort, .15);
	second_order_lowpass_init(&lp2, 0.7, 500, Ts);
	crossover_init(&cross, .1, 3); 
    crossover2_init(&cross2, .3, .3, 1 );
    bitcrusher_init(&bcrush, 127, .5, 10);


	int status = 0;
	PaError stream_status;	
	PaStream *stream;
	PaStreamParameters output_params;

	SNDFILE *audio_file; 
	int16_t *wav_data;
	SF_INFO audio_info = {.format = 0};
	char *audio_name = "./audios/Guitar-Gentle.wav";
	
  FILE *filenull = freopen("/dev/null", "w", stderr);
	/* Initialize PortAudio */
	stream_status = Pa_Initialize();
	if (stream_status != paNoError) {
		printf("Error initializing Portaudio\n");
		fclose(filenull);
    return 1;

	}

	/* Open audio file */
	audio_file = sf_open(audio_name, SFM_READ, &audio_info);
	if (audio_file == NULL) {
		printf("Not possible to open audio file\n");
		status = 1;
		goto error_sf_open;	
	}

#if 0 
	printf("Format = %x\n", audio_info.format);
	printf("Frames = %ld\n", audio_info.frames);
	printf("samplerate = %d\n", audio_info.samplerate);
	printf("channels = %d\n", audio_info.channels);
	printf("sections = %x\n", audio_info.sections);
	printf("seekable = %x\n", audio_info.seekable);
#endif
	
	/* Allocate memory to read audio file data */	
	wav_data = malloc( audio_info.channels * audio_info.frames * sizeof(int16_t));
	if (wav_data == NULL) {
		printf("Not possible to allocate memory to read audio file\n");
		status = 1;
		goto error_wav_malloc;	
	}

	/* Allocate memory for mono audio */
	float *wav_data_mono = malloc(audio_info.frames * sizeof(float)); 
	if (wav_data_mono == NULL) {
		printf("Not possible to allocate memory for wav_data_mono\n");
		status = 1;
		goto error_wav_mono_malloc;
	}

	/* Read audio file data */
	sf_read_int(audio_file, (int *) wav_data, audio_info.frames); 
	/* Audio file is no longer needed */
	sf_close(audio_file);

	/* Convert stereo audio to mono*/
	stereo_to_mono(wav_data_mono, wav_data, audio_info.frames);
	buffer_ptr = wav_data_mono;

	/* Audio stream configuration */	
	output_params.device = Pa_GetDefaultOutputDevice();
	output_params.channelCount = 1;
	output_params.sampleFormat = paFloat32;
	output_params.suggestedLatency = Pa_GetDeviceInfo(output_params.device)->defaultLowOutputLatency;
	output_params.hostApiSpecificStreamInfo = NULL;

  while (window_flag) {
    if (play_flag) {
      /* Open audio stream */ 
	      stream_status = Pa_OpenStream(&stream,
	      							  NULL,		// input parameters 
	      							  &output_params,
	      							  audio_info.samplerate,
	      							  N_SAMPLES,		// frames per buffer (samples per buffer)
	      							  paClipOff,
	      							  &stream_callback,
	      							  NULL);
	      if (stream_status != paNoError) {
	      	printf("Not possible to open stream\n");
	      	status = 1;
	      	goto error_pa_open_stream;
	      }

	      /* Start stream */
	      stream_status = Pa_StartStream(stream);
	      if (stream_status != paNoError) {
	      	printf("Unable to start stream\n");
	      	status = 1;
	      	goto error_pa_open_stream;
	      }

        while (play_flag) {}
	      Pa_CloseStream(stream);
    }
  }
error_pa_open_stream:
	flanger_terminate(&flgr);
	Pa_Terminate();
	free(wav_data_mono);
	free(wav_data);
	return status;

error_wav_mono_malloc:
	free(wav_data);
error_wav_malloc:
	sf_close(audio_file);
error_sf_open:
	flanger_terminate(&flgr);
	Pa_Terminate();
  fclose(filenull);
	return status;

}


