#ifndef PX_SEM_H_
#define PX_SEM_H_ 1
#include <errno.h>
#include <time.h>
#include <semaphore.h>
#if 0
int sem_init(sem_t *sem, int pshared, unsigned int value);
sem_t *sem_open(const char *name, int oflag,mode_t mode, unsigned int value);
int sem_close(sem_t *sem);
int sem_getvalue(sem_t *sem, int *sval);
int sem_post(sem_t *sem);
int sem_wait(sem_t *sem);
int sem_trywait(sem_t *sem);
int sem_timedwait(sem_t *sem, const struct timespec *abs_timeout);
#endif

#define px_seminit(p_sem,pshared,value) ({\
	int ret=0;\
	ret=sem_init(p_sem,pshared,value);\
	if(ret)show_errno(0,"sem_init");\
	ret;\
})

#define px_semopen(pp_sem,path,value) ({\
	int ret=0;\
	do{\
		sem_t* p_sem=sem_open(path,O_CREAT,0644,value);\
		if(SEM_FAILED==p_sem){\
			show_errno(0,"sem_open");\
			ret=-1;\
			break;\
		}\
		*pp_sem=p_sem;\
	}while(0);\
	ret;\
})

#define px_semclose(p_sem) ({\
	int ret=sem_close(p_sem);\
	if(ret<0)show_errno(0,"sem_close");\
	ret;\
})

#define  px_semgetvalue(p_val,p_sem) ({\
	int ret=sem_getvalue(p_sem,p_val);\
	if(ret)show_errno(0,"sem_getvalue");\
	ret=ret?ret:*p_val;\
})

#define px_sempost(p_sem,max) ({\
	int ret=0,value=0;\
	do{\
		if(px_semgetvalue(&value,p_sem)<0){\
			ret=-1;\
			break;\
		}\
		if(value < max){\
			ret=sem_post(p_sem);\
			if(ret){\
				show_errno(0,"sem_post");\
				break;\
			}\
		}\
	}while(0);\
	ret;\
})

#define px_semwait(p_sem) ({\
	int ret=sem_wait(p_sem);\
	if(ret)show_errno(0,"sem_wait");\
	ret;\
})

#define px_semtrywait(p_sem) ({\
	int ret=sem_trywait(p_sem);\
	if(ret)show_errno(0,"sem_trywait");\
	ret;\
})

#define px_semtimedwait(p_sem,tsv) ({\
	int ret=0;\
	struct timespec ts={0};\
	do{\
		ret=clock_gettime(CLOCK_REALTIME, &ts);\
		if(ret){\
			show_errno(0,"clock_gettime");\
			break;\
		}\
		ts.tv_sec+=tsv.tv_sec;\
		ts.tv_nsec+=tsv.tv_nsec;\
		ret=sem_timedwait(p_sem,&ts);\
		if(ret)show_errno(0,"sem_timedwait");\
	}while(0);\
	ret;\
})

#define px_semdestroy(p_sem) ({\
	int ret=sem_destroy(p_sem);\
	if(ret)show_errno(0,"sem_destroy");\
	ret;\
})

#define px_semunlink(path) ({\
	int ret=sem_unlink(path);\
	if(ret)show_errno(0,"sem_unlink");\
	ret;\
})

#define MAX_BUF_SIZE 512
#define SLOG_SEM_PATH "/""__FILE__"
#endif