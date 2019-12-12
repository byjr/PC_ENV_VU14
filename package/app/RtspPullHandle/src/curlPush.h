#ifndef _CURL_PUSH_H
#define _CURL_PUSH_H
#include <curl/curl.h>
class curlPushCliPar{
public:	
	char *mSrcPath;
	char *mDstPath;
	char *userpwd;
};
class curlPushCli{
	curlPushCliPar *mPar;
	FILE *mfp;
public:	
	curlPushCli(curlPushCliPar* par){
		mPar = par;
		curl_global_init(CURL_GLOBAL_ALL);	
	}
	~curlPushCli(){
		curl_global_cleanup();
	}
	static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *stream);
	int pushPerform();
};
#endif