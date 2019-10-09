#ifndef UN_SIGNAL_H_
#define UN_SIGNAL_H_ 1
#include <stdio.h>
#include <sys/file.h>
#include <unistd.h>
#include <fcntl.h> 
#include <sys/stat.h>
#include "fd_op.h"
#include "misc.h"
#include "../slog/slog.h"
#define FIFO_BUF_SIZE 	1024

#define  un_mkfifo(path,mode) ({\
	int ret=mkfifo(path,mode);\
	if(-1==ret)show_errno(0,"un_mkfifo");\
	ret?-1:0;\
})
extern int un_fifo_init(char *path,char *mode);
extern int un_fifo_read(int fifo_fd,char *buf,size_t size);
extern int un_fifo_write_str(int fifo_fd,char *ptr);
extern int un_fifo_write(int fifo_fd,char *buf,size_t size);
extern int un_fifo_deinit(int fifo_fd);
#endif
