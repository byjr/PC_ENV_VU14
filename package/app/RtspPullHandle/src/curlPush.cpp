#include "curlPush.h"
static size_t curlPushCli::read_callback(void *ptr, size_t size, size_t nmemb, curlPushCli *cli){
	return fread(ptr,size,nmemb,cli->mfp);
}
int curlPushCli::pushPerform(){
		CURL *curl = curl_easy_init();
		if(!curl){
			s_err("curl_easy_init failed!");
			return -1;
		}
		FILE *fp = fopen(mSrcPath,"rb");
		if(!){
			show_errno(0,fopen);
		}
		struct stat file_info;
		if(fstat(fileno(fd), &file_info) != 0){
			show_errno(0,fopen);
		}
		curl_easy_setopt(curl, CURLOPT_READDATA, this);	
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
	
		curl_easy_setopt(curl, CURLOPT_URL,mDstPath);
		curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
		
		curl_off_t lenth =  (curl_off_t)file_info.st_size;
		curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE,lenth);
						 
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, (long)CURLAUTH_ANY);
		curl_easy_setopt(curl, CURLOPT_USERPWD, userpwd);
		
		curl_easy_setopt(curl, CURLOPT_HEADDATA, stdout);			
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

		res = curl_easy_perform(curl);
		if(res != CURLE_OK) {
		  s_err("curl_easy_perform() failed: %s\n",curl_easy_strerror(res));
		  curl_easy_cleanup(curl);
		  return -2;
		}
		curl_easy_cleanup(curl);
		return 0;
	}
}	
 
