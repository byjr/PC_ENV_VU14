#include <unistd.h>
#include "BinSplit.h"
using namespace CPPBox;
BinSplit::BinSplit(BinSplitPar* par){
	mPar = par;
	mMaxNuhSize = getMaxNaluhSzie();
	mFp = fopen(mPar->iPath,"rb");
	if(!mFp){
		s_err("mPar->iPath=%s",mPar->iPath);
		show_errno(0,"fopen");
		return;
	}
	mBuf = new char[mPar->bytesPerRead + mMaxNuhSize];
	if(!mBuf){
		s_err("oom");
		return;
	}
	if(!checkAndPreread()){
		s_err("checkAndPreread failed");
		return;
	}
	RunSplit();
}
BinSplit::~BinSplit(){
	if(mBuf)
		delete []mBuf;
	if(mFp)
		fclose(mFp);
}	
bool BinSplit::checkAndPreread(){
	auto buf = new char[mMaxNuhSize];
	int readedBytes = fread(buf,1,mMaxNuhSize,mFp);
	if(readedBytes < mMaxNuhSize){
		if(!feof(mFp)){
			show_errno(0,"fread");
			return false;
		}
	}
	rewind(mFp);
	for(auto nuh:mPar->nuhVct){
		if(memcmp(buf,nuh->data,nuh->size)==0){
			mMaxNuh = mOldNuh =  mNewNuh = nuh;
			delete []buf;
			return true;
		}
	}
	delete []buf;
	return false;
}
void BinSplit::getPreSize(const char* buf,size_t size,NaluHead* nuh){
	const char* bufEnd = buf+size;
	int i = 0,j =0;
	size_t count = 0;
	for(i=nuh->size-1;i >= 0;i--){
		for(j=0;nuh->data[j] == *(bufEnd-i);i--,j++){
			count ++;
			if(i==1){
				nuh->preSize = count;
				return ;
			}		
		}
	}
	nuh->preSize = 0;
}
size_t BinSplit::getMaxNaluhSzie(){
	size_t max = mPar->nuhVct[0]->size;
	for(auto nuh:mPar->nuhVct){
		if(max < nuh->size){
			max = nuh->size;
		}
	}
	return max;
}
bool BinSplit::updateMaxNuh(const char* buf,size_t size){
	for(auto nuh:mPar->nuhVct){
		getPreSize(buf,size,nuh);
	}
	mMaxNuh = mPar->nuhVct[0];
	for(auto nuh:mPar->nuhVct){
		if(mMaxNuh->preSize < nuh->preSize){
			mMaxNuh = nuh;
		}
	}
	return true;
}
char* BinSplit::findNextNalu(const char* buf,size_t size){
	char* newNaluAddr = NULL;
	for(auto nuh:mPar->nuhVct){
		newNaluAddr = (char *)memmem(buf,size,nuh->data,nuh->size);
		if(newNaluAddr){
			mOldNuh = mNewNuh;
			mNewNuh = nuh;
			return newNaluAddr;
		}
	}
	return NULL;
}
void BinSplit::RunSplit(){
	size_t mFileIdx = 0;
	size_t findStartIdx = mNewNuh->size;
	size_t foundBytes = 0;	
	char* findStartAddr = mBuf+mNewNuh->size;		
	for(;!feof(mFp);){
		int readedBytes = fread(mBuf+mMaxNuh->preSize,1,mPar->bytesPerRead,mFp);
		if(readedBytes < mPar->bytesPerRead){
			if(!feof(mFp)){
				show_errno(0,"fread");
				return ;
			}
		}
		mFileIdx = ftell(mFp);
		bool bufFindUncomplete = true;
		for(;!feof(mFp) && bufFindUncomplete;){
			char* newNaluAddr = findNextNalu(findStartAddr,mFileIdx-findStartIdx);				
			if(!newNaluAddr){
				if(feof(mFp)){
					break;
				}
				updateMaxNuh(findStartAddr,readedBytes);
				size_t newFoundBytes = mFileIdx - findStartIdx - mMaxNuh->preSize;
				if(mMaxNuh->preSize){
					memcpy(mBuf,mMaxNuh->data,mMaxNuh->preSize);
				}
				foundBytes += newFoundBytes;
				findStartIdx += newFoundBytes;
				findStartAddr = mBuf;
				bufFindUncomplete = false;
				continue;
			}
			foundBytes += newNaluAddr - findStartAddr;
			mPar->nuSizeVct.push_back(foundBytes + mOldNuh->size);
			s_inf("naluIdx=0x%08X[%d]",findStartIdx-mOldNuh->size,foundBytes + mOldNuh->size);
			findStartIdx += (newNaluAddr - findStartAddr) + mNewNuh->size;
			findStartAddr = newNaluAddr + mNewNuh->size;
			foundBytes = 0;
			
		}		
	}
	foundBytes += mFileIdx - findStartIdx;
	mPar->nuSizeVct.push_back(foundBytes + mNewNuh->size);		
}
#if 1
#include <lzUtils/base.h>
#include <getopt.h>
static int help_info(int argc ,char *argv[]){
	s_err("%s help:",get_last_name(argv[0]));
	s_err("\t-i [input path]");
	s_err("\t-o [output url]");
	s_err("\t-l [logLvCtrl]");
	s_err("\t-p [logPath]");
	s_err("\t-h show help");
	return 0;
}
int BinSplit_main(int argc,char *argv[]){
	
	std::vector<NaluHead*> mNuhVct;
	const char longNh[]={0,0,0,1};
	NaluHead nh1={longNh,sizeof(longNh)};
	mNuhVct.push_back(&nh1);
	
	const char shortNh[]={0,0,1};
	NaluHead nh2={shortNh,sizeof(shortNh)};
	mNuhVct.push_back(&nh2);
	
	BinSplitPar mPar = {
		.iPath = "/home/lz/work_space/media/slice.264",
		.bytesPerRead = 32*1024,
		.nuhVct = mNuhVct,
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
	BinSplit mBinSplit(&mPar);
	
	FILE* mFp = fopen(mPar.iPath,"rb");
	if(!mFp){
		s_err("mPar->iPath=%s",mPar.iPath);
		show_errno(0,"fopen");
		exit(-1);
	}	
	for(auto size:mPar.nuSizeVct){
		auto buf = new char[size];
		int res = fread(buf,1,size,mFp);
		if(res < size){
			if(!feof(mFp)){
				show_errno(0,"fread");
				exit(-1);
			}
		}
		showHexBuf(buf,5);
		delete []buf;
	}	
	return 0;
}
#endif
