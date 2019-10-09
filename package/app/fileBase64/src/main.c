#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
//编码目标字符集
static const char * base64Char = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz+/"; 
//长度关系 (y：输出长度，x:输入长度):y=(x%3)?(x/3+1)*4:x/3*4 考虑传输效率将每次编解码的二进制字节数暂定为：(1024/4-1)*3=765
size_t base64Encode(char *bin,size_t binSize,char *base64,size_t baseSize){
    int i, j;
    char current;
    for ( i = 0, j = 0 ; i < binSize ; i += 3 ){
		if(j+4 >= baseSize){
			err("out buf is't enough,alredy en %u bytes,output %u bytes !",i,j);
			base64[j] = '\0';
			return -1;
		}
        current = (bin[i] >> 2) ;
        current &= (char)0x3F;
        base64[j++] = base64Char[(int)current];

        current = ( (char)(bin[i] << 4 ) ) & ( (char)0x30 ) ;
        if ( i + 1 >= binSize ){
            base64[j++] = base64Char[(int)current];
            base64[j++] = '=';
            base64[j++] = '=';
            break;
        }
        current |= ( (char)(bin[i+1] >> 4) ) & ( (char) 0x0F );
        base64[j++] = base64Char[(int)current];

        current = ( (char)(bin[i+1] << 2) ) & ( (char)0x3C ) ;
        if ( i + 2 >= binSize ){
            base64[j++] = base64Char[(int)current];
            base64[j++] = '=';
            break;
        }
        current |= ( (char)(bin[i+2] >> 6) ) & ( (char) 0x03 );
        base64[j++] = base64Char[(int)current];

        current = ( (char)bin[i+2] ) & ( (char)0x3F ) ;
        base64[j++] = base64Char[(int)current];
    }
    base64[j] = '\0';
    return j;
}
size_t base64Decode(char *base64,size_t baseSize,char * bin,size_t binSize){
    int i, j;
    char k;
    char temp[4];
	if((baseSize/4-1)*3>binSize){
		err("out buf is't enough,alredy encode %u bytes,output %u bytes !",i,j);
		return -1;
	}
    for ( i = 0, j = 0;i < baseSize ; i += 4 ){
        memset( temp, 0xFF, sizeof(temp) );
        for ( k = 0 ; k < 64 ; k ++ ){
            if ( base64Char[k] == base64[i] )
                temp[0]= k;
        }
        for ( k = 0 ; k < 64 ; k ++ ){
            if ( base64Char[k] == base64[i+1] )
                temp[1]= k;
        }
        for ( k = 0 ; k < 64 ; k ++ ){
            if ( base64Char[k] == base64[i+2] )
                temp[2]= k;
        }
        for ( k = 0 ; k < 64 ; k ++ ){
            if ( base64Char[k] == base64[i+3] )
                temp[3]= k;
        }

        bin[j++] = ((char)(((char)(temp[0] << 2))&0xFC)) |
                ((char)((char)(temp[1]>>4)&0x03));
        if ( base64[i+2] == '=' )
            break;

        bin[j++] = ((char)(((char)(temp[1] << 4))&0xF0)) |
                ((char)((char)(temp[2]>>2)&0x0F));
        if ( base64[i+3] == '=' )
            break;

        bin[j++] = ((char)(((char)(temp[2] << 6))&0xF0)) |
                ((char)(temp[3]&0x3F));
    }
    return j;
}
#define s_inf(x...) printf("\e[1;42mINF [%s:%d]:\e[0m",__FILE__,__LINE__);printf(x);printf("\n");
#define s_err(x...) printf("\e[1;41mERR [%s:%d]:\e[0m",__FILE__,__LINE__);printf(x);printf("\n");
#define s_dbg(x...) printf("\e[1;43mDBG [%s:%d]:\e[0m",__FILE__,__LINE__);printf(x);printf("\n");
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
	s_inf("%s build in:[%s %s].",argv[0],__DATE__,__TIME__);
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
				s_err("fread");
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
