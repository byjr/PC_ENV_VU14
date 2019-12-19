#ifndef _COND_FIFO_H
#include <mutex>
#include <deque>
#include <condition_variable>
extern "C" {
#include <libavformat/avformat.h>
}
class anyPkt{
	AVPacket *mPkt;
public:		
	anyPkt(AVPacket* pPkt = NULL){
		mPkt = new AVPacket();
		memset(mPkt,0,sizeof(AVPacket));
		if(pPkt){
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
	}
	anyPkt(const anyPkt& one){
		memcpy(this,&one,sizeof(anyPkt));
		memcpy(this->getPkt()->data,one.getPkt()->data,one.getPkt()->size);		
	}
	anyPkt& operator= (const anyPkt& one){
		memcpy(this,&one,sizeof(anyPkt));
		memcpy(this->getPkt()->data,one.getPkt()->data,one.getPkt()->size);
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
	std::deque<anyPkt> q;
	std::mutex mu;
	std::condition_variable cond;
	size_t maxCount;
	volatile bool m_IsWaitWasExited;
public:
	std::deque<anyPkt> getQuery(){
		return q;
	}
	CondFifo(size_t max){
		maxCount = max < q.max_size() ? max:q.max_size();
		s_inf("maxCount:%u",maxCount);
		m_IsWaitWasExited = false;
	}
	void setWaitExitCond(){
		m_IsWaitWasExited = true;
	}	
	anyPkt* readOne(std::chrono::milliseconds td){
		std::unique_lock<std::mutex> locker(mu);
		if(q.empty() && !isWaitWasExited){
			m_IsWaitWasExited = false;
			if(wait_for.wait_for(locker,td) == 	std::cv_status::timeout){
				return NULL;
			}
		}
		auto pOne = new anyPkt();
		*pOne = q.back();
		q.pop_back();
        return pOne;
	}
	anyPkt readOne(){
		std::unique_lock<std::mutex> locker(mu);
        while (q.empty()){
			cond.wait(locker);
		}           
        anyPkt one = q.back();
        q.pop_back();
        return one;	
	}
	anyPkt readOne(){
		std::unique_lock<std::mutex> locker(mu);
        while (q.empty()){
			cond.wait(locker);
		}           
       	anyPkt one = q.back();
        q.pop_back();
        return one;	
	}	
	int writeOne(anyPkt& one){
		std::unique_lock<std::mutex> locker(mu);
		if(q.size() >= maxCount){
			return -1;
		}
		q.push_front(one);
		locker.unlock();
		cond.notify_one();
		return 0;
	}
	void writeCyc(anyPkt& one){
		std::unique_lock<std::mutex> locker(mu);
		if(q.size() >= maxCount){
			q.pop_back();
		}
		q.push_front(one);
		locker.unlock();
		cond.notify_one();
	}
	void clear(){
		std::unique_lock<std::mutex> locker( mu);
		q.clear();
	}
	~CondFifo(){
		q.clear();
	}	
};
#endif