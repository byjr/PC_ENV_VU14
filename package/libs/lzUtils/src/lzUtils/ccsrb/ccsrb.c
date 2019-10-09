#include "ccsrb.h"
#include "../slog/slog.h"
ccsrb_t* ccsrb_create(size_t size){
	ccsrb_t* ptr = (ccsrb_t *)calloc(1,sizeof(ccsrb_t));
	if(!ptr){
		s_err("calloc ccsrb failed!");
		return NULL;
	}
	ptr->mm = (char *)malloc(size);
	if(!ptr->mm){
		s_err("calloc ptr->mm failed!");
		return NULL;
	}
	ptr->empty = ptr->cBytes = size;
	pthread_mutex_init(&ptr->mtx,NULL);
	return ptr;
}
void ccsrb_destroy(ccsrb_t* ptr){
	if(!ptr) return;
	pthread_mutex_destroy(&ptr->mtx);
	if(ptr->rcm)free(ptr->rcm);
	if(ptr->wcm)free(ptr->wcm);
	free(ptr->mm);
	free(ptr);
}
char *ccsrb_write_query(ccsrb_t* ptr,size_t size){
	if(!ptr) return NULL;
	char * wAddr = NULL;
	if(size <= ptr->empty){
		if(ptr->wi + size < ptr->cBytes){
			wAddr = (char *)ptr->mm + ptr->wi;
		}else{
			wAddr = ptr->wcm = (char *)malloc(size);
		}
	}else{
		s_dbg("write_query size=%d,ptr->contex=%d",(int)size,(int)ptr->empty);
	}
	return wAddr;
}
void ccsrb_write_sync(ccsrb_t* ptr,size_t size){
	if(!ptr) return;
	pthread_mutex_lock(&ptr->mtx);
	if(ptr->wcm){
		memcpy(ptr->mm+ptr->wi,ptr->wcm,ptr->cBytes-ptr->wi);
		memcpy(ptr->mm,ptr->wcm+(ptr->cBytes-ptr->wi),size-(ptr->cBytes-ptr->wi));
		free(ptr->wcm);
		ptr->wcm = NULL;
		ptr->wi = size-(ptr->cBytes-ptr->wi);
	}else{
		ptr->wi += size;
	}	
	ptr->empty -= size;
	ptr->contex += size;
	pthread_mutex_unlock(&ptr->mtx);		
}
char *ccsrb_read_query(ccsrb_t* ptr,size_t size){
	if(!ptr) return NULL;
	char *rAddr = NULL;
	if(size <=  ptr->contex){
		if(ptr->ri + size < ptr->cBytes){
			rAddr = (char *)ptr->mm + ptr->ri;
		}else{
			ptr->rcm = (char *)calloc(1,size);
			memcpy(ptr->rcm,ptr->mm+ptr->ri,ptr->cBytes-ptr->ri);
			memcpy(ptr->rcm+(ptr->cBytes-ptr->ri),ptr->mm,size-(ptr->cBytes-ptr->ri));
			rAddr = ptr->rcm;
		}		
	}else{
		s_dbg("read_query size=%d,ptr->contex=%d",(int)size,(int)ptr->contex);
	}
	return rAddr;
}

void ccsrb_read_sync(ccsrb_t* ptr,size_t size){
	if(!ptr) return;
	pthread_mutex_lock(&ptr->mtx);	
	if(ptr->rcm){
		ptr->ri = size - (ptr->cBytes - ptr->ri);
		free(ptr->rcm);
		ptr->rcm = NULL;
	}else{
		ptr->ri += size;
	}	
	ptr->contex -= size;
	ptr->empty += size;
	pthread_mutex_unlock(&ptr->mtx);	
}
void ccsrb_clear(ccsrb_t* ptr){
	if(!ptr) return ;
	pthread_mutex_lock(&ptr->mtx);
	ptr->wi = ptr->ri = 0;
	ptr->contex = 0;
	ptr->empty = ptr->cBytes;	
	pthread_mutex_unlock(&ptr->mtx);
}
//--------------------ccsrb.c end------------------------------
#include <sys/stat.h>
#if 0//测试代码
size_t rsize = 320;
size_t wsize = 256;
size_t tsize = 1280;
typedef struct subTreadArgs_t{
	FILE* ofp;
	ccsrb_t *m_ccsrb;
	volatile char isFileReadCompleted;
}subTreadArgs_t;

