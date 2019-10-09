#include "slog.h"
#include "../common/misc.h"
log_ctrl_t logCtrlArray[MAX_TYPE]={
	[_ERR]	={"ERR"		,Hred		},
	[_WAR]	={"WAR"		,Hyellow	},
	[_INF]	={"INF"		,Hgreen		},
	[_DBG]	={"DBG"		,Hblue		},
	[_TRC]	={"TRC"		,Hpurple	},
};
static FILE *fp=NULL;
static pthread_mutex_t line_lock = PTHREAD_MUTEX_INITIALIZER;
char log_ctrl_set[MAX_TYPE+1]="111";
void lzUtils_getTimeMs(char *ts,int size){
	struct timeval tv={0};
	if(gettimeofday(&tv,NULL)<0){
		return;
	}
	snprintf(ts,size,"%013lu",tv.tv_sec*1000+tv.tv_usec/1000);
}
int lzUtils_logInit(char *ctrl,char *path){
	if(ctrl){
		strncpy(log_ctrl_set,ctrl,CCHIP_MIN(sizeof(log_ctrl_set),strlen(ctrl)));
	}
	if(path){
		pthread_mutex_lock(&line_lock);
		if(!(fp=fopen(path,"a"))){
			printf("fopen %s fail,errno=%d\n",path,errno);
			fp=stdout;
		}
		pthread_mutex_unlock(&line_lock);
	}
}

void lzUtils_slog(log_type_t n,char lock,char *log_ctrl_set,const char *ts,\
					const char *file,const int line,const char *fmt,...){						
	if('1'==log_ctrl_set[n]){
		if(lock)pthread_mutex_lock(&line_lock);	
		if(!fp){
			fp=stdout;
		}
		va_list args;
		fprintf(fp,"[%s%s%s\033[0m %s:%d]:",ts,logCtrlArray[n].color,logCtrlArray[n].name,get_last_name(file),line);			
		va_start(args,fmt);
		vfprintf(fp,fmt,args);
		va_end(args);
		fprintf(fp,"\n");
		if(lock)pthread_mutex_unlock(&line_lock);
	}
}
void lzUtils_rlog(log_type_t n,char lock,char *log_ctrl_set,const char *fmt,...){
	if('1'==log_ctrl_set[n]){
		if(lock)pthread_mutex_lock(&line_lock);
		if(!fp){
			fp=stdout;
		}
		va_list args;
		va_start(args,fmt);
		vfprintf(fp,fmt,args);
		va_end(args);
		if(lock)pthread_mutex_unlock(&line_lock);		
	}
}