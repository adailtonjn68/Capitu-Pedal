#ifndef AUDIO_MANAGER_H_
#define AUDIO_NAMAGER_H_


#include <portaudio.h>
#include <stddef.h>


void process_buffer(size_t audio_index); 
int stream_callback(const void *input_buffer, void *output_buffer,
					unsigned long frames_per_buffer,
					const PaStreamCallbackTimeInfo *timeinfo,
					PaStreamCallbackFlags status_flags, void *user_data); 
int audio_device_manager(void *arg); 






#endif /* AUDIO_MANAGER_H_*/
