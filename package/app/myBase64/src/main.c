#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <lzUtils/base.h>

#define work_mode_is_encode() ({\
	int res = 1;\
	if(strcmp(work_mode,"encode")){\
		res = 0;\
	}\
	res;\
})
#define work_mode_is_decode() ({\
	int res = 1;\
	if(strcmp(work_mode,"decode")){\
		res = 0;\
	}\
	res;\
})
int userage(int argc,char *argv[]){
	printf("%s help:\n",argv[0]);
	printf("-m [mode]    :mode:decode or encode\n");
	printf("-i [iPath]   :input file path\n");
	printf("-o [oPath]   :output file path\n");
	return 0;
}
int main(int argc,char *argv[]){
	s_war("%s build in:[%s %s].",argv[0],__DATE__,__TIME__);
	int opt = 0;
	char *work_mode = NULL;
	char *iPath = NULL;
	char *oPath = NULL;
	while ((opt = getopt(argc, argv, "i:o:m:h")) != -1) {
		switch (opt) {
		case 'm':work_mode = optarg;
			s_inf("work_mode:%s",work_mode);
			break;
		case 'i':iPath =  optarg;
			break;
		case 'o':oPath =  optarg;
			break;				
		case 'h':userage(argc,argv);
			return 0;
		default:printf("invaild option!\n");
			userage(argc,argv);
			return -__LINE__;
	   }
	}
	if(!work_mode){
		printf("invaild option!\n");
		userage(argc,argv);
		return -__LINE__;
	}
	FILE *ifp = fopen(iPath,"r");
	if(!ifp){
		s_err("fopen");
		return -__LINE__;
	}
	FILE *ofp = fopen(oPath,"w+");
	if(!ofp){
		s_err("fopen");
		return -__LINE__;
	}
	#define MAX_USER_BUF_BYTES 512
	#define USER_BUF_BYTES_ONE 16
	char iBuf[MAX_USER_BUF_BYTES];
	char oBuf[MAX_USER_BUF_BYTES];
	char *line = NULL;
	size_t bytes = MAX_USER_BUF_BYTES;
	do{
		ssize_t res = 0;
		if(work_mode_is_decode()){
			res = getline(&line,&bytes,ifp);
			if(res <= 0 ){
				s_err("getline failed ,res :%d",(int)res);			
				break;
			}
			res = base64Decode(line,res,oBuf,sizeof(oBuf));
			if(res <= 0){
				break;
			}
		}else if(work_mode_is_encode()){
			res = fread(iBuf,1,USER_BUF_BYTES_ONE,ifp);
			if(res <= 0){
				s_err("fread,res=%d",res);
				break;
			}
			res = base64Encode(iBuf,res,oBuf,sizeof(oBuf)-1);
			if(res <= 0){
				break;
			}
			strcat(oBuf,"\n");
			res += 1;
		}else{
			s_err("unsurport mode!");
			break;
		}
		ssize_t ret = fwrite(oBuf,1,res,ofp);
		if(ret < res ){
			s_err("fwrite");
			break;
		}
	}while(!feof(ifp));
	if(line) free(line);
	fclose(ofp);
	fclose(ifp);
	return 0;
}
