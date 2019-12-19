#ifndef _STREAMHANDLE_H
#define _STREAMHANDLE_H
extern "C" {
#include <libavformat/avformat.h>
}
#include "condFifo.h"
class BAStream{
	CondFifo *befor;
	CondFifo *after;
	bool is_befor_read_done;
	volatile bool is_all_write_done;
public:	
	BAStream(size_t max){
		befor = new CondFifo(max);
		after = new CondFifo(max);
		is_befor_read_done = false;
		is_all_write_done = false;
	}
	~BAStream(){
		delete befor;
		delete after;
	}
	int getRiFlag(){
		if(befor.empty()){
			return 0;
		}
		return befor->back().getPkt()->flags;
	}	
	void writeBefor(AVPacket& pkt){
		befor->writeCyc(pkt);
	}
	bool writeAfter(AVPacket& pkt){
		if(after->writeOne(pkt) < 0){
			is_all_write_done = true;
			return false;
		}
		return true;
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
	ssize_t readBuf(char **pBuf,size_t size){
		static AVPacket *pPkt = NULL;
		static size_t ri = 0;
		static size_t ctx = 0;
		if(!pPkt){
			while(!readOne(&pPkt)){
				if(ri+2 < after->getMax()){
					if(is_all_write_done){
						return -1;
					}
					s_war("waiting write...");
					usleep(1000*10);
					continue;
				}		
				pPkt = NULL;
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