#include "shrb.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include "../slog/slog.h"
static shrb_t* shrb_create(char *name,size_t size){
	int fail = 0;
	int fd=shm_open(name,O_RDWR|O_CREAT,0777);
	if(fd <= 0){
		show_errno(0,"shm_open");
		return NULL;
	}
	shrb_t *_shrb = NULL;
	size_t bytes = sizeof(shrb_t) + size*2;
	do{
		int ret=ftruncate(fd,bytes);
		if(ret < 0){
			show_errno(0,"ftruncate");
			fail =1;
			break;			
		}
		_shrb = (shrb_t *)mmap(NULL,bytes,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
		s_inf("_shrb=%p",_shrb);
		if(!_shrb){
			show_errno(0,"mmap");
			fail =2;
			break;
		}
		close(fd);
		_shrb->name=name;	
		_shrb->bytes=bytes;
		_shrb->wi = _shrb->ri = 0;
		_shrb->empty = _shrb->capacity = size;
		pthread_mutexattr_init(&_shrb->mtxAttr);
		pthread_mutexattr_setpshared(&_shrb->mtxAttr,PTHREAD_PROCESS_SHARED);
		pthread_mutex_init(&_shrb->mtx,&_shrb->mtxAttr);
		s_trc("_shrb=%u",_shrb);
		s_trc("_shrb->head=%u",_shrb->head);
		s_trc("_shrb->capacity=%u",_shrb->capacity);
		s_trc("_shrb->_shrb->bytes=%u",_shrb->bytes);
//		s_inf("_shrb->name:%s",_shrb->name);
	}while(0);
	switch(fail){
	case 0:return _shrb;	
	case 3:munmap(_shrb,bytes);
	case 2:
	case 1:close(fd);
		shm_unlink(name);
	default:return NULL;
	}
}
shrb_t* shrb_get(char *name,size_t size){
	int fail = 0;
	int fd=shm_open(name,O_RDWR|O_EXCL,0777);
	if(fd <= 0){
		show_errno(0,"shrb_get/shm_open");		
		return shrb_create(name,size);
	}
	shrb_t *_shrb = NULL;
	size_t bytes = sizeof(shrb_t) + size*2;
	do{
		_shrb = (shrb_t *)mmap(NULL,bytes,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
		s_inf("_shrb=%p",_shrb);
		if(!_shrb){
			show_errno(0,"mmap");
			fail =1;
			break;
		}
		close(fd);
		s_inf("shrb_get succeed!");
		s_trc("_shrb=%u",_shrb);
		s_trc("_shrb->head=%u",_shrb->head);
		s_trc("_shrb->_shrb->capacity=%u",_shrb->capacity);
		s_trc("_shrb->_shrb->bytes=%u",_shrb->bytes);
//		s_inf("_shrb->name:%s",_shrb->name);
	}while(0);
	switch(fail){
	case 0:return _shrb;
	case 1:close(fd);
		shm_unlink(name);
	default:return NULL;
	}	
}
void shrb_detach(shrb_t *ptr){
	if(!ptr){
		return;
	}
	munmap(ptr,ptr->bytes);
}
char *shrb_write_query(shrb_t* ptr,size_t size){
	if(!ptr) return NULL;
	if(size > ptr->empty){
		s_dbg("shrb_write_query failed,size:%u > empty:%u",size,ptr->empty);
		return NULL;
	}
	return (char *)ptr->head + ptr->wi;
}
char *shrb_read_query(shrb_t* ptr,size_t size){
	if(!ptr) return NULL;
	if(size > ptr->contex){
		s_dbg("shrb_read_query failed,size:%u > contex:%u",size,ptr->contex);
		return NULL;
	}
	return (char *)ptr->head + ptr->ri;
}
void shrb_write_sync(shrb_t* ptr,size_t size){
	if(!ptr) return;
	int i = ptr->wi;
	char *tail = ptr->head + ptr->capacity;
	if(i + size < ptr->capacity){
		for(;i < size; i++){
			tail[i] = ptr->head[i];
		}
		ptr->wi += size;
	}else{
		for(;i < (ptr->capacity - ptr->wi); i++){
			tail[i] = ptr->head[i];
		}
		for(i = 0;i < size - (ptr->capacity - ptr->wi); i++){
			ptr->head[i] = tail[i];
		}
		ptr->wi = size - (ptr->capacity - ptr->wi);
	}
	int res = pthread_mutex_lock(&ptr->mtx);
	if(res){
		s_err("");
	}
	ptr->empty -= size;
	ptr->contex += size;
	res = pthread_mutex_unlock(&ptr->mtx);
	if(res){
		s_err("");
	}
}
void shrb_read_sync(shrb_t* ptr,size_t size){
	if(!ptr) return;
	if(ptr->ri + size < ptr->capacity){
		ptr->ri += size;
	}else{
		ptr->ri = size - (ptr->capacity - ptr->ri);
	}
	int res = pthread_mutex_lock(&ptr->mtx);
	if(res){
		s_err("");
	}
	ptr->contex -= size;
	ptr->empty += size;
	res = pthread_mutex_unlock(&ptr->mtx);
	if(res){
		s_err("");
	}
}
void shrb_clear(shrb_t* ptr,char side){
	if(!ptr) return ;s_war(__func__);
	pthread_mutex_lock(&ptr->mtx);
	ptr->wi = ptr->ri = 0;
	ptr->contex = 0;
	ptr->empty = ptr->capacity;	
	pthread_mutex_unlock(&ptr->mtx);
}


