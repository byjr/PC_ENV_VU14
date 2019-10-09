#include <stdlib.h>
#include "user_timer.h"

int userTimerDestroy(userTimer_t *this){
	if(this){
		px_timer_del(this->id);
		free(this);
		this=NULL;
	}
	return 0;
}

int userTimerTrigger(userTimer_t *this,void *args){
	s_trc(__func__);
	struct itimerspec* itPtr=(struct itimerspec*)args;
	if(itPtr){
		memcpy(&this->it,itPtr,sizeof(struct itimerspec));
	}
	return px_timer_set(this->id,0,&this->it,NULL);
}

int userTimerStop(userTimer_t *this){
	s_trc(__func__);
	struct itimerspec it;
	memset(&it,0,sizeof(it));
	return px_timer_set(this->id,0,&it,NULL);
}

userTimer_t *userTimerCreate(int signo,userTimerCB_t callback){
	userTimer_t* userTimerPtr=calloc(1,sizeof(userTimer_t));
	if(!userTimerPtr){
		s_err("userTimerCreate failure");
		return NULL;
	}
	// userTimerPtr->create=&userTimerCreate;
	userTimerPtr->trigger=&userTimerTrigger;
	userTimerPtr->stop=&userTimerStop;
	userTimerPtr->destroy=&userTimerDestroy;
	userTimerPtr->evp.sigev_signo=signo?signo:SIGRTMIN;
	userTimerPtr->evp.sigev_notify = SIGEV_SIGNAL;
	signal(userTimerPtr->evp.sigev_signo,callback);
	int ret=px_timer_create(CLOCK_MONOTONIC,&userTimerPtr->evp,&userTimerPtr->id);
	if(ret<0) return NULL;
	struct itimerspec it={{12,0},{11,0}};
	memcpy(&userTimerPtr->it,&it,sizeof(it));
	return userTimerPtr;	
}