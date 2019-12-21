#ifndef _FP_OP_H
#define _FP_OP_H 1

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include "../slog/slog.h"
#include "misc.h"

int fp_write_file(char *path,char *data,char *mode);
int fp_copy_file(char* dst_path,char *src_path);
int fp_read_file(char **pData,char *path,char *mode);
unsigned get_path_size(char *path);
// size_t f_get_string(char *path,char *buf,size_t size);

#define fd_open(path,flags,mode) ({\
	int fd=0;\
	fd=open(path,flags,mode);\
	if(-1==fd){\
		show_errno(0,"open "#path);\
	}\
	fd;\
})
#define fd_read(fd,buf,count) ({\
	ssize_t ret_size=read(fd,buf,count);\
	if(-1==ret_size){\
		show_errno(0,"write");\
	}\
	ret_size;\
})
#define fd_write(fd,buf,count) ({\
	ssize_t ret_size=write(fd,buf,count);\
	if(-1==ret_size){\
		show_errno(0,"write");\
	}\
	ret_size;\
})
#define fd_close(fd) ({\
	int ret=0;\
	ret=close(fd);\
	if(-1==ret){\
		show_errno(0,"close");\
	}\
	ret=-1==ret?-1:0;\
})
#define rec_lock(fd,p_lock) ({\
	int ret=fcntl(fd,F_SETLKW,p_lock);\
	if(ret<0)show_errno(0,"rec_lock");\
	ret;\
})
#endif
