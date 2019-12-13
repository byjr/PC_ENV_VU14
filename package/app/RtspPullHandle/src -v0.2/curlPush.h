#ifndef _CURL_PUSH_H
#define _CURL_PUSH_H
#include <unistd.h>
#include <curl/curl.h>
#include <lzUtils/base.h>
#include "streamHandle.h"
#include <string>
class curlPushCliPar{
public:
	std::string mDstPath;
	char *userpwd;
	BAStream *vBAs;
};
class curlPushCli{
	curlPushCliPar *mPar;
	bool mPushState;
public:	
	curlPushCli(curlPushCliPar* par){
		mPar = par;
		mPushState = false;
		curl_global_init(CURL_GLOBAL_ALL);	
	}
	~curlPushCli(){
		curl_global_cleanup();
	}
	static size_t read_callback(void *ptr, size_t size, size_t nmemb, curlPushCli *cli);
	int pushPerform();
};
#endif