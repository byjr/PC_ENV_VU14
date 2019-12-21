#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include "fd_op.h"
size_t fd_get_size(int fd){
	struct stat stat={0};
	int ret=un_fstat(&stat,fd);
	if(ret<0)return -1;
	return stat.st_size;
}
void show_every_byte(char *buf,size_t size){
	int i=0;
	s_inf("buf show begin:------\\");
	for(i=0;i<size;i++){
		s_raw("%02d:0x%02x\n",i,buf[i]);
	}
	s_inf("buf show end:--------/");
}
int path_get_size(size_t *p_size,char *path){
	struct stat lstat={0};
	int ret=stat(path,&lstat);
	if(ret<0)return -1;
	*p_size=lstat.st_size;
	return 0;
}
int fd_read_file(char **p_buf,char *path){
	int ret=0;
	int fd=0,fail=1;
	fd=un_open(path,O_CREAT|O_RDONLY,0666);
	if(-1==fd)return -1;
	ssize_t size=fd_get_size(fd);
	// ret=path_get_size(&size,path);	
	if(ret<0)goto exit1;
	char *buf=(char *)calloc(1,size+1);
	if(!buf)goto exit1;	
	ret=(int)un_read(fd,buf,size);
	if(ret<0){
		FREE(buf);
		goto exit1;
	}
	// show_every_byte(buf,size);
	*p_buf=buf;
	fail=0;
exit1:
	un_close(fd);
	if(fail)return -1;
	return ret;
}
int fd_write_file(char *path,char *buf,ssize_t size,char *op){
	size_t ret_size=0;
	int fd=0,fail=1,ret=0;
	int flags=O_CREAT|O_RDWR;
	if('a'==op[0])flags|=O_APPEND;
	if('w'==op[0])flags|=O_TRUNC;
	fd=un_open(path,flags,0666);
	if(-1==fd)return -1;
	ret_size=un_write(fd,buf,size);
	if(ret_size<size)goto exit1;	
	fail=0;
exit1:
	ret=un_close(fd);
	if(-1==ret)return -1;
	if(fail)return -1;
	return ret_size;	
}
int fd_cpoy_file(char *sPath,char *dPath){
	char *data=NULL;
	int ret_size=fd_read_file(&data,sPath);
	if(ret_size<0){
		return -1;
	}
	ret_size=fd_write_file(dPath,data,ret_size,"w");
	if(ret_size<0){
		free(data);
		return -2;
	}
	return 0;
}
int fd_set_flag(int fd,int flag){
	int flags = fcntl(fd,F_GETFL, 0);
	if(flags<0){
		show_errno(0,"fcntl F_GETFL failed!");
		return -1;
	}
	int ret=fcntl(fd,F_SETFL, flags|flag);
	if(ret<0){
		show_errno(0,"fcntl F_SETFL failed!");
		return -2;		
	}
	return 0;
}
int fd_clear_flag(int fd,int flag){
	int flags = fcntl(fd,F_GETFL, 0);
	if(flags<0){
		show_errno(0,"fcntl F_GETFL failed!");
		return -1;
	}
	int ret=fcntl(fd,F_SETFL, flags &= ~(flag));
	if(ret<0){
		show_errno(0,"fcntl F_SETFL failed!");
		return -2;		
	}
	return 0;
}
char *fd_mmap_for_read(const char *path,size_t* _bytes){
	int err = -1;
	int fd = un_open(path,O_CREAT|O_RDONLY,0666);
	if(!fd){
		return NULL;
	}
	do{
		ssize_t nBytes = fd_get_size(fd);
		if(nBytes < 0){
			s_err("fd_get_size");
			break;
		}
		char *datPtr = (char *)mmap(NULL,nBytes,PROT_READ,MAP_PRIVATE,fd,0);
		if(!datPtr){
			show_errno(0,"mmap");
			break;
		}
		close(fd);
		*_bytes = nBytes;
		return datPtr;
	}while(0);
	close(fd);
	return NULL;
}




