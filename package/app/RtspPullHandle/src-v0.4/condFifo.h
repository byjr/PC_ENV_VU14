#ifndef _COND_FIFO_H
#include <mutex>
#include <deque>
#include <condition_variable>
class CondFifo{
	std::deque<std::string> q;
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