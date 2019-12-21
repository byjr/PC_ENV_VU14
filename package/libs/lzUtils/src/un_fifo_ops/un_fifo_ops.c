#include <stdlib.h>
#include "../common/fd_op.h"
#include "un_fifo_ops.h"
// 1.若以只读或者只写方式打开fifo 会卡在open位置
int un_fifo_ops_init(unixFifoOps_t* ptr){
	if(ptr->needLock)pthread_mutex_lock(&ptr->mtx);
	int fd=un_open(ptr->path,O_RDWR|O_EXCL,0666);
	if(fd<0){
		mkfifo(ptr->path,06666);
		fd=un_open(ptr->path,O_RDWR|O_EXCL,0666);
		if(fd<0){
			s_err("open %s failed!");
			return fd;
		}
	}
	if(ptr->needLock)pthread_mutex_unlock(&ptr->mtx);
	return fd;
}
ssize_t un_fifo_ops_read(unixFifoOps_t* ptr,char *buf,size_t size){
	ssize_t ret=un_read(ptr->fd,buf,size);
	if(ret<0){
		return -1;
	}
	return ret;
}
ssize_t un_fifo_ops_write(unixFifoOps_t* ptr,char *buf,size_t size){
	if(ptr->needLock)pthread_mutex_lock(&ptr->mtx);
	char *widx=buf;
	size_t remSize=size;
	ssize_t ret=0;
	for(;remSize>PIPE_BUF;){
		ret=un_write(ptr->fd,widx,PIPE_BUF);
		if(ret<0){
			goto exit;
		}
		remSize-=PIPE_BUF;
		widx+=PIPE_BUF;
	}
	ret=un_write(ptr->fd,widx,remSize); 
	if(ret<0){
		goto exit;
	}
exit:	
	if(ptr->needLock)pthread_mutex_unlock(&ptr->mtx);
	return ret;
}
int un_fifo_ops_destory(unixFifoOps_t* ptr){
	if(ptr->needLock)pthread_mutex_lock(&ptr->mtx);	
	int ret=un_close(ptr->fd);
	pthread_mutex_t* pMtx=&ptr->mtx;
	char needLock=ptr->needLock;
	free(ptr);
	ptr=NULL;	
exit:
	if(needLock){
		pthread_mutex_unlock(pMtx);
		pthread_mutex_destroy(pMtx);
	}	
	return ret;
}
unixFifoOps_t* un_fifo_ops_create(const char *path,char needLock){
	unixFifoOps_t *ptr=calloc(1,sizeof(unixFifoOps_t));
	if(!ptr){
		s_err("calloc failed");
		return NULL;
	}
	ptr->path = (char *)path;
	ptr->needLock=needLock;
	if(needLock){
		pthread_mutex_init(&(ptr->mtx),NULL);
	}
	ptr->fd=un_fifo_ops_init(ptr);
	if(ptr->fd<0){
		s_err("un_fifo_ops_init failed!");
		return NULL;
	}
	return ptr;
}