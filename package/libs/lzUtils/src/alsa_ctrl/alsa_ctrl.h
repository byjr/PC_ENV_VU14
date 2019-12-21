#ifdef __cplusplus
extern "C" {
#endif
//--------------------alsa_ctrl.h start-----------------------------
#ifndef ALSA_CTRL_H_
#define ALSA_CTRL_H_

#include <stdint.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>
#include <pthread.h>

#define ALSA_MAX_BUFFER_TIME 500000

typedef struct alsa_args_t{
	const char *device;
	int sample_rate;
	char channels;	
	char action;
	char flags;
	int fmt;
}alsa_args_t;

typedef struct alsa_ctrl_t{
    snd_pcm_t* handle;
    pthread_mutex_t mtx;
    char can_pause;
	snd_pcm_uframes_t period_frames;
	snd_pcm_uframes_t buffer_bytes;
	size_t bytes_per_sample;
	size_t bytes_per_frame;
    volatile char abort_flag;
	alsa_args_t* mPar;
}alsa_ctrl_t;


#define declar_auto_lock(name) extern int name(alsa_ctrl_t* ptr);

extern alsa_ctrl_t*  alsa_ctrl_create(alsa_args_t *args);
declar_auto_lock(alsa_ctrl_prepare)
declar_auto_lock(alsa_ctrl_pause)
declar_auto_lock(alsa_ctrl_resume)
declar_auto_lock(alsa_ctrl_abort)
declar_auto_lock(alsa_ctrl_clear)
declar_auto_lock(alsa_ctrl_close)
declar_auto_lock(alsa_ctrl_destroy)
extern int alsa_ctrl_write_stream(alsa_ctrl_t* ptr,
							const void* buffer,
							size_t buff_size);
extern int alsa_ctrl_read_stream(alsa_ctrl_t* ptr,
							const void* buffer,
							size_t bytes);
extern int alsa_ctrl_reset(alsa_ctrl_t* ptr,alsa_args_t *args);					
#endif
//--------------------csrb.h end------------------------------
#ifdef __cplusplus
	}
#endif

