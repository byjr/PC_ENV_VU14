#ifndef _FD_OP_H_1
#define _FD_OP_H_1 1
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "misc.h"
#include "../slog/slog.h"
/* // -----------------------------------------------------------------------------------------------------------
描述：
	open是UNIX系统（包括LINUX、Mac等）的系统调用函数，区别于C语言库函数fopen。
参数：
	flags：
		O_RDONLY	:只读模式
		O_WRONLY	:只写模式
		O_RDWR		:读写模式
		打开/创建文件时，至少得使用上述三个常量中的一个。以下常量是选用的：
		--------------------------------------------------------------------------
		O_APPEND	每次写操作都写入文件的末尾
		O_CREAT		如果指定文件不存在，则创建这个文件
		O_EXCL		如果要创建的文件已存在，则返回-1，并且修改errno的值
		O_TRUNC		如果文件存在，并且以只写/读写方式打开，则清空文件全部内容(即将其长度截短为0)
		O_NOCTTY	如果路径名指向终端设备，不要把这个设备用作控制终端。
		O_NONBLOCK	如果路径名指向FIFO/块文件/字符文件，则把文件的打开和后继I/O设置为非阻塞模式
		O_DSYNC		等待物理I/O结束后再write。在不影响读取新写入的数据的前提下，不等待文件属性更新。
		O_RSYNC		等待所有写入同一区域的写操作完成后再进行
		O_SYNC		等待物理I/O结束后再write，包括更新文件属性的I/O
		--------------------------------------------------------------------------
		如果使用了O_CREAT参数则需要指定模式（mode 例如 0666）
	mode：---
返回值：
       open(), openat(), and creat() return the new file descriptor, or -1 if an error occurred (in which case, errno is set appropri ately).
// -----------------------------------------------------------------------------------------------------------*/
/*//  ____________________________________________________________________________
int open(const char *pathname, int flags);
int open(const char *pathname, int flags, mode_t mode);
int creat(const char *pathname, mode_t mode);
int openat(int dirfd, const char *pathname, int flags);
int openat(int dirfd, const char *pathname, int flags, mode_t mode);
ssize_t read(int fd, void *buf, size_t count);
ssize_t pread(int fd, void *buf, size_t count, off_t offset);
int readdir(unsigned int fd, struct old_linux_dirent *dirp,unsigned int count);
ssize_t write(int fd, const void *buf, size_t count);
ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset);
off_t lseek(int fd, off_t offset, int whence);
int close(int fd);
int unlink(const char *path);//删除连接数
// ____________________________________________________________________________*/
#define un_open(path,flags,mode) ({\
	int fd=0;\
	fd=open(path,flags,mode);\
	if(-1==fd){\
		show_errno(0,"open!");\
	}\
	fd;\
})
#define un_read(fd,buf,size) ({\
	ssize_t ret=0,count=0;\
	for(;;){\
		ret=read(fd,buf+count,size-count);\
		if(ret>0)count+=ret;\
		if(count==size){\
			break;\
		}\
		if(errno==EINTR){\
			show_errno(0,"read");\
			continue;\
		}\
		if(ret<0 && errno!=EAGAIN){\
			show_errno(0,"read");\
		}\
		break;\
	}\
	count;\
})

#define un_write(fd,buf,size) ({\
	ssize_t ret=0,count=0;\
	for(;;){\
		ret=write(fd,buf+count,size-count);\
		if(ret>0)count+=ret;\
		if(count==size){\
			break;\
		}\
		if(errno==EINTR){\
			show_errno(0,"write");\
			continue;\
		}\
		if(ret<0 && errno!=EAGAIN){\
			show_errno(0,"write");\
		}\
		break;\
	}\
	count;\
})

#define un_lseek(fd,offset,whence) ({\
	ssize_t ret=0;\
	ret=lseek(fd,offset,whence);\
	if(-1==ret){\
		show_errno(0,"lseek");\
	}\
	ret=-1==ret?-1:0;\
})
#define un_close(fd) ({\
	int ret=0;\
	ret=close(fd);\
	if(-1==ret){\
		show_errno(0,"close");\
	}\
	ret=-1==ret?-1:0;\
})
//经测试ulink 可以直接删除/tmp/tt1 并不管其他的进程是否打卡此文件
#define un_unlink(path) ({\
	int ret=0;\
	ret=unlink(path);\
	if(-1==ret){\
		show_errno(0,"unlink");\
	}\
	ret=-1==ret?-1:0;\
})
#define un_fstat(p_stat,fd) ({\
	int ret=0;\
	ret=fstat(fd,p_stat);\
	if(-1==ret){\
		show_errno(0,"fstat");\
	}\
	ret;\
})
//fdopen fd->fp
//fileno fp->fd
size_t fd_get_size(int fd);
int fd_read_file(char **p_buf,char *path);
char *fd_mmap_for_read(const char *path,size_t* _bytes);
int fd_write_file(char *path,char *buf,ssize_t size,char *op);
int fd_cpoy_file(char *sPath,char *dPath);
int fd_set_flag(int fd,int flag);
int fd_clear_flag(int fd,int flag);
#endif