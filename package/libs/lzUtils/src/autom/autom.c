//--------------------autom.c start-----------------------------
#include "autom.h"
#include "../slog/slog.h"
autom_t *autom_create(char *head,size_t pre_bytes){
	autom_t *ptr = (autom_t *)calloc(1,sizeof(autom_t));
	if(!ptr){
		s_err("out of memery!");
		return NULL;
	}
	ptr->pre_bytes = pre_bytes;
	if(!head){
		ptr->is_malloc = 1;
		head = (char *)calloc(1,pre_bytes);
		if(!head){
			free(ptr);
			s_err("out of memery!");
			return NULL;			
		}
		ptr->head = ptr->tail = head;
	}else{
		ptr->head = head;
		ptr->cur_bytes = pre_bytes;
		ptr->tail = head + pre_bytes;
	}
	return ptr;
}
ssize_t autom_write(autom_t *ptr,char *buf,size_t bytes){
	size_t need_spece_bytes = ptr->cur_bytes + bytes + 1;
	if(need_spece_bytes > ptr->pre_bytes){
		ptr->pre_bytes = need_spece_bytes;
		ptr->head = (char *)realloc(ptr->head,need_spece_bytes);
		if(!ptr->head){
			s_err("out of memery!");
			return -1;
		}			
	}
	memcpy(ptr->head + ptr->cur_bytes,buf,bytes);
	ptr->head[need_spece_bytes - 1] = 0;
	ptr->cur_bytes += bytes;
	ptr->tail = ptr->head + ptr->cur_bytes;
	return bytes;
}
char *autom_read(autom_t *ptr,size_t bytes){
	if(ptr->cur_bytes < bytes){
		return NULL;
	}
	char *ri = ptr->tail - ptr->cur_bytes;
	ptr->cur_bytes -= bytes;
	return ri;
}
int autom_reset(autom_t *ptr){
	ptr->cur_bytes = ptr->tail - ptr->head;
	return 0;
}
int autom_breset(autom_t *ptr,size_t bytes){
	ptr->cur_bytes = ptr->tail - ptr->head;
	if(ptr->pre_bytes > bytes){
		ptr->head = (char *)realloc(ptr->head,bytes);
		if(!ptr->head){
			s_err("out of memery!");
			return -1;
		}		
	}
	return 0;
}
int autom_clear(autom_t *ptr){
	memset(ptr->head,0,ptr->cur_bytes);
	ptr->cur_bytes = 0;
	ptr->tail = ptr->head;
	return 0;
}
int autom_destroy(autom_t *ptr){
	if(!ptr) return -1;
	if(ptr->is_malloc){
		if(ptr->head){
			free(ptr->head);
		}
	}
	free(ptr);
	return 0;
}
//--------------------autom.c end------------------------------