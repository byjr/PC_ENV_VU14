#ifndef CHUNKBUF_H_
#define CHUNKBUF_H_ 1
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
typedef struct ccsrb_t{
	size_t ri;//读索引
	volatile size_t contex;//可读字节数
	char *rcm;//读缓存	
	size_t wi;//写索引
	volatile size_t empty;//可写字节数
	char *wcm;//写缓存	
	pthread_mutex_t mtx;//读写互斥锁	
	size_t cBytes;//容量（字节数）	
	char *mm;//主存	
}ccsrb_t;

ccsrb_t* ccsrb_create(size_t size);
char *ccsrb_write_query(ccsrb_t* ptr,size_t size);
char *ccsrb_read_query(ccsrb_t* ptr,size_t size);
void ccsrb_write_sync(ccsrb_t* ptr,size_t size);
void ccsrb_read_sync(ccsrb_t* ptr,size_t size);
void ccsrb_clear(ccsrb_t* ptr);
void ccsrb_destroy(ccsrb_t* ptr);
#endif