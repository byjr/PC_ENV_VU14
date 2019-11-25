#include <cstring>
#include <cstdio>
#include <iostream>
#include <errno.h>
#include <sstream>
#include <istream>
#include <fstream>
#include <csignal>
#include <future>
#include <chrono>
#include <thread>

#ifdef DEBUG
#include <arpa/inet.h>
#endif

#include <lzUtils/base.h>
#include <lzUtils/common/fd_op.h>
#include "WebServer.h"


#define DEFAULT_SERVER_PORT    HTTP_MUSIC_STREAM_PORT

#define SOCK_MAX_CONN   64
#define MAX_HEADER_SIZE 4096
#define SEND_LEN        4096
static char *resouce_path="./out.mp4";
using namespace std;

TCPSocket::~TCPSocket() {s_trc(__func__);
    if(m_sockfd >= 0) {
        close(m_sockfd);
    }
}

void TCPSocket::closeSocket() {s_trc(__func__);
    if(m_sockfd >= 0) {
        close(m_sockfd);
    }
}

/**
 * send data via socket
 *
 * @author etsai (2018/6/1)
 *
 * @param buf - buffer to transmit
 * @param len - length of transmitted buffer
 *
 * @return int - On success, these calls return the number of
 *         characters sent. On error, return -1 or
 *         ERROR_NOT_WRITEABLE (failed to select)
 */
int TCPSocket::Send(const char * buf, int len) {
    if(m_exitWork) {
        return 0;
    }

    FD_ZERO(&m_writefds);
    FD_SET(m_sockfd, &m_writefds);
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 500000;

    if (select(m_sockfd + 1, NULL, &m_writefds, NULL, &timeout) <= 0) {
        return WEBSERVER_ERROR::ERROR_NOT_WRITEABLE;
    }
    return send(m_sockfd, (void*)buf, len, 0);
}

int TCPSocket::Recv(char * buf, int len) {
    if(m_exitWork) {
        return 0;
    }

    FD_ZERO(&m_readfds);
    FD_SET(m_sockfd, &m_readfds);
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 20000;
    if (select(m_sockfd + 1, &m_readfds, NULL, NULL, &timeout) == -1) {
        return WEBSERVER_ERROR::ERROR_NOT_READABLE;
    }
    return recv(m_sockfd, (void *)buf, len, 0);
}

bool TCPSocket::isReadable() {
    struct timeval timeout;
    FD_ZERO(&m_readfds);
    FD_SET(m_sockfd, &m_readfds);
    timeout.tv_sec = 0;
    timeout.tv_usec = 20000;
    if (select(m_sockfd + 1, &m_readfds, NULL, NULL, &timeout) == -1) {
        return false;
    }
    return true;
}

#define FILE_READ_LEN   (MAX_HEADER_SIZE*4)
void HttpClient::SendStreaming(const char * filename) {
	FILE *fp = fopen(filename,"r");
	if(!fp){
		s_err("fopen");
		return ;
	}
	char buf[FILE_READ_LEN];
	do{
		ssize_t rret = fread(buf,1,FILE_READ_LEN,fp);
		if(rret <= 0){
			s_err("rret=%d",rret)
			continue;
		}
		char * wi = buf;
		size_t rem_size = rret ;
		do{
			ssize_t wret = Send(wi, rem_size);
			if(wi <= 0){
				s_err("wret=%d",wret)
				continue;
			}
			rem_size -= wret;
			wi += wret;
		}while(rem_size > 0);
	}while(!feof(fp));
}

void HttpClient::ResponseHead(httpHeadParam &param) {
    stringstream responseString;
    time_t ts = time(NULL);
    static char strDate[128];
    struct tm *t = gmtime(&ts);
    strftime(strDate, sizeof(strDate), "%a, %d %b %Y %H:%M:%S GMT", t);

    responseString << "HTTP/1.1 200 OK\r\n";
    responseString << "Accept-Ranges: bytes\r\n";
    responseString << "Date: " << strDate << "\r\n";
    responseString << "Content-Type: " << param.mimeType << "\r\n";
	if(param.contentLen){
		responseString << "Content-Length: " << param.contentLen << "\r\n";
	}	
	responseString << "\r\n";
    string strBuf = responseString.str();

    int totalLen = strBuf.length();
    const char *sendBuf = strBuf.c_str();

    int sendIdx = 0;
    int sendLen = 0;
    while (totalLen > 0) {
        sendLen = Send(&sendBuf[sendIdx], totalLen);
        if (sendLen <= 0)
            break;
        sendIdx += sendLen;
        totalLen -= sendLen;
    }
}

