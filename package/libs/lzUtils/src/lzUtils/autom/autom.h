#ifdef __cplusplus
extern "C" {
#endif
//--------------------autom.h start-----------------------------
#ifndef AI_MEM_H_
#define AI_MEM_H_ 1
#include <stdio.h>
#include <stdlib.h>

typedef struct autom_t{
	size_t pre_bytes;
	size_t cur_bytes;
	char *head;
	char *tail;
	char is_malloc;
}autom_t;

autom_t *autom_create(char *head,size_t raw_bytes);
ssize_t autom_write(autom_t *ptr,char *buf,size_t bytes);
int autom_destroy(autom_t *ptr);
char *autom_read(autom_t *ptr,size_t bytes);
int autom_clear(autom_t *ptr);
int autom_reset(autom_t *ptr);
int autom_breset(autom_t *ptr,size_t bytes);
#endif
//--------------------autom.h end------------------------------
#ifdef __cplusplus
}
#endif