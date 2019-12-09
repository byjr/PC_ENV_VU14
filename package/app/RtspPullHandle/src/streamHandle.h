#ifndef _STREAMHANDLE_H
#define _STREAMHANDLE_H
extern "C" {
#include <libavformat/avformat.h>
}
#include <vector>
class anyPkt{
	AVPacket *mPkt;
public:	
	anyPkt(AVPacket *pPkt){
		mPkt = new AVPacket();
		memset(mPkt,0,sizeof(AVPacket));
		mPkt->data = new unsigned char[pPkt->size];
		memcpy(mPkt->data,pPkt->data,pPkt->size);
		mPkt->size = pPkt->size;
		mPkt->pts = pPkt->pts;
		mPkt->dts =  pPkt->dts;
		mPkt->pos = pPkt->pos;
		mPkt->duration =  pPkt->duration;			
		mPkt->stream_index = pPkt->stream_index;
		mPkt->flags =  pPkt->flags;	
	}
	~anyPkt(){
		delete[]mPkt->data;
		delete mPkt;
	}
	AVPacket* getPkt(){
		return mPkt;
	}
};
enum class anyStatus{
	SUCCEED,
	FAILED,
	DONE,
};
class anyStream{
	std::vector<anyPkt*> mVct;
	size_t mMax;
	size_t ri;
	size_t wi;
public:
	std::vector<anyPkt*> getVect(){
		return mVct;
	}
	anyStream(size_t max){
		mMax = max;
		ri = wi = 0;
		for(int i = 0; i < mMax;i ++){
			mVct.push_back(NULL);
		}	 
	}
	anyPkt* getOne(){
		if(wi == ri){
			return NULL;
		}
		return mVct[ri];
	}
	void getSync(){
		delete mVct[ri];
		mVct[ri] = NULL;
		ri = (ri + 1 < mMax) ? (ri + 1) : 0;
	}	
	bool readOne(AVPacket **ppPkt){
		auto anyPkt = getOne();
		if(!anyPkt){
			return false;
		}
		auto dPkt = new AVPacket();
		memcpy(dPkt,anyPkt->getPkt(),sizeof(AVPacket));		
		dPkt->data = new unsigned char[anyPkt->getPkt()->size];
		memcpy(dPkt->data,anyPkt->getPkt()->data,anyPkt->getPkt()->size);
		*ppPkt = dPkt;
		s_inf("%s:ri=%d,pts=%lld,dts=%lld",__func__,ri,dPkt->pts,dPkt->dts);
//		s_inf("%s:ri=%d,pos=%d",__func__,ri,dPkt->pos);
		getSync();
		return true;
	}
	bool cycWrite(AVPacket *pPkt){
		size_t next = (wi +1 < mMax) ? (wi +1) : 0;
		s_dbg("wi=%d,next=%d",wi,next);
		if(next == ri){
			delete mVct[ri];
			mVct[ri] = NULL;
			ri = (ri + 1 < mMax) ? (ri +1) : 0;
		}
		auto newAnyPkt = new anyPkt(pPkt);
		if(!newAnyPkt){			
			return false;
		}
		s_inf("%s:wi=%d,pts=%lld,dts=%lld",__func__,wi,pPkt->pts,pPkt->dts);
//		s_inf("%s:wi=%d,pos=%d",__func__,wi,pPkt->pos);
		mVct[wi] = newAnyPkt;
		wi = next;
		return true;
	}
	anyStatus writeOne(AVPacket *pPkt){
		size_t next = (wi +1 < mMax) ? (wi +1) : 0;
		s_dbg("wi=%d,next=%d",wi,next);
		if(next == ri){
			s_inf("anyStream writeOne DONE!");
			return anyStatus::DONE;
		}
		auto newAnyPkt = new anyPkt(pPkt);
		if(!newAnyPkt){			
			return anyStatus::FAILED;
		}
		s_inf("wi=%d,pts=%lld,dts=%lld",wi,pPkt->pts,pPkt->dts);
//		s_inf("%s:wi=%d,pos=%d",__func__,wi,pPkt->pos);
		mVct[wi] = newAnyPkt;
		wi = next;	
		return anyStatus::SUCCEED;
	}
	void clear(){
		for(int i = 0;i < mMax; i++){
			if(mVct[i]){
				delete mVct[i];
				mVct[i]=NULL;
			}
		}
		wi = ri = 0;
	}
	~anyStream(){
		clear();
		for(int i = 0; i < mMax;i ++){		
			mVct.pop_back();
		}	
	} 
};
class BAStream{
	anyStream *befor;
	anyStream *after;
	bool is_befor_read_done;
public:	
	BAStream(size_t max){
		befor = new anyStream(max);
		after = new anyStream(max);
		is_befor_read_done = false;
	}
	~BAStream(){
		delete befor;
		delete after;
	}
	bool writeBefor(AVPacket *pPkt){
		return befor->cycWrite(pPkt);
	}
	anyStatus writeAfter(AVPacket *pPkt){
		return after->writeOne(pPkt);
	}
	bool readOne(AVPacket** ppPkt){
		if(!is_befor_read_done){
			if(!befor->readOne(ppPkt)){
				is_befor_read_done = true;
				return after->readOne(ppPkt);
			}
			return true;
		}else{
			return after->readOne(ppPkt);
		}
	}
	void adjustTs(){
		std::vector<anyPkt*> bVct = befor->getVect();
		long long bPts = befor->getOne()->getPkt()->pts;
		long long bDts = befor->getOne()->getPkt()->dts;
		long long bts = bPts < bDts ? bPts : bDts;
		s_war("bPts=%lld,bDts=%lld",bPts,bDts);
		for(auto pPkt:bVct){
			if(pPkt){
				pPkt->getPkt()->pts -= bts;
				pPkt->getPkt()->dts -= bts;
			}
		}
		std::vector<anyPkt*> aVct = after->getVect();		
		for(auto pPkt:aVct){
			if(pPkt){
				pPkt->getPkt()->pts -= bts;
				pPkt->getPkt()->dts -= bts;
			}
		}	
	}
};
#endif