#include "un_fifo.h"

// 1.若以只读或者只写方式打开fifo 会卡在open位置
int un_fifo_init(char *path,char *mode){
	int style=mode[0]=='r'?O_RDONLY:mode[0]=='w'?O_WRONLY:O_RDWR;
	int fifo_fd=open(path,style|O_EXCL);
	if(fifo_fd<0){
		unlink(path);
		int ret=mkfifo(path,0777);
		if(-1==ret){
			show_errno(0,"mkfifo");
			return -1;		
		}
		fifo_fd=un_fifo_init(path,mode);
		if(fifo_fd<0) {
			return -2;
		}
		// s_dbg("open fifo:%s succeed,fd=%d",path,fifo_fd);
		return fifo_fd;
	}
	// s_dbg("open fifo:%s succeed,fd=%d",path,fifo_fd);
	return fifo_fd;
}
int un_fifo_read(int fifo_fd,char *buf,size_t size){
	size_t ret=un_read(fifo_fd,buf,size);
	if(ret<1) return -1;
	return 0;
}
// 2.若指定的写入写入长度大于数据长度会自动在行未加上换行符
int un_fifo_write(int fifo_fd,char *buf,size_t size){
	size_t ret=un_write(fifo_fd,buf,size);
	if(ret<1) return -1;
	return 0;
}
int un_fifo_write_str(int fifo_fd,char *ptr){
	if(!(ptr && ptr[0])) {
		s_err("ptr is invalid!");
		return -1;
	}
	size_t ret=un_write(fifo_fd,ptr,strlen(ptr));
	if(ret<1) return -2;
	return 0;
}
int un_fifo_deinit(int fifo_fd){
	int ret=un_close(fifo_fd);
	if(ret<0){
		return -1;
	}
	return 0;
}