void HttpClient::Response404() {s_err(__func__);
    stringstream responseString;
    responseString << "HTTP/1.1 404 Not Found\r\n";
    responseString << "Connection: Closed\r\n";
    responseString << "content-type: text/html; charset=UTF-8\r\n";
	responseString << "\r\n";
    responseString << "<html><head><title>404 Not Found</title></head><body><h1>Not Found</h1></body></html>";
    string strBuf = responseString.str();
    int totalLen = strBuf.length();
    const char *sendBuf = strBuf.c_str();
    int sendIdx = 0;
    int sendLen = 0;
    while (totalLen > 0) {
        sendLen = Send(&sendBuf[sendIdx], totalLen);
        if (sendLen <= 0)
            break;
        sendIdx += sendLen;
        totalLen -= sendLen;
    }
}

void HttpClient::HandleResponse(std::string req) {
//-----------------GET HANDLE-----------------	
	s_inf("---:%s",req.data());
	int idx = req.find("GET /");
	if( idx >= 0 ){
		int cmd_end = req.find_first_of('?');
		if(cmd_end < 0){
			s_err("cmd end not found");
			goto UNSURPRT_EXIT;
		}		
		size_t cmd_len = cmd_end - idx - strlen("GET /");
		std::string cmd = req.substr(idx + strlen("GET /"),cmd_len);
		if(cmd.compare("getVideo")==0){
			idx = req.find_first_of("& ",cmd_end+1);
			s_inf("idx:%d",idx);
			std::string path = req.substr(cmd_end+1,idx-(cmd_end+1));
			s_inf("len:%d",idx-(cmd_end+1));
			s_inf("path:%s",path.data());
			httpHeadParam HeadParam={
				.mimeType="video/mp4",
			};
			path_get_size(&HeadParam.contentLen,(char *)path.data());
			ResponseHead(HeadParam);
			SendStreaming(path.data());
			return;
		}
		if(cmd.compare("message")==0){
					
		}
	}
//-----------------POST HANDLE-----------------		
	idx = req.find("POST /");
	if( idx >= 0 ){
		
	}
//-----------------PUT HANDLE-----------------		
	idx = req.find("PUT /");
	if( idx >= 0 ){
		
	}
UNSURPRT_EXIT:
	Response404();
}
void show_hex_in_file(const char *buf,size_t len,const char *path){
	FILE* fp = fopen(path,"a+");
	if(!fp){
		s_err(__func__);
		return;
	}
	int i = 0;
	for(;i<len;i++){
		fprintf(fp,"0x%02x ",buf[i]);
		if(i && i%30 == 0){
			fprintf(fp,"\n");
		}
	}
	fprintf(fp,"\n");
	fprintf(fp,"-----------------------------------------------------\n");
	fclose(fp);
}
void HttpClient::HandleRequestThread() {
	std::string req;
	char buf[1024]="";
	bool is_find_http_end = false;
	for(int i = 0; i< 50;i++){
		memset(buf,0,sizeof(buf));
		int res = Recv(buf,sizeof(buf));
		if(res > 0){
			req += buf;
		}
		res = req.find("\r\n\r\n");
		if(res >= 0){
			s_inf("find out the http end!");			
			is_find_http_end = true;
			break;
		}
		res = req.find("\n\n");
		if(res >= 0){
			s_inf("find out the http end!");			
			is_find_http_end = true;
			break;
		}
	}
	if(req.size() < 16){
		return;
	}
	// show_hex_in_file(req.data(),req.size(),"./dump.txt");
	if(!is_find_http_end){
		s_err("can't find http end!");	
		Response404();
		return;
	}
    if(m_exitWork) {
		Response404();
        return;
    }
	
	HandleResponse(req.c_str());
    m_responseStep = RESPONSE_FINISH;
    closeSocket();
}

void HttpClient::Stop() {
    m_exitWork = true;
    if(m_threadResponse.joinable()) {
        m_threadResponse.join();
    }
}

bool HttpClient::HandleRequest() {
    m_responseStep = RESPONSE_START;
    m_threadResponse = thread(&HttpClient::HandleRequestThread, this);
    return true;
}

int WebServer::Bind() {s_inf(__func__);
    int ret = WEBSERVER_ERROR::NO_ERROR;
    int yes = 1;
    char addr_service[8] = {0};
    sprintf(addr_service, "%d", m_bindPort);

    struct addrinfo hints;
    struct addrinfo *result, *rp;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // use IPv4 or IPv6, whichever
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // fill in my IP for me

    ret = getaddrinfo(NULL, addr_service, &hints, &result);

    if(ret != 0) {
		show_errno(0,__func__);
        return WEBSERVER_ERROR::ERROR_GETADDR;
    }

    ret = WEBSERVER_ERROR::NO_ERROR;
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        m_sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (m_sockfd == -1)
            continue;

        /* "address already in use" */
        if( setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1 )
        {
			show_errno(0,__func__);
            break;
        }

        if (bind(m_sockfd, rp->ai_addr, rp->ai_addrlen) == 0) {
			s_inf("BindSuccess");
            break;
        }
		s_inf("rp=%lu",rp);
        // bind failed, and continue
        close(m_sockfd);
    }

    if (rp == NULL) {
		show_errno(0,"bind");
        return WEBSERVER_ERROR::ERROR_BIND;
    }
    freeaddrinfo(result);
	s_inf("rp=%lu",rp);
    return ret;
}

