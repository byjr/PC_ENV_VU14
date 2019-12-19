#ifndef _STREAMHANDLE_H
#define _STREAMHANDLE_H
extern "C" {
#include <libavformat/avformat.h>
}
#include <mutex>
#include <deque>
#include <condition_variable>
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
		if(mPkt->data){
			delete[]mPkt->data;
		}		
		delete mPkt;
	}
	AVPacket* getPkt(){
		return mPkt;
	}
};
class CondFifo{
	std::deque<anyPkt*> q;
	std::mutex mu;
	std::condition_variable cond;
	size_t maxCount;
	volatile bool m_IsWaitWasExited;
public:
	CondFifo(size_t max){
		maxCount = max < q.max_size() ? max:q.max_size();
		s_inf("maxCount:%u",maxCount);
		m_IsWaitWasExited = false;
	}
	size_t getMax(){
		return maxCount;
	}
	void setWaitExitCond(){
		m_IsWaitWasExited = true;
	}
	int getRiFlags(){
		return q.back()->getPkt()->flags;
	}
	long long getBts(){
		std::unique_lock<std::mutex> locker(mu);
		return q.back()->getPkt()->pts < q.back()->getPkt()->dts ?
			q.back()->getPkt()->pts :
			q.back()->getPkt()->dts;
	}
	void adjustTs(long long bts){
		std::unique_lock<std::mutex> locker(mu);
		for(anyPkt* pPkt:q){
			if(pPkt){
				pPkt->getPkt()->pts -= bts;
				pPkt->getPkt()->dts -= bts;
			}
		}
	}
	anyPkt* readOne(){
		std::unique_lock<std::mutex> locker(mu);
        while(q.empty()){
			if(m_IsWaitWasExited){
				return NULL;
			}
			if(cond.wait_for(locker,std::chrono::seconds(1)) == std::cv_status::timeout){
				continue;
			}
		}
        anyPkt* one = q.back();
        q.pop_back();
        return one;	
	}	
	bool writeOne(anyPkt*    one){
		std::unique_lock<std::mutex> locker(mu);
		if(q.size() >= maxCount){
			return false;
		}
		q.push_front(one);
		locker.unlock();
		cond.notify_one();
		return true;
	}
	void writeCyc(anyPkt* one){
		std::unique_lock<std::mutex> locker(mu);
		if(q.size() >= maxCount){
			q.pop_back();
		}
		q.push_front(one);
		locker.unlock();
		cond.notify_one();
	}
	void clear(){
		std::unique_lock<std::mutex> locker(mu);
		q.clear();
	}
	~CondFifo(){
		q.clear();
	}	
};
class BAStream{
	CondFifo *befor;
	CondFifo *after;
	bool is_befor_read_done;
public:	
	BAStream(size_t max){
		befor = new CondFifo(max);
		after = new CondFifo(max);
		is_befor_read_done = false;
	}
	~BAStream(){
		delete befor;
		delete after;
	}
	int getRiFlags(){
		return befor->getRiFlags();
	}	
	void setWaitExitCond(){
		after->setWaitExitCond();
	}	
	void writeBefor(AVPacket *pPkt){
		auto any = new anyPkt(pPkt);		
		befor->writeCyc(any);
	}
	bool writeAfter(AVPacket *pPkt){
		auto any = new anyPkt(pPkt);
		if(after->writeOne(any) < 0){
			return false;
		}
		return true;
	}
	AVPacket* readOne(){
		AVPacket* pPkt = NULL;
		anyPkt* any = NULL;
		is_befor_read_done = false;
		if(!is_befor_read_done){
			any = befor->readOne();
			if(!any){
				is_befor_read_done = true;
				any =  after->readOne();
			}		
		}else{
			any = after->readOne();
		}
		if(!any){
			return NULL;
		}		
		pPkt = new AVPacket();
		memcpy(pPkt,any->getPkt(),sizeof(AVPacket));
		memcpy(pPkt->data,any->getPkt()->data,any->getPkt()->size);
		delete any;
		return pPkt;
	}
	ssize_t readBuf(char **pBuf,size_t size){		
		static AVPacket *pPkt = NULL;
		static size_t ri = 0;
		static size_t ctx = 0;
		if(!pPkt){
			pPkt = readOne();
			if(!pPkt){
				return -1;
			}
			ri = 0;
			ctx = pPkt->size;
		}
		size_t r_get = size < ctx ? size : ctx;
		memcpy(*pBuf,pPkt->data + ri,r_get);
		ri += r_get;
		ctx -= r_get;
		if(0 == ctx){
			delete []pPkt->data;
			delete pPkt;
			pPkt = NULL;
		}
		return r_get;		
	}		
	void adjustTs(){
		long long bts =befor->getBts();
		befor->adjustTs(bts);
		after->adjustTs(bts);
	}
};
#endif