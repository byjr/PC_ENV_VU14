#include "fp_op.h"

unsigned get_size_by_path(const char *path){
	struct stat lstat={0};
	int ret=stat(path,&lstat);
	if(ret<0)return -1;
	return lstat.st_size;
}

unsigned get_path_size(const char *path){
    unsigned filesize = -1;  
    FILE *fp;  
    fp = fopen(path, "r");  
    if(fp == NULL)  
        return filesize;  
    fseek(fp, 0L, SEEK_END);  
    filesize = ftell(fp);  
    fclose(fp);  
    return filesize;  
}
int fp_getc_file(char **pBuf, size_t size,char *path){
	FILE* pfile=fopen(path,"r");
	if(!pfile){
		show_errno(0,"fopen");
		return -1;
	}
	char *p=calloc(1,size);
	*pBuf=p;
	if(!p){
		s_err("calloc %d failure",size);
		return -2;
	}
	int i=0;
	for(i=0;i<size;i++){
		p[i]=fgetc(pfile);
		if(EOF==p[i]){
			break;
		}
	}
	if(!p[0]){
		free(p);
		s_err("%s no contex!",path);
		return -3; 	
	}
	return 0;
}
int fp_write_file(char *path,char *data,char *mode){
	int fail=1,ret=0,len=0;
	FILE *fp=NULL;
	if(!data){
		s_err("data==NULL");
		goto Err0;
	}
	if(!(fp=fopen(path,mode))){
		s_err("fopen %s fail,err:%d[%s]",path,errno,strerror(errno));
		goto Err0;
	}
	if((len=strlen(data))<1){
		s_war("data content is empty!");
		goto Err1;
	}
	if((ret=fwrite(data,sizeof(char),len,fp))<len){
		s_err("fwrite %s fail,err:%d[%s]",path,errno,strerror(errno));
		goto Err1;		
	}
	fail=0;
Err1:
	fclose(fp);
Err0:
	if(fail)return -1;
	return 0;
}

int fp_copy_file(char* dst_path,char *src_path){
	char *data=NULL;
	int fail=1;
	if(fp_read_file(&data,src_path,"r")<0){		
		goto exit0;
	}
	if(fp_write_file(dst_path,data,"w")<0){
		goto exit1;
	}
	fail=0;
exit1:
	FREE(data);
exit0:
	if(fail)return -1;
	return 0;
}

int fp_read_file(char **pData,char *path,char *mode){
	FILE *fp=NULL;
	int ret=0,fail=1;
	char *data=NULL;
	struct stat f_stat={0};
	if(!(fp=fopen(path,mode))){
		s_war("fopen %s fail,err:%d[%s]",path,errno,strerror(errno));
		goto Err0;
	}
	if((ret=stat(path,&f_stat))<0){
		s_err("stat %s fail,err:%d[%s]",path,errno,strerror(errno));
		goto Err1;
	}
	if(!(data=(char*)calloc(1,f_stat.st_size+1))){
		s_err("calloc fail!");
		goto Err1;
	}
	if((ret=fread(data,sizeof(char),f_stat.st_size,fp))<f_stat.st_size){
		s_err("fread %s fail,err:%d[%s]",path,errno,strerror(errno));
		goto Err2;	
	}
	if(!data){
		s_err("fread data fial!");
		goto Err2;
	}
	*pData=data;
	fail=0;
Err2:
	if(fail)FREE(data);
Err1:
	fclose(fp);
Err0:
	if(fail)return -1;
	return 0;
}