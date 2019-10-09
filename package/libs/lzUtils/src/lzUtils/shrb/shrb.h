#ifdef __cplusplus
extern "C" {
#endif
//--------------------shrb.h start-----------------------------
#ifndef SHRB_H_
#define SHRB_H_ 1
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
typedef struct shrb_t{
	char *name;
	size_t bytes;
	volatile size_t ri;
	volatile size_t wi;
	volatile size_t contex;
	volatile size_t empty;
	size_t capacity;//读方和写方最大方单元字节数的最小公倍数
	pthread_mutexattr_t mtxAttr;
	pthread_mutex_t mtx;
	char head[0];
}shrb_t;

shrb_t* shrb_get(char *name,size_t len);
void shrb_detach(shrb_t *ptr);
char *shrb_write_query(shrb_t* ptr,size_t size);
char *shrb_read_query(shrb_t* ptr,size_t size);
void shrb_write_sync(shrb_t* ptr,size_t size);
void shrb_read_sync(shrb_t* ptr,size_t size);
void shrb_clear(shrb_t* ptr,char side);
#endif
//--------------------shrb.h end------------------------------
#ifdef __cplusplus
}
#endif