int WebServer::Listen() {
    if (listen(m_sockfd, SOCK_MAX_CONN) != 0)
        return WEBSERVER_ERROR::ERROR_LISTEN;

    return WEBSERVER_ERROR::NO_ERROR;
}



int WebServer::Run() {
    int ret = WEBSERVER_ERROR::NO_ERROR;
    ret = Bind();
    if (ret != WEBSERVER_ERROR::NO_ERROR) {
        return ret;
    }

    ret = Listen();
    if (ret != WEBSERVER_ERROR::NO_ERROR) {
        return ret;
    }

    FD_ZERO(&m_readfds);    // http server only care about read
    FD_SET(m_sockfd, &m_readfds);

    int client_sockfd;
    int max_sockfd = 0;
    fd_set tmp_fd;
    struct timeval timeout;
    static struct timeval ubus_sent_time = {0, 0};
    static struct timeval current_time, diff_time;

    max_sockfd = max(max_sockfd, m_sockfd);
    while (1) {
        // reset timeout
        timeout.tv_sec = 0;
		timeout.tv_usec = 200000;
        tmp_fd = m_readfds; // must using tmp_fd to every time, or select will not work

        if(m_exitWork) {
            s_inf("exit");
            break;
        }

        if (select(max_sockfd + 1, &tmp_fd, NULL, NULL, &timeout) == -1) {
			s_err("selectError");
            return WEBSERVER_ERROR::ERROR_SELECT;
        }
        if (FD_ISSET(m_sockfd, &tmp_fd)) {
            // server part
            if ((client_sockfd = accept(m_sockfd, NULL, 0)) != -1) {
                HttpClient *newClient = new HttpClient(client_sockfd);
                m_clients.push_back(newClient);
                FD_SET(client_sockfd, &m_readfds);
                max_sockfd = max(max_sockfd, client_sockfd);
            }
        } else {
            list<HttpClient*>::iterator it = m_clients.begin();
            for (; it != m_clients.end(); ++it) {
                if ((*it)->IsWaitingRequest() && (*it)->isReadable()) {
					s_inf("%s:GetSocketFD=%d",__func__,(*it)->GetSocketFD());
                    FD_CLR((*it)->GetSocketFD(), &m_readfds);
                    (*it)->HandleRequest();
                }
            }
            for (it = m_clients.begin(); it != m_clients.end();) {
                if ((*it)->IsFinishResponse()) {
                    (*it)->Stop();
                    delete (*it);
                    it = m_clients.erase(it);
                } else {
                    it++;
                }
            }
        }
    }
    return 0;
}

void WebServer::Stop() {
    m_exitWork = true;
    if (m_mainLoopThread.joinable()) {
        m_mainLoopThread.join();
    }

    list<HttpClient*>::iterator it = m_clients.begin();
    for (;it != m_clients.end(); ++it) {
        (*it)->Stop();
        delete (*it);
    }
    m_clients.clear();

    if(m_sockfd) {
        close(m_sockfd);
        m_sockfd = -1;
    }
}
WebServer::~WebServer(){
	
}
WebServer::WebServer() {s_trc(__func__);
    if (m_bindPort == 0) {
        m_bindPort = DEFAULT_SERVER_PORT;
    }
    m_mainLoopThread = thread(&WebServer::Run, this);
}
static int help_info(int argc ,char *argv[]){
	printf("%s help:\n",get_last_name(argv[0]));
	printf("\t-r [share path]\n");
	printf("\t-l [logLvCtrl]\n");
	printf("\t-p [logPath]\n");
	printf("\t-h show help\n");
	return 0;
}
#include <getopt.h>
int main(int argc,char *argv[]){
	int opt = 0;
	while ((opt = getopt_long_only(argc, argv, "r:l:p:dh",NULL,NULL)) != -1) {
		switch (opt) {
		case 'l':
			lzUtils_logInit(optarg,NULL);
			break;
		case 'p':
			lzUtils_logInit(NULL,optarg);
			break;
		case 'r':
			resouce_path=optarg;
			break;
		default: /* '?' */
			return help_info(argc ,argv);
	   }
	}	
//打印编译时间
	showCompileTmie(argv[0],s_war);
	
	WebServer * webaerv = new WebServer();
	if(!webaerv){
		return -1;
	}
	while(1){
		sleep(1);
	}
	delete webaerv;
}