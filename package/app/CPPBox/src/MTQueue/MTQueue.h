#ifndef _MT_QUEUE_H
#define _MT_QUEUE_H
#include <mutex>
#include <deque>
#include <condition_variable>
#include <chrono>
#include <lzUtils/base.h>

class MTQueue{
	std::deque<void*> q;
	std::mutex mu;
	std::condition_variable cond;
	size_t maxCount;
	volatile bool mIsWaitWasExited;
public:
	void setWaitExitState(){
		mIsWaitWasExited = true;
	}
	MTQueue(size_t max){
		maxCount = max < q.max_size() ? max:q.max_size();
		s_inf("maxCount:%u",maxCount);
		mIsWaitWasExited = false;
	}
	void* read(){
		std::unique_lock<std::mutex> locker(mu);
        while (q.empty()){
			cond.wait(locker);
		}           
        void* one = q.back();
        q.pop_back();
        return one;
	}
	void* read(size_t tdMsec){
		std::unique_lock<std::mutex> locker(mu);
        while(q.empty()){
			if(cond.wait_for(locker,std::chrono::seconds(1),
				[&]()->bool{return !q.empty();})){
				break;
			}
			if(!mIsWaitWasExited){
				continue;
			}
			mIsWaitWasExited = false;
			return NULL;			
		}
        void* one = q.back();
        q.pop_back();
        return one;
	}	
	int write(void* one){
		std::unique_lock<std::mutex> locker(mu);
		if(q.size() >= maxCount){
			return -1;
		}
		q.push_front(one);
		locker.unlock();
		cond.notify_one();
		return 0;
	}
	void cycWrite(void* one){
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
	size_t getSize(){
		std::unique_lock<std::mutex> locker(mu);
		return q.size();
	}
	~MTQueue(){
		q.clear();
	}	
};
#endif