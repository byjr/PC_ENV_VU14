#include "curlPush.h"
int curlPushCli::pushPerform(){
	CURL *curl = curl_easy_init();
	if(!curl) {
		s_err("curl_easy_init failed!");
		return -1;
	}
	FILE *oFp = fopen(mPar->mSrcPath.data(),"rb");
	if(!oFp){
		show_errno(0,fopen);
	}
	struct stat file_info;
	if(fstat(fileno(oFp), &file_info) != 0){
		show_errno(0,fileno);
	}	
	/* enable TCP keep-alive for this transfer */
	curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
	/* keep-alive idle time to 120 seconds */
	curl_easy_setopt(curl, CURLOPT_TCP_KEEPIDLE, 120L);
	/* interval time between keep-alive probes: 60 seconds */
	curl_easy_setopt(curl, CURLOPT_TCP_KEEPINTVL, 60L);
	curl_easy_setopt( curl, CURLOPT_TIMEOUT, 10L);
	curl_easy_setopt( curl, CURLOPT_CONNECTTIMEOUT, 3 );
	
	curl_easy_setopt(curl, CURLOPT_READDATA, oFp);	
	// curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
	s_inf("mDstPath:%s",mPar->mDstPath.data());
	curl_easy_setopt(curl, CURLOPT_URL,mPar->mDstPath.data());	
	curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    /* HTTP PUT please */
    curl_easy_setopt(curl, CURLOPT_PUT, 1L);
	
	curl_off_t lenth =  (curl_off_t)file_info.st_size;
	curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE,(curl_off_t)lenth);
	
	// curl_easy_setopt(conn->easy, CURLOPT_NOPROGRESS, 1L);
	// curl_easy_setopt(conn->easy, CURLOPT_PROGRESSFUNCTION, prog_cb);						 
	curl_easy_setopt(curl, CURLOPT_HTTPAUTH, (long)CURLAUTH_ANY);
	curl_easy_setopt(curl, CURLOPT_USERPWD, mPar->userpwd.data());
	s_inf("userPsw:%s",mPar->userpwd.data());
	
	// curl_easy_setopt(curl, CURLOPT_HEADERDATA, stdout);			
	// curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

	CURLcode res = curl_easy_perform(curl);
	// s_inf("mPushState:%d",mPushState?1:0);
	if(res != CURLE_OK) {
	  s_err("curl_easy_perform() failed: %s\n",curl_easy_strerror(res));
	  curl_easy_cleanup(curl);
	  return -2;
	}
	curl_easy_cleanup(curl);
	return 0;
}