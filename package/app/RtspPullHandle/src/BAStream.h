#ifndef _BASTREAM_H_
#define _BASTREAM_H_
extern "C" {
#include <libavformat/avformat.h>
}
#include <cppUtils/MTQueue/MTQueue.h>
class frameData{
	char *data;
	size_t size;
	int flags;
public:
	frameData(AVPacket& pkt){
		data = new char[pkt.size];
		memcpy(data,pkt.data,pkt.size);
		size = pkt.size;
		flags = pkt.flags;
	}
	~frameData(){
		delete []data;
	}
	char* getData(){
		return data;
	}
	size_t getSize(){
		return size;
	}
	size_t getFlags(){
		return size;
	}
};
static void frameDataDestroy(void* one){
	auto frame = (frameData*)one;
	if(!frame){
		return ;
	}
	delete frame;
}
class BAStream{
	MTQueue* befor;
	MTQueue* after;
	bool is_befor_read_done;
public:	
	BAStream(MTQueuePar* mPar){
		befor = new MTQueue(mPar);
		after = new MTQueue(mPar);
		is_befor_read_done = false;
	}
	~BAStream(){
		delete befor;
		delete after;
	}
	int getRiFlags(){
		auto frame = (frameData*)befor->get();
		if(!frame){
			return 0;
		}
		return frame->getFlags();
	}	
	void setWaitExitState(){
		after->setWaitExitState();
	}	
	void writeBefor(AVPacket& frame){
		auto one = new frameData(frame);
		befor->cycWrite(one);
	}
	bool writeAfter(AVPacket& frame){
		auto one = new frameData(frame);
		if(after->write(one) < 0){
			return false;
		}
		return true;
	}
	frameData* readOne(){
		frameData* frame = NULL;
		is_befor_read_done = false;
		if(!is_befor_read_done){
			frame = (frameData*)befor->read();
			if(!frame){
				is_befor_read_done = true;
				frame = (frameData*)after->read(20);
			}		
		}else{
			frame = (frameData*)after->read(20);
		}
		if(!frame){
			return NULL;
		}		
		return frame;
	}
	ssize_t readBuf(char **pBuf,size_t size){	
		static frameData *frame = NULL;
		static size_t ri = 0;
		static size_t ctx = 0;
		if(!frame){
			frame = readOne();
			if(!frame){
				return -1;
			}
			ri = 0;
			ctx = frame->getSize();
		}
		size_t r_get = size < ctx ? size : ctx;
		memcpy(*pBuf,frame->getData() + ri,r_get);
		ri += r_get;
		ctx -= r_get;
		if(0 == ctx){
			delete frame;
			frame = NULL;
		}
		return r_get;		
	}
	void setWaitExitCond(){
		after->setWaitExitState();
	}
};
#endif