void* sub_thread_proc(void *args){
	subTreadArgs_t* _args = (subTreadArgs_t*)args;
	FILE* ofp = _args->ofp;
	ccsrb_t* ccsrb = _args->m_ccsrb;
	int retryCount = 0;
	for(;;){
		char *rAddr = ccsrb_read_query(ccsrb,rsize);
		if(!rAddr){
			if(_args->isFileReadCompleted && retryCount++ > 5){
				break;
			}
			continue;
		}
		int wret = fwrite(rAddr,1,rsize,ofp);
		if(wret < rsize){
			s_err("");
		}
		ccsrb_read_sync(ccsrb,rsize);		
	}
	do{
		char *rAddr = ccsrb_read_query(ccsrb,ccsrb->contex);
		fwrite(rAddr,1,ccsrb->contex,ofp);
		ccsrb_read_sync(ccsrb,ccsrb->contex);		
	}while(0);
	return NULL;
}
int ccsrb_handle(int argc,char* argv[]){
	char *ipath,*opath;
	int opt=0;
	while ((opt = getopt(argc, argv, "l:p:i:o:r:w:t:h")) != -1) {
		switch (opt) {
		case 'l':
			log_init(optarg,NULL);
			break;
		case 'p':
			log_init(NULL,optarg);
			break;			
		case 'i':ipath = optarg;
			break;
		case 'o':opath = optarg;
			break;
		case 'r':rsize = atoi(optarg);
			break;
		case 'w':wsize = atoi(optarg);
			break;
		case 't':tsize = atoi(optarg);
			break;			
		case 'h':
			return 0;
		default: /* '?' */
			printf("\terr option!\n");
			return -1;
	   }
	}
	do{
		ccsrb_t* ccsrb = ccsrb_create(tsize);
		if(!ccsrb){
			s_err("");
			return -1;
		}
		FILE *ifp = fopen(ipath,"rb");
		if(!ifp){
			s_err("");
			return -1;
		}
		FILE *ofp = fopen(opath,"wb");
		if(!ofp){
			s_err("");
			return -1;
		}
		s_inf("sizeof(int):%d",sizeof(int));
		subTreadArgs_t m_args = {0};
		m_args.ofp = ofp;
		m_args.m_ccsrb = ccsrb;
		pthread_t sub_thread;
		pthread_create(&sub_thread,NULL,&sub_thread_proc,&m_args);
		struct stat resStat;
		int res = fstat(fileno(ifp),&resStat);
		if(res < 0){
			s_err("fstat failure!");
			return -1;
		}
		s_inf("--------file_size is %d bytes",(int)resStat.st_size);
		int rCount = 0;
		int process = 0;
		do{
			char *wAddr = ccsrb_write_query(ccsrb,wsize);
			if(!wAddr){
				continue;
			}
			int rret = fread(wAddr,1,wsize,ifp);
			if(rret < wsize){
				s_err("---------------------------------------------------------");
				if(rret <= 0){
					if(feof(ifp)){
						s_inf("read done !!!--");
						m_args.isFileReadCompleted = 1;
						break;
					}
					perror("fread");
				}
			}
			ccsrb_write_sync(ccsrb,rret);
			rCount += rret;
			static int m_process = 0;
			process = rCount * 100 / resStat.st_size;
			if(m_process != process){
				m_process = process;
				s_err("----------------------------------------------m_process:%3d %%",(int)m_process);
			}
		}while(1);
		s_inf("rCount=%d",(int)rCount);
		pthread_join(sub_thread,NULL);
		fclose(ifp);
		fclose(ofp);
		
		ifp = fopen(ipath,"r");
		if(!ifp){
			s_err("");perror("fopen");
			return -1;
		}
		ofp = fopen(opath,"r");
		if(!ofp){
			s_err("");
			return -1;
		}
		char ibuf[1024];
		char obuf[1024];
		int i= 0;
		for(;i < 1024;i++){
			int res = fread(ibuf,1,sizeof(ibuf),ifp);
			if(feof(ifp)){
				s_err("ifp ecf");
				return -1;
			}
			if(res < sizeof(ibuf)){
				s_err("res < sizeof(ibuf)");
				continue;
			}
			res = fread(obuf,1,sizeof(obuf),ofp);
			if(feof(ofp)){
				s_err("ofp ecf");
				return -1;
			}		
			if(res < sizeof(obuf)){
				s_err("res < sizeof(obuf)");
				continue;
			}
			if(memcmp(ibuf,obuf,sizeof(ibuf))){
				s_err("-------------------failed---in %d----------!!!",i);	
				return -1;
			}
		}
		s_inf("transfer succeed!!");
		ccsrb_destroy(ccsrb);
	}while(1);
	return 0;
}
#endif