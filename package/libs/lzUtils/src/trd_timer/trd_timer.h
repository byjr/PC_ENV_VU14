#ifdef __cplusplus
extern "C" {
#endif


#ifndef TRD_TIMER_H_
#define TRD_TIMER_H_ 1
#include "../common/px_timer.h"
#include <pthread.h>
typedef void (*timer_handle_t)(union sigval val);	
#if 0
union sigval{    
    int sival_int; //integer valu  
    void *sival_ptr; //pointer value
}
struct sigevent {
    int           sigev_notify;            //Notification type. 
    int           sigev_signo;            //Signal number. 
    union sigval  sigev_value;             //Signal value. 
    void         (*sigev_notify_function)(union sigval); //Notification function. 
    pthread_attr_t *sigev_notify_attributes;  //Notification attributes. 
};
#endif
typedef struct trd_timer_args_t{
	pthread_attr_t* _attr;
	timer_handle_t handle;
	union sigval  arg;
	struct itimerspec its;
}trd_timer_args_t;

typedef struct trd_timer_t{
	timer_t id;
	struct sigevent evp;
	struct itimerspec its;
	pthread_mutex_t mtx;
}trd_timer_t;

trd_timer_t* trd_timer_create(trd_timer_args_t* args);
int trd_timer_trigger(trd_timer_t *ptr,void *args);
int trd_timer_stop(trd_timer_t *ptr);
int trd_timer_destroy(trd_timer_t *ptr);

#endif

#ifdef __cplusplus
}
#endif

