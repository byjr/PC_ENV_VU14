#ifdef __cplusplus
extern "C" {
#endif


#ifndef USER_TIMER_H_
#define USER_TIMER_H_ 1
#include "../common/px_timer.h"
typedef void (*userTimerCB_t)(int sig);	
typedef struct userTimer_t{
	timer_t id;
	struct sigevent evp;
	struct itimerspec it;	
	int (*create)(struct userTimer_t* ,int sig,userTimerCB_t callback);
	int (*trigger)(struct userTimer_t* ,void *args);
	int (*stop)(struct userTimer_t* );
	int (*destroy)(struct userTimer_t* );
}userTimer_t;

userTimer_t* userTimerCreate(int signo,userTimerCB_t callback);

#endif

#ifdef __cplusplus
}
#endif
