#include "curlPush.h"
// #include <sys/stat.h>
size_t curlPushCli::read_callback(void *ptr, size_t size, size_t nmemb, curlPushCli *cli){
	ssize_t res =  cli->mPar->vBAs->readBuf((char **)&ptr,size*nmemb);
	if(res < 0){
		s_err("readBuf:%d",res);
		cli->mPushState = true;
		return -1;
	}s_inf("res:%u,size=%u,nmemb=%u",res,size,nmemb);
	return res;
}
int curlPushCli::pushPerform(){
	CURL *curl = curl_easy_init();
	if(!curl) {
		s_err("curl_easy_init failed!");
		return -1;
	}
	// struct stat file_info;
	// if(fstat(fileno(fd), &file_info) != 0){
		// show_errno(0,fopen);
	// }
	
	/* enable TCP keep-alive for this transfer */
	curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
	/* keep-alive idle time to 120 seconds */
	curl_easy_setopt(curl, CURLOPT_TCP_KEEPIDLE, 120L);
	/* interval time between keep-alive probes: 60 seconds */
	curl_easy_setopt(curl, CURLOPT_TCP_KEEPINTVL, 60L);
	curl_easy_setopt( curl, CURLOPT_TIMEOUT, 10L);
	curl_easy_setopt( curl, CURLOPT_CONNECTTIMEOUT, 3 );
	
	curl_easy_setopt(curl, CURLOPT_READDATA, this);	
	curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
	s_inf("mDstPath:%s",mPar->mDstPath.data());
	curl_easy_setopt(curl, CURLOPT_URL,mPar->mDstPath.data());	
	curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    /* HTTP PUT please */
    curl_easy_setopt(curl, CURLOPT_PUT, 1L);
	
	// curl_off_t lenth =  (curl_off_t)file_info.st_size;
	curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE,(curl_off_t)8*1024*1024);
	
	// curl_easy_setopt(conn->easy, CURLOPT_NOPROGRESS, 1L);
	// curl_easy_setopt(conn->easy, CURLOPT_PROGRESSFUNCTION, prog_cb);						 
	curl_easy_setopt(curl, CURLOPT_HTTPAUTH, (long)CURLAUTH_ANY);
	curl_easy_setopt(curl, CURLOPT_USERPWD, mPar->userpwd);
	
	// curl_easy_setopt(curl, CURLOPT_HEADERDATA, stdout);			
	// curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

	CURLcode res = curl_easy_perform(curl);
	s_inf("mPushState:%d",mPushState?1:0);
	if(res != CURLE_OK) {
	  s_err("curl_easy_perform() failed: %s\n",curl_easy_strerror(res));
	  curl_easy_cleanup(curl);
	  return -2;
	}
	curl_easy_cleanup(curl);
	return 0;
}