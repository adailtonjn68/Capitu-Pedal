#include "main.h"
#include <math.h>
#include <threads.h>



int main(void) {
	int status;
	int status_audio_thrd, status_window_thrd;
	
	thrd_t audio_manager_thrd;
	thrd_t window_thrd;

	thrd_create(&window_thrd, window_manager, NULL);
	thrd_create(&audio_manager_thrd, audio_device_manager, NULL);

  thrd_join(window_thrd, &status_window_thrd);
	thrd_join(audio_manager_thrd, &status_audio_thrd);

	status = status_audio_thrd | status_window_thrd;

	return status;
}
