#include <stdlib.h>
#include "px_timer.h"

inline long get_tsd(struct timespec *pt2,struct timespec *pt1,char lev){	
	struct timespec tv={0};
	long tsd=get_tsv(tv,*pt2,*pt1);
	if(tsd<0)return -1;
	switch(lev){
	case 's'://秒
		tsd=tv.tv_sec;
		break;
	case 'm'://毫秒
		tsd=tv.tv_sec*1000+tv.tv_nsec/1000000;
		break;
	case 'u'://微秒
		tsd=tv.tv_sec*1000000+tv.tv_nsec/1000;
		break;
	case 'n'://纳秒
		tsd=tv.tv_sec*1000000000+tv.tv_nsec;
		break;
	default:
		return -1;
	}
	return tsd;
}


int trc_time_sync(trc_time_t* ptr){
	int ret=px_gettime(ptr->cid,&ptr->last);
	if(ret<0)return -1;
	if(CLOCK_REALTIME == ptr->cid){
		char *timeStr=getTmieStr(ptr->last);
		if(timeStr){
			s_raw("%s:%s\n",__func__,timeStr);
			free(timeStr);
		}
	}
	return 0;	
}
long long trc_time_get(trc_time_t* ptr,int sync){
	struct timespec tv={0};
	int ret=px_gettime(ptr->cid,&ptr->cur);
	if(ret<0)return -1;
	tv.tv_sec=ptr->cur.tv_sec-ptr->last.tv_sec;
	tv.tv_nsec=ptr->cur.tv_nsec-ptr->last.tv_nsec;
	if(CLOCK_REALTIME == ptr->cid){
		char *timeStr=getTmieStr(ptr->cur);
		if(timeStr){
			s_raw(timeStr);
			free(timeStr);
		}		
	}
	if(sync){
		memcpy(&ptr->last,&ptr->cur,sizeof(struct timespec));
	}
	return tv.tv_sec*1000000000+tv.tv_nsec;	
}
trc_time_t *trc_time_create(int cid,struct timespec *pTv){
	trc_time_t *ptr=calloc(1,sizeof(trc_time_t));
	if(!ptr){
		return NULL;
	}
	ptr->cid=cid;
	if(pTv){
		memcpy(&ptr->tv,pTv,sizeof(struct timespec));	
	}
	return ptr;
}
int trc_time_proc_sync(trc_time_t* ptr){
	int ret=px_gettime(ptr->cid,&ptr->last);
	if(ret<0)return -1;
	ptr->last.tv_sec += ptr->tv.tv_sec;
	ptr->last.tv_nsec += ptr->tv.tv_nsec;
	if(ptr->last.tv_nsec > 999999999){
		ptr->last.tv_sec ++;
		ptr->last.tv_nsec -= 1000000000;
	}
	return 0;
}
int trc_time_proc_msync(trc_time_t* ptr,struct timespec *_tv){
	int ret=px_gettime(ptr->cid,&ptr->last);
	if(ret<0)return -1;
	if(_tv){
		memcpy(&ptr->tv,_tv,sizeof(struct timespec));
	}
	ptr->last.tv_sec += _tv->tv_sec;
	ptr->last.tv_nsec += _tv->tv_nsec;
	if(ptr->last.tv_nsec > 999999999){
		ptr->last.tv_sec ++;
		ptr->last.tv_nsec -= 1000000000;
	}
	return 0;
}
int trc_time_proc_delay(trc_time_t* ptr){
	struct timespec rem={0};
	int ret = 0;
	do{
		int ret =clock_nanosleep(ptr->cid,TIMER_ABSTIME,&ptr->last,&rem);
		if(!ret){
			return 0;
		}
		if(ret == EINTR){
			px_gettime(ptr->cid,&ptr->last);
			ptr->last.tv_sec += rem.tv_sec;
			ptr->last.tv_nsec += rem.tv_nsec;
			if(ptr->last.tv_nsec > 999999999){
				ptr->last.tv_sec ++;
				ptr->last.tv_nsec -= 1000000000;
			}	
		}else{
			show_errno(ret,"clock_nanosleep");
			return -1;
		}
	}while(ret);
	return 0;
}
static struct timespec mts={0};
int rec_tmie_start(clockid_t cid){
	int ret=px_gettime(cid,&mts);
	if(ret<0)return -1;
	if(CLOCK_REALTIME==cid){
		char *timeStr=getTmieStr(mts);
		if(timeStr){
			s_raw("%s:%s\n",__func__,timeStr);
			free(timeStr);
		}		
	}	
	return 0;
}

inline long long int rec_tmie_get(clockid_t cid,char sync){
	struct timespec ts={0},tv={0};
	int ret=px_gettime(cid,&ts);
	if(ret<0)return -1;
	tv.tv_sec=ts.tv_sec-mts.tv_sec;
	tv.tv_nsec=ts.tv_nsec-mts.tv_nsec;
	if(CLOCK_REALTIME==cid){
		char *timeStr=getTmieStr(ts);
		if(timeStr){
			s_raw(timeStr);
			free(timeStr);
		}		
	}
	if(sync){
		memcpy(&mts,&ts,sizeof(mts));
	}
	return tv.tv_sec*1000000000+tv.tv_nsec;
}

inline char *get_fmt_time(char *tm_str,size_t size,char *fmt){
	time_t rawtime;
	struct tm* timeinfo;
	time(&rawtime);
	timeinfo=localtime(&rawtime);
	strftime(tm_str,size,fmt,timeinfo);
	return tm_str;
}