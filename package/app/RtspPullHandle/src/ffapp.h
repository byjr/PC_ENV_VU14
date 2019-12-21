#ifndef _FFAPP_H
#define _FFAPP_H
#include <iostream>
#include <thread>
#include <string>
extern "C" {
#include <libavformat/avformat.h>
#include <lzUtils/base.h>
#include <unistd.h>
}
#include "BAStream.h"
#include "curlPush.h"

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
	char* oPath;
	char* bakPath;
	char* userpwd;
};
class PackPar{
public:
	BAStream* vBAs;
	BAStream* aBAs;
	AVFormatContext* ifmt_ctx;
	std::string oPath;
	std::string oUrl;
	std::string userpwd;
};

class RtspClient{
	RtspCliPar *mParam;
	std::thread mPullTread;
	int vsIdx;
	int asIdx;
	size_t mFps;
	volatile dumpSteamStatus mBAStream_status;
public:
	RtspClient(RtspCliPar* par);
	int pullSteamLoop();
	std::string getTimeStemp();
	int curlPushUp(curlPushCliPar* curlPar);
	void setBAStreamStatus(){
		if(mBAStream_status != dumpSteamStatus::GETI){
			mBAStream_status = dumpSteamStatus::GETI;
		}
	}
	~RtspClient();
};

#endif
