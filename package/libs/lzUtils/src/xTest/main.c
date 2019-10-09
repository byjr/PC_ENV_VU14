#include <base.h>
#include <getopt.h>

// typedef struct handle_item_t{
	// const char *name;
	// int (*handle)(int argc,char *argv[]);
	// void *args;
// }handle_item_t;

// int readJso_handle(int argc,char* argv[]);
// int writeJso_handle(int argc,char* argv[]);
// int tFifoWrite_handle(int argc,char* argv[]);
// int tFifoRead_handle(int argc,char* argv[]);
// int uartdFifo_handle(int argc,char* argv[]);
// int base64_handle(int argc,char *argv[]);
// int showFileData_handle(int argc,char* argv[]);
// int chunkMatch_handle(int argc,char* argv[]);

// static handle_item_t box_handle_array[]={
	// ADD_HANDLE_ITEM(chunkMatch,NULL)
	// ADD_HANDLE_ITEM(showFileData,NULL)
	// ADD_HANDLE_ITEM(base64,	NULL)
	// ADD_HANDLE_ITEM(tFifoWrite,	NULL)
	// ADD_HANDLE_ITEM(tFifoRead,	NULL)
	// ADD_HANDLE_ITEM(uartdFifo,	NULL)
// };

int main(int argc,char *argv[]) {
	showCompileTmie("TTT",s_war);
	int i=0;
	int opt = 0;
	char op_mode = 0;
	int vol = 0;
	while ((opt = getopt(argc, argv, "l:p:t:v:w:rh")) != -1) {
		switch (opt) {
		case 'l':lzUtils_logInit(optarg,NULL);		
			break;
		case 'p':lzUtils_logInit(NULL,optarg);			
			break;	
		case 't':op_mode = optarg[0];
			break;
		case 'v':vol = atoi(optarg);
			break;		
		case 'h':printf("%s help:\n",get_last_name(argv[0]));
			return 0;
		default:printf("invaild option!\n");
			return -1;
	   }
	}
	switch(op_mode){
	case 's':
		// set_mixer_volume("Common",vol);
		break;
	case 'g':
		// s_inf("vol=%d",get_mixer_volume("Common"));
		break;
	default:
		break;
	}
	return -2;
}


