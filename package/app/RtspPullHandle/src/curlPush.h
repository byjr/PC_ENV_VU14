#ifndef _CURL_PUSH_H
#define _CURL_PUSH_H
#include <curl/curl.h>
class curlPushCliPar{
public:	
	char *mSrcPath;
	char *mDstPath;
};
class curlPushCli{
	curlPushCliPar *mPar;
public:	
	curlPushCli(curlPushCliPar* par){
		mPar = par;
		curl_global_init(CURL_GLOBAL_ALL);	
	}
	~curlPushCli(){
		curl_global_cleanup();
	}
	static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *stream);
	int pushProcess(char* srcPath,char *dstPath){
		CURL *curl = curl_easy_init();
		if(!curl){
			s_err("curl_easy_init failed!");
			return -1;
		}
		/* we want to use our own read function */
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);

		/* enable uploading */
		curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

		/* HTTP PUT please */
		curl_easy_setopt(curl, CURLOPT_PUT, 1L);

		/* specify target URL, and note that this URL should include a file
		   name, not only a directory */
		curl_easy_setopt(curl, CURLOPT_URL, url);
		
		char *sPath = srcPath ? srcPath : mSrcPath;
		if(sPath){
			
		}
		/* now specify which file to upload */
		curl_easy_setopt(curl, CURLOPT_READDATA, hd_src);

		/* provide the size of the upload, we specicially typecast the value
		   to curl_off_t since we must be sure to use the correct data size */
		curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE,(curl_off_t)-1);

		/* Now run off and do what you've been told! */
		CURLcode res = curl_easy_perform(curl);
		/* Check for errors */
		if(res != CURLE_OK){
			s_err("curl_easy_perform() failed: %s\n",curl_easy_strerror(res));
			curl_easy_cleanup(curl);
			return -2;
		}
		curl_easy_cleanup(curl);
		return 0;
	}
};
#endif