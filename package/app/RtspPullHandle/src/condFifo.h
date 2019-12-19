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
	anyPkt(AVPacket& pPkt){
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
class CondFifo{
	std::deque<anyPkt> q;
	std::mutex mu;
	std::condition_variable cond;
	size_t maxCount;
public:
	CondFifo(size_t max){
		maxCount = max < q.max_size() ? max:q.max_size();
		s_inf("maxCount:%u",maxCount);
	}
	std::string readOne(){
		std::unique_lock<std::mutex> locker(mu);
        while (q.empty()){
			cond.wait(locker);
		}
        std::string one = q.back();
        q.pop_back();
        return one;	
	}
	int writeOne(std::string& one){
		std::unique_lock<std::mutex> locker(mu);
		if(q.size() >= maxCount){
			return -1;
		}
		q.push_front(one);
		locker.unlock();
		cond.notify_one();
		return 0;
	}
	void writeCyc(std::string& one){
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
#endif