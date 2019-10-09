#define __USE_GNU         /* See feature_test_macros(7) */
#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "user_fifo.h"
#include "../slog/slog.h"
#include "../common/misc.h"
#include "../common/fd_op.h"
#include "../un_fifo_ops/un_fifo_ops.h"

#define UARTD_FIFO_FILE 	"/tmp/UartFIFO"
#define FMT_PREFIX 			"^"
#define FMT_SUBFFIX 		"\n"
#define FMT_PREFIX_LEN 		(sizeof(FMT_PREFIX)-1)
#define FMT_SUBFFIX_LEN 	(sizeof(FMT_SUBFFIX)-1)

unixFifoOps_t *user_fifo_write_init(const char *path){
	unixFifoOps_t * ptr=un_fifo_ops_create(path,0);
	if(!ptr){
		s_err("ptr create failed!");
		return NULL;
	}
	int ret=fd_set_flag(ptr->fd,O_NONBLOCK);
	if(ret<0){
		s_err("fd_set_flag failed!");
		return NULL;
	}
	return ptr;
}
int user_fifo_write_str(unixFifoOps_t *ptr,char *contex){
	if(!ptr){
		s_err("ptr is NULL");
		return -1;
	}
	size_t size=strlen(contex);
	int ret=un_fifo_ops_write(ptr,contex,size);
	if(ret<0){
		ret=un_fifo_ops_init(ptr);
		if(ret<0){
			return -2;
		}
		ret = un_fifo_ops_write(ptr,contex,size);
		if(ret < 0) {
			return -3;
		}
	}
	return 0;
}
int user_fifo_write_fmt(unixFifoOps_t *ptr,const char *fmt,...){
	char buf[1024]=FMT_PREFIX;
	char *contex=buf+FMT_PREFIX_LEN;
	size_t availableSize=sizeof(buf)-FMT_PREFIX_LEN-FMT_SUBFFIX_LEN-1;
	va_list args;
	va_start(args,fmt);
	ssize_t ret=vsnprintf(contex,availableSize,fmt,args);
	if(ret <= 0 ){
		s_err("vsnprintf err!");
		return -1;
	}
	if( ret >= availableSize ){
		s_err("vsnprintf buf isn't enough!");
		return -2;
	}
	va_end(args);
	strcat(buf,FMT_SUBFFIX);
	ssize_t wRet =user_fifo_write_str(ptr,buf);
	if(wRet<ret){
		return -3;
	}
	return 0;
}
int user_fifo_write_fmt_len(unixFifoOps_t *ptr,size_t len,const char *fmt,...){
	char buf[1024]=FMT_PREFIX;
	char *contex=buf+FMT_PREFIX_LEN;
	size_t availableSize=sizeof(buf)-FMT_PREFIX_LEN-FMT_SUBFFIX_LEN-1;
	va_list args;
	va_start(args,fmt);
	ssize_t ret=vsnprintf(contex,CCHIP_MIN(availableSize,len),fmt,args);
	if(ret <= 0 ){
		s_err("vsnprintf err!");
		return -1;
	}
	if( ret >= availableSize ){
		s_err("vsnprintf buf isn't enough!");
		return -2;
	}
	va_end(args);
	strcat(buf,FMT_SUBFFIX);
	ssize_t wRet =user_fifo_write_str(ptr,buf);
	if(wRet<ret){
		return -3;
	}
	return 0;
}
int user_fifo_read_init(fifo_cmd_t *pArgs){
	switch(pArgs->type){
	case FIFO_CMD_UNSPECIFIED_FIFO:
	case FIFO_CMD_SPECIFIED_FIFO:{
		pArgs->ops_ptr=un_fifo_ops_create(pArgs->path,0);
		if(!pArgs->ops_ptr){
			return -1;
		}
		return 0;
	}
	case FIFO_CMD_UNSPECIFIED_PIPE:
	case FIFO_CMD_SPECIFIED_PIPE:{
		unixFifoOps_t *ptr=calloc(1,sizeof(unixFifoOps_t));
		if(!ptr){
			s_err("calloc failed");
			return -2;
		}
		ptr->fd=pArgs->fd;
		pArgs->ops_ptr=ptr;
		return 0;
	}
	default:
		return -2;	
	}
}

static int user_fifo_cmd_handle(fifo_cmd_t* pArgs,char *buf,size_t len){
	int ret=0,argc=0,cmpLen=0;
	item_arg_t item_arg={0};
	s_dbg("argl:%s",buf);
	char **argv=argl_to_argv(buf,&argc);
	if(!argv){return -1;}
	size_t i=0;
	for(;pArgs->tbl[i].cmd;i++){
		cmpLen=CCHIP_MIN(strlen(argv[0]),strlen(pArgs->tbl[i].cmd));
		if(strncmp(pArgs->tbl[i].cmd,argv[0],cmpLen)){
			continue;
		}
		item_arg.argc=argc;
		item_arg.argv=argv;
		item_arg.args=pArgs->tbl[i].args;
		ret=pArgs->tbl[i].handle(&item_arg);
		ret=ret<0?-2:0;
		goto exit;
	}
	ret=-3;
exit:
	if(0==ret)		{s_dbg("%s excute succeed!",argv[0]);}
	else if(-2==ret){s_err("%s excute failure!",argv[0]);}
	else if(-3==ret){s_err("%s not found!",argv[0]);}
	FREE(argv);
	return ret;
}
int user_fifo_read_proc(fifo_cmd_t *pArgs){
	char buf[1024]="";
	for(;;){
		bzero(buf, sizeof(buf));
		int get_len =un_fifo_ops_read(pArgs->ops_ptr,buf,sizeof(buf));
		if(get_len > 0){
			char *idx=buf-FMT_SUBFFIX_LEN;
			char *cmd=NULL;
			for(;;){
				cmd=idx=memmem(idx+FMT_SUBFFIX_LEN, buf+get_len-idx-FMT_SUBFFIX_LEN, FMT_PREFIX,FMT_PREFIX_LEN);
				if(!cmd)break;
				idx= memmem(cmd+FMT_PREFIX_LEN, buf+get_len-idx-FMT_PREFIX_LEN, FMT_SUBFFIX, FMT_SUBFFIX_LEN);
				if(!idx)break;
				if(pArgs->handle){
					pArgs->handle(cmd+FMT_PREFIX_LEN,idx-cmd-FMT_PREFIX_LEN);
					continue;
				}
				user_fifo_cmd_handle(pArgs,cmd+FMT_PREFIX_LEN,idx-cmd-FMT_PREFIX_LEN);
			}
		}
	}	
}