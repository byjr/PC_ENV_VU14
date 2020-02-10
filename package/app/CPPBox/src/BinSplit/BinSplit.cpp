#include <unistd.h>
#include <lzUtils/base.h>
#include <getopt.h>
#include "BinSplit.h"
using namespace CPPBox;
static int help_info(int argc ,char *argv[]){
	s_err("%s help:",get_last_name(argv[0]));
	s_err("\t-i [input path]");
	s_err("\t-o [output url]");
	s_err("\t-l [logLvCtrl]");
	s_err("\t-p [logPath]");
	s_err("\t-h show help");
	return 0;
}
#if 1
int BinSplit_main(int argc,char *argv[]){
	const char longNh[]={0,0,0,1};
	const char shortNh[]={0,0,1};
	NaluHead nh1={
		.sb = longNh,
		.sbSize = sizeof(longNh),
	};
	NaluHead nh2={
		.sb = shortNh,
		.sbSize = sizeof(shortNh),
	};
	std::vector<NaluHead*> mNhVct ={&nh1,&nh2};
	BinSplitPar mPar = {
		.iPath = "/home/lz/work_space/media/slice.264",
		.bytesPerRead = 16*1024,
		.nhVct = mNhVct,
	};
	int opt = 0;	
	while ((opt = getopt_long_only(argc, argv,"u:b:i:o:l:p:h",NULL,NULL)) != -1) {
		switch (opt) {
		case 'l':
			lzUtils_logInit(optarg,NULL);
			break;
		case 'p':
			lzUtils_logInit(NULL,optarg);
		case 'i':
			mPar.iPath = optarg;	
			break;
		default: /* '?' */
			return help_info(argc ,argv);
	   }
	}
	auto mBinSplit = new BinSplit(&mPar);
	if(!mBinSplit){
		return -1;
	}
	mBinSplit->RunSplit();
	FILE* mFp = fopen(mPar.iPath,"rb");
	if(!mFp){
		s_err("mPar->iPath=%s",mPar.iPath);
		show_errno(0,"fopen");
		exit(-1);
	}
	auto buf = new char[18537*2];
	for(auto size:mPar.sizeVct){
		int res = fread(buf,1,size,mFp);
		if(res < size){
			if(!feof(mFp)){
				show_errno(0,"fread");
				exit(-1);
			}
		}
		showHexBuf(buf,5);
	}
	delete []buf;
	delete mBinSplit;
	return 0;
}
#else
static size_t GetSbSizeBeforeBoundary(const char* buf,size_t size,const char* sb,size_t sbSize){
	const char* end = buf+size;
	int i = 0,j =0;
	size_t count = 0;
	for(i=sbSize-1;i >= 0;i--){
		for(j=0;sb[j] == *(end-i);i--,j++){
			count ++;
			if(i==1){
				return count;
			}		
		}
	}
	return 0;
}
int BinSplit_main(int argc,char *argv[]){
	int opt = 0;
	while ((opt = getopt_long_only(argc, argv,"u:b:i:o:l:p:h",NULL,NULL)) != -1) {
		switch (opt) {
		case 'l':
			lzUtils_logInit(optarg,NULL);
			break;
		case 'p':
			lzUtils_logInit(NULL,optarg);
		default: /* '?' */
			return help_info(argc ,argv);
	   }
	}
	const char buf[]={0,1,2,3,5,1,1,1,5,6,7,8,9};
	const char sb[]={6,7,8,9};
	int res = GetSbSizeBeforeBoundary(buf,sizeof(buf),sb,sizeof(sb));
	s_war("res=%d",res);
	return 0;
}	
#endif
