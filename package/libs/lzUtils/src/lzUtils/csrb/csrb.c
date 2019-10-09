//--------------------csrb.c start-----------------------------
#include "csrb.h"
#include "../slog/slog.h"
csrb_t* csrb_create(char *head,size_t size){
	csrb_t* ptr = (csrb_t *)calloc(1,sizeof(csrb_t));
	if(!ptr){
		s_err("calloc csrb failed!");
		return NULL;
	}
	if(!head){
		ptr->head = (char *)calloc(2,size);
		if(!ptr->head){
			s_err("calloc ptr->head failed!");
			return NULL;
		}
	}else{
		ptr->head = head;
		ptr->alloc = 1;
	}
	ptr->empty = ptr->capacity = size;
	pthread_mutex_init(&ptr->mtx,NULL);
	return ptr;
}
void csrb_destroy(csrb_t* ptr){
	if(!ptr) return;
	pthread_mutex_destroy(&ptr->mtx);
	if(!ptr->alloc){
		free(ptr->head);
	}
	free(ptr);
}
char *csrb_write_query(csrb_t* ptr,size_t size){
	if(!ptr) return NULL;
	if(size > ptr->empty){
		s_dbg("csrb_write_query failed,size:%u > empty:%u",size,ptr->empty);
		return NULL;
	}
	return ptr->head + ptr->wi;
}
char *csrb_read_query(csrb_t* ptr,size_t size){
	if(!ptr) return NULL;
	if(size > ptr->contex){
		s_dbg("csrb_read_query failed,size:%u > contex:%u",size,ptr->contex);
		return NULL;
	}
	return ptr->head + ptr->ri;
}
void csrb_write_sync(csrb_t* ptr,size_t size){
	if(!ptr) return;
	pthread_mutex_lock(&ptr->mtx);	
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
	ptr->empty -= size;
	ptr->contex += size;
	pthread_mutex_unlock(&ptr->mtx);
}
void csrb_read_sync(csrb_t* ptr,size_t size){
	if(!ptr) return;
	pthread_mutex_lock(&ptr->mtx);
	if(ptr->ri + size < ptr->capacity){
		ptr->ri += size;
	}else{
		ptr->ri = size - (ptr->capacity - ptr->ri);
	}
	ptr->contex -= size;
	ptr->empty += size;
	pthread_mutex_unlock(&ptr->mtx);
}
void csrb_clear(csrb_t* ptr,char side){
	if(!ptr) return ;
	pthread_mutex_lock(&ptr->mtx);
	ptr->wi = ptr->ri = 0;
	ptr->contex = 0;
	ptr->empty = ptr->capacity;	
	pthread_mutex_unlock(&ptr->mtx);
}
//--------------------csrb.c end------------------------------
#if 0//测试代码
#define BYTES_PER_READ 256
#define BYTES_PER_WRITE 320
int main(int argc,char* argv[]){
	char *ipath,*opath;
	int opt=0;
	while ((opt = getopt(argc, argv, "i:o:h")) != -1) {
		switch (opt) {
		case 'i':ipath = optarg;
			break;
		case 'o':opath = optarg;
			break;
		case 'h':
			return 0;
		default: /* '?' */
			printf("\terr option!\n");
			return -1;
	   }
	}
	csrb_t* csrb = csrb_create(NULL,512);
	if(!csrb){
		s_err("");
		return -1;
	}
	FILE *ifp = fopen(ipath,"r");
	if(!ifp){
		s_err("");perror("fopen");
		return -1;
	}
	FILE *ofp = fopen(opath,"w");
	if(!ofp){
		s_err("");perror("fopen");
		return -1;
	}
	int bytes = 0;
	do{
		// usleep(1000*10);
		char *wi = csrb_write_query(csrb,BYTES_PER_READ);
		if(!wi){
			s_err("csrb_write_query");
			continue;
		}
		int rret = fread(wi,1,BYTES_PER_READ,ifp);
		if(rret < BYTES_PER_READ){
			s_inf("rret = %d",rret);
			if(rret <= 0){
				if(feof(ifp)){
					s_inf("read done !!!");
					exit(0);
				}
				perror("fread");
			}
		}
		csrb_write_sync(csrb,BYTES_PER_READ);

		char *ri = csrb_read_query(csrb,BYTES_PER_WRITE);
		if(!ri){
			s_err("csrb_read_query");
			continue;
		}
		int wret = fwrite(ri,1,BYTES_PER_WRITE,ofp);
		if(wret < BYTES_PER_WRITE){
			s_err("");perror("fwrite");
		}
		csrb_read_sync(csrb,BYTES_PER_WRITE);
	}while(1);
}
#endif