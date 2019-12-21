#include "alsa_ctrl.h"
#include "../slog/slog.h"
#define alsa_ctrl_assert(ptr) ({\
	if(NULL == ptr){\
		return -1;\
	}\
})

#define assert_iret(x) ({\
	if (iret < 0) {\
		show_errno(-iret,#x);\
		oret = -1;\
		break;\
	}\
})

#define auto_lock_func_name(name) int name(alsa_ctrl_t* ptr)
#define jonit_auto_lock(name) auto_lock_func_name(name){\
	int iret = 0;\
	pthread_mutex_lock(&ptr->mtx);\
	iret = _##name(ptr);\
	pthread_mutex_unlock(&ptr->mtx);\
	return iret;\
}

static int _alsa_ctrl_set_params(alsa_ctrl_t* ptr,alsa_args_t *args){
    uint32_t buffer_time = 0;
    uint32_t period_time = 0;
	snd_pcm_hw_params_t *params = NULL;
    int iret = 0,oret = 0;
    snd_pcm_hw_params_alloca(&params);
	for(;;){
		iret = snd_pcm_hw_params_any(ptr->handle, params);
		assert_iret(snd_pcm_hw_params_any);
		
		iret = snd_pcm_hw_params_set_access(ptr->handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
		assert_iret(snd_pcm_hw_params_set_access);
		
		iret = snd_pcm_hw_params_set_format(ptr->handle, params, args->fmt);
		assert_iret(snd_pcm_hw_params_set_format);
		
		iret = snd_pcm_hw_params_set_channels(ptr->handle, params, args->channels);
		assert_iret(snd_pcm_hw_params_set_channels);
		
		iret = snd_pcm_hw_params_set_rate_near(ptr->handle, params, &args->sample_rate, NULL);
		assert_iret(snd_pcm_hw_params_set_rate_near);
		
		iret = snd_pcm_hw_params_get_buffer_time_max(params, &buffer_time, NULL);
		assert_iret(snd_pcm_hw_params_get_buffer_time_max);
		
		buffer_time = buffer_time > ALSA_MAX_BUFFER_TIME ? ALSA_MAX_BUFFER_TIME : buffer_time;
		period_time = buffer_time / 4;
		
		iret = snd_pcm_hw_params_set_buffer_time_near(ptr->handle, params, &buffer_time, NULL);
		assert_iret(snd_pcm_hw_params_set_buffer_time_near);
		
		iret = snd_pcm_hw_params_set_period_time_near(ptr->handle, params, &period_time, NULL);
		assert_iret(snd_pcm_hw_params_set_period_time_near);
		
		iret = snd_pcm_hw_params(ptr->handle, params);
		assert_iret(snd_pcm_hw_params);
		
		ptr->can_pause = snd_pcm_hw_params_can_pause(params);	
		assert_iret(snd_pcm_hw_params_can_pause);
		
		iret = snd_pcm_hw_params_get_period_size(params, &ptr->period_frames, NULL);
		assert_iret(snd_pcm_hw_params_get_period_size);
		
		iret = snd_pcm_hw_params_get_buffer_size(params, &ptr->buffer_bytes);
		assert_iret(snd_pcm_hw_params_get_buffer_size);
		
		size_t bits_per_sample = snd_pcm_format_physical_width(args->fmt);		
		ptr->bytes_per_sample = bits_per_sample / 8;
		ptr->bytes_per_frame = ptr->bytes_per_sample * args->channels;
		break;
	}
    return oret;
}
static int _alsa_ctrl_open(alsa_ctrl_t* ptr,alsa_args_t *args){
	int iret = snd_pcm_open(&ptr->handle,args->device,args->action,args->flags);
	if(iret < 0){
		show_errno(-iret,"snd_pcm_open");
		return -1;
	}
	return 0;
}

alsa_ctrl_t*  alsa_ctrl_create(alsa_args_t *args){
	int iret = 0,oret = 0;
	alsa_ctrl_t* ptr=calloc(1,sizeof(alsa_ctrl_t));
	if(!ptr){
		return NULL;
	}
	for(;;){
		iret = _alsa_ctrl_open(ptr,args);
		if(iret < 0){
			oret = -1;
		}		
		iret = _alsa_ctrl_set_params(ptr,args);
		if(iret < 0){
			oret = -1;
		}
		break;
	}
	pthread_mutex_init(&ptr->mtx,NULL);
	if(oret < 0){
		free(ptr);
		return NULL;
	}
	return ptr;
}

static int _alsa_ctrl_prepare(alsa_ctrl_t* ptr){
	alsa_ctrl_assert(ptr);
	snd_pcm_prepare(ptr->handle);
	ptr->abort_flag = 0;
	return 0;
}


static int _alsa_ctrl_pause(alsa_ctrl_t* ptr){
	alsa_ctrl_assert(ptr);
    int err = 0;
    if (ptr->can_pause) {
        if ((err = snd_pcm_pause(ptr->handle, 1)) < 0) {
            return -1;
        }
    } else {
        if ((err = snd_pcm_drop(ptr->handle)) < 0) {
            return -1;
        }
    }
    return 0;
}

static int _alsa_ctrl_resume(alsa_ctrl_t* ptr){
   alsa_ctrl_assert(ptr);
    int err = 0;
    if (snd_pcm_state(ptr->handle) == SND_PCM_STATE_SUSPENDED) {
        while ((err = snd_pcm_resume(ptr->handle)) == -EAGAIN) {
            sleep(1);
        }
    }
    if (ptr->can_pause) {
        if ((err = snd_pcm_pause(ptr->handle, 0)) < 0) {
            return -1;
        }
    } else {
        if ((err = snd_pcm_prepare(ptr->handle)) < 0) {
            return -1;
        }
    }
    return 0;
}

static int _alsa_ctrl_abort(alsa_ctrl_t* ptr){
    alsa_ctrl_assert(ptr);
	ptr->abort_flag = 1;
	snd_pcm_abort(ptr->handle);
	return 0;
}

static int _alsa_ctrl_clear(alsa_ctrl_t* ptr){
	alsa_ctrl_assert(ptr);
    snd_pcm_drop(ptr->handle);
    return 0;
}

static int _alsa_ctrl_close(alsa_ctrl_t* ptr){
	alsa_ctrl_assert(ptr);
    snd_pcm_drop(ptr->handle);
    snd_pcm_close(ptr->handle);
    ptr->handle = NULL;
    return 0;
}

static int _alsa_ctrl_destroy(alsa_ctrl_t* ptr){
	alsa_ctrl_assert(ptr);
	_alsa_ctrl_close(ptr);
	pthread_mutex_destroy(&ptr->mtx);
	free(ptr);
	return 0;
}

static int _alsa_ctrl_write_stream(alsa_ctrl_t* ptr,
								const char * buffer,
								size_t bytes) {
								
    if (ptr->abort_flag || ptr->handle == NULL) {
        return -1;
    }
	char *p = (char *)buffer;
	size_t c = bytes / ptr->bytes_per_frame;
	int silence_bytes =  (ptr->period_frames / ptr->bytes_per_frame) - bytes;
	if(silence_bytes > 0){
		snd_pcm_format_set_silence(SND_PCM_FORMAT_S16_LE,p + bytes,silence_bytes);
		c = ptr->period_frames;
	}
	ssize_t r = 0;
	while ( c > 0) {
		if (ptr->abort_flag) break;
		r = snd_pcm_writei(ptr->handle, p,  c);
		if (r == -EAGAIN || (r >= 0 && (size_t)r < c)) {
			snd_pcm_wait(ptr->handle, 100);
		} else if (r == -EPIPE) {
			snd_pcm_prepare(ptr->handle);
			r = snd_pcm_recover(ptr->handle, r, 1);
			s_war("EPIPE, buffer underrun.");
			if ( (r > 0) && (r <= c) ) {
				r = snd_pcm_writei(ptr->handle, p, r );
			}
		} else if (r == -ESTRPIPE) {
			do{
				usleep(100);
				if (ptr->abort_flag) break;
				r = snd_pcm_resume(ptr->handle);
			}while(r == -EAGAIN);
			if (r < 0) {
				if (ptr->abort_flag) break;
				if ((r = snd_pcm_prepare(ptr->handle)) < 0) {
					s_err("snd_pcm_prepare error: %s", snd_strerror(r));
					break;
				}
				continue;
			}
		} else if (r < 0) {
			s_err("snd_pcm_writei error: %s", snd_strerror(r));
			break;
		}
		if (r > 0) {
			c -= r;
			p += r * ptr->bytes_per_frame;
		}		
    }
	return (int)(p - (char *)buffer);
}

inline int alsa_ctrl_write_stream(alsa_ctrl_t* ptr,
							const void* buffer,
							size_t bytes) {
	int iret = 0;
	pthread_mutex_lock(&ptr->mtx);
	iret = _alsa_ctrl_write_stream(ptr,buffer,bytes);
	pthread_mutex_unlock(&ptr->mtx);
	return iret;
}

static int _alsa_ctrl_read_stream(alsa_ctrl_t* ptr,
								const char * buffer,
								size_t bytes) {
								
    if (ptr->abort_flag || ptr->handle == NULL) {
        return -1;
    }
	char *p = (char *)buffer;
	size_t c = bytes / ptr->bytes_per_frame;
	int r = 0;
	while ( c > 0) {
		if (ptr->abort_flag) break;
		r = snd_pcm_readi(ptr->handle, p,  c);
		if (r == -EAGAIN || (r >= 0 && (size_t)r < c)) {
			snd_pcm_wait(ptr->handle, 100);
		} else if (r == -EPIPE) {
			snd_pcm_prepare(ptr->handle);
			r = snd_pcm_recover(ptr->handle, r, 1);
			s_war("EPIPE, buffer overrun.");
			if ( (r > 0) && (r <= c) ) {
				r = snd_pcm_readi(ptr->handle, p, r );
			}
		} else if (r == -ESTRPIPE) {
			do{
				usleep(100);
				if (ptr->abort_flag) break;
				r = snd_pcm_resume(ptr->handle);
			}while(r == -EAGAIN);
			if (r < 0) {
				if (ptr->abort_flag) break;
				if ((r = snd_pcm_prepare(ptr->handle)) < 0) {
					show_errno(-r,"snd_pcm_prepare");
					break;
				}
				continue;
			}
		} else if (r < 0) {
			show_errno(-r,"snd_pcm_readi");
			break;
		}
		if (r > 0) {
			c -= r;
			p += (r * ptr->bytes_per_frame);
		}		
    }
	return (int)(p - (char *)buffer);
}

inline int alsa_ctrl_read_stream(alsa_ctrl_t* ptr,
							const void* buffer,
							size_t bytes) {
	int iret = 0;
	pthread_mutex_lock(&ptr->mtx);
	iret = _alsa_ctrl_read_stream(ptr,buffer,bytes);
	pthread_mutex_unlock(&ptr->mtx);
	return iret;
}

static int _alsa_ctrl_reset(alsa_ctrl_t* ptr,alsa_args_t *args){
	int iret = 0,oret = 0;
	_alsa_ctrl_close(ptr);
	iret = _alsa_ctrl_open(ptr,args);
	if(iret < 0){
		return -1;
	}		
	iret = _alsa_ctrl_set_params(ptr,args);
	if(iret < 0){
		return -1;
	}
	return 0;
}

inline int alsa_ctrl_reset(alsa_ctrl_t* ptr,alsa_args_t *args){
	int iret = 0;
	pthread_mutex_lock(&ptr->mtx);
	iret = _alsa_ctrl_reset(ptr,args);
	pthread_mutex_unlock(&ptr->mtx);
	return iret;
}

jonit_auto_lock(alsa_ctrl_prepare)
jonit_auto_lock(alsa_ctrl_pause)
jonit_auto_lock(alsa_ctrl_resume)
jonit_auto_lock(alsa_ctrl_abort)
jonit_auto_lock(alsa_ctrl_clear)
jonit_auto_lock(alsa_ctrl_close)
jonit_auto_lock(alsa_ctrl_destroy)