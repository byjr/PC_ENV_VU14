#ifndef _BIN_SPLIT_H
#define _BIN_SPLIT_H
#include <lzUtils/base.h>
#include <vector>
#include <memory>
namespace CPPBox{
class NaluHead{
public:	
	const char* sb;
	size_t sbSize;
	size_t rem;
};
class BinSplitPar{
public:
	const char* iPath;
	size_t bytesPerRead;
	std::vector<NaluHead*> nhVct;
	std::vector<size_t> sizeVct;
};
class BinSplit{
	BinSplitPar* mPar;
	FILE *mFp;
	char* mBuf;
	NaluHead* mOldNh;
	NaluHead* mNewNh;
	NaluHead* mMaxNh;
	size_t mMaxSbSize;
	size_t uCount;
public:
	BinSplit(BinSplitPar* par){
		mPar = par;
		mMaxSbSize = GetMaxSbSize();
		mFp = fopen(mPar->iPath,"rb");
		if(!mFp){
			s_err("mPar->iPath=%s",mPar->iPath);
			show_errno(0,"fopen");
			exit(-1);
		}
		mBuf = new char[mPar->bytesPerRead + mMaxSbSize];
		if(!mBuf){
			s_err("oom");
			exit(-1);
		}
		if(!checkDataVaild()){
			s_err("checkDataVaild failed");
			exit(-1);		
		}
	}
	bool checkDataVaild(){
		auto buf = new char[mMaxSbSize];
		int res = fread(buf,1,mMaxSbSize,mFp);
		if(res < mMaxSbSize){
			if(!feof(mFp)){
				show_errno(0,"fread");
				exit(-1);
			}
		}
		for(auto nh:mPar->nhVct){
			if(memcmp(buf,nh->sb,nh->sbSize)==0){
				mMaxNh = mOldNh =  mNewNh = nh;
				delete []buf;
				return true;
			}
		}
		delete []buf;
		return false;
	}
	void GetSbSizeBeforeBoundary(const char* buf,size_t size,NaluHead* nh){
		const char* end = buf+size;
		int i = 0,j =0;
		size_t count = 0;
		for(i=nh->sbSize-1;i >= 0;i--){
			for(j=0;nh->sb[j] == *(end-i);i--,j++){
				count ++;
				if(i==1){
					nh->rem = count;
					return ;
				}		
			}
		}
		nh->rem = 0;
	}
	size_t GetMaxSbSize(){
		size_t max = mPar->nhVct[0]->sbSize;
		for(auto nh:mPar->nhVct){
			if(max < nh->sbSize){
				max = nh->sbSize;
			}
		}
		return max;
	}
	bool getNhRemBoundary(const char* buf,size_t size){
		for(auto nh:mPar->nhVct){
			// const char *data = buf+size-4;
			// showHexBuf(data,4);
			// showHexBuf(nh->sb,nh->sbSize);
			GetSbSizeBeforeBoundary(buf,size,nh);
			// s_inf("sbSize=%d,rem=%d",nh->sbSize,nh->rem);
		}
		mMaxNh = mPar->nhVct[0];
		for(auto nh:mPar->nhVct){
			if(mMaxNh->rem < nh->rem){
				mMaxNh = nh;
			}
		}
		return true;
	}
	char* compareSb(const char* buf,size_t size){
		char* end = NULL;
		for(auto nh:mPar->nhVct){
			end = (char *)memmem(buf,size,nh->sb,nh->sbSize);
			if(end){
				mOldNh = mNewNh;
				mNewNh = nh;
				return end;
			}
		}
		return NULL;
	}
	void RunSplit(){
		size_t mIdex = 0;
		size_t startIdx = mNewNh->sbSize;
		size_t findedBytes = 0;	
		char* start = mBuf+mNewNh->sbSize;
		char* end = mBuf;
		int res = 0;
		rewind(mFp);
		uCount =0;
reRead:		
		res = fread(mBuf+mMaxNh->rem,1,mPar->bytesPerRead,mFp);
		if(res < mPar->bytesPerRead){
			if(!feof(mFp)){
				show_errno(0,"fread");
				exit(-1);
			}
		}
		mIdex += res;
		// s_inf("mIdex=%d",mIdex);
reCompare:
		end = compareSb(start,mIdex-startIdx);
		if(!end){
			if(feof(mFp)){
				goto eofHandle;
			}
			getNhRemBoundary(start,res);//s_inf("mMaxNh->rem=%d",mMaxNh->rem);
			if(mMaxNh->rem){
				memcpy(mBuf,mMaxNh->sb,mMaxNh->rem);
			}
			findedBytes += mIdex - startIdx - mMaxNh->rem;
			start = mBuf;
			startIdx += mIdex - startIdx - mMaxNh->rem;
			// s_inf("startIdx=%d",startIdx);
			goto reRead;
		}
		
		findedBytes += end - start;
		mPar->sizeVct.push_back(findedBytes + mOldNh->sbSize);
		startIdx += (end - start) + mNewNh->sbSize;
		s_war("findedBytes:%d,mOldNh->sbSize:%d,size:%d",findedBytes,mOldNh->sbSize,findedBytes + mOldNh->sbSize);
		s_inf("startIdx=[%d]%d(0x%08X),%d",++uCount,startIdx,startIdx-mNewNh->sbSize,mNewNh->sbSize);		
		start = end + mNewNh->sbSize;
		findedBytes = 0;
		goto reCompare;
eofHandle:
		findedBytes += mIdex - startIdx;
		mPar->sizeVct.push_back(findedBytes + mNewNh->sbSize);
	}
	~BinSplit(){
		delete []mBuf;
		fclose(mFp);	
	}
};
}


#endif