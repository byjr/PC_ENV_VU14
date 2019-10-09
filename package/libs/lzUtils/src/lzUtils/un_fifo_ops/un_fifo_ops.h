#ifndef UNIX_FIFO_OPS_H_
#define UNIX_FIFO_OPS_H_ 1
#include <stdio.h>
#include <sys/file.h>
#include <unistd.h>
#include <fcntl.h> 
#include <sys/stat.h>
#include <pthread.h>
#include <limits.h>
typedef struct unixFifoOps_t{
	int fd;
	char *path;
	char needLock;
	pthread_mutex_t mtx;
}unixFifoOps_t;

int un_fifo_ops_init(unixFifoOps_t* ptr);
ssize_t un_fifo_ops_read(unixFifoOps_t* ptr,char *buf,size_t size);
ssize_t un_fifo_ops_write(unixFifoOps_t* ptr,char *buf,size_t size);
ssize_t un_fifo_ops_write_fmt(unixFifoOps_t* ptr,char *fmt,...);
int un_fifo_ops_destory(unixFifoOps_t* ptr);
unixFifoOps_t* un_fifo_ops_create(const char *path,char needLock);
#endif
