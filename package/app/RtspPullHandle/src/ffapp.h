#ifndef _FFAPP_H
#define _FFAPP_H
#include <iostream>
#include <thread>
#include <string>
#include <lzUtils/base.h>
#include <unistd.h>
#include "streamHandle.h"
extern "C" {
#include <libavformat/avformat.h>
}
enum class dumpSteamStatus{
	START,
	CYCW,
	DUMP,
	GETI,
	DONE,
};
class RtspCliPar{
public:
	char* iPath;
	char* oSufix;
	size_t fps;
	char* bakPath;
};
class PackPar{
public:
	BAStream* vBAs;
	BAStream* aBAs;
	AVFormatContext* ifmt_ctx;
	std::string oPath;
};

class RtspClient{
	RtspCliPar *mParam;
	std::thread mPullTread;
	int vsIdx;
	int asIdx;
	volatile dumpSteamStatus mBAStream_status;
public:
	RtspClient(RtspCliPar* par);
	int mediaPackageUp(PackPar* par);
	int pullSteamLoop();
	std::string getTimeStemp();
	void setBAStreamStatus(){
		if(mBAStream_status != dumpSteamStatus::GETI){
			mBAStream_status = dumpSteamStatus::GETI;
		}
	}
	~RtspClient();
};

#endif
