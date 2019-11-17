#define TEST_PATH "/home/lz/work_space/PC_ENV_VU14/run_dir/jc.mp4"


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

#include "WebServer.h"

exten "C"{
	#include <lzUtils/common/fd_op.h>
}
#define DEFAULT_SERVER_PORT    HTTP_MUSIC_STREAM_PORT

#define SOCK_MAX_CONN   64
#define MAX_HEADER_SIZE 4096
#define SEND_LEN        4096

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

void HttpClient::ResponseMimeType(const char *mimeType) {
    stringstream responseString;

    time_t ts = time(NULL);
    static char strDate[128];
    struct tm *t = gmtime(&ts);
    strftime(strDate, sizeof(strDate), "%a, %d %b %Y %H:%M:%S GMT", t);

    responseString << "HTTP/1.1 200 OK\r\n";
    responseString << "Accept-Ranges: bytes\r\n";
    responseString << "Date: " << strDate << "\r\n";
    responseString << "Content-Type: " << mimeType << "\r\n";
	size_t bytes = 0;
	int ret = path_get_size(&bytes,TEST_PATH);
	if(ret < 0){
		s_err("path_get_size failed!");		
	}
	responseString << "Content-Length: " << bytes << "\r\n\r\n";
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

void HttpClient::Response404() {
    stringstream responseString;

    responseString << "HTTP/1.1 404 Not Found\r\n";
    responseString << "Connection: Closed\r\n";
    responseString << "content-type: text/html; charset=UTF-8\r\n\r\n";
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

bool HttpClient::SearchHeaderEnd(char *searchBuf, int endIdx) {
    int i = endIdx - 4;
    if (i < 0)
        return false;
    if (searchBuf[i] == '\r' && searchBuf[i+1] == '\n' &&
        searchBuf[i+2] == '\r' && searchBuf[i+3] == '\n') {
        return true;
    }
    return false;
}

void HttpClient::HandleResponse(const char * requestHeader) {
	s_inf("%s:requestHeader:%s",__func__,requestHeader)
    char *ptr = strstr((char*)requestHeader, "GET /");
	if(!ptr){
		Response404();
		return;
	}
    ptr += strlen("GET /");
	
    struct mimetype *m = &supported_mime_types[0];

    int testIdx = 0;
    int streamType = STREAM_NONE;
    while (m->extn) {
        if(strncmp(ptr, m->extn, strlen(m->extn)) == 0) {
            streamType = m->stream_type;
            break;
        }
        m++;
        testIdx++;
    }
    if (!m->extn) {
        Response404();
        return;
    }

	s_inf("%s:mimeType=%s",m->mime);
    ResponseMimeType(m->mime);

	if (streamType == STREAM_ATTACHMENT) {
        SendStreaming(TEST_PATH);
    } else {
		s_err("%s:readerNotSet",__func__)
        Response404();
    }
}

void HttpClient::HandleRequestThread() {
    char recvBuf[MAX_HEADER_SIZE];
    memset(recvBuf, 0, sizeof(recvBuf));
    int totalSize = MAX_HEADER_SIZE - 1;
    int recvSize = 0;
    int recvIdx = 0;
    bool bFoundEnd = false;
    while((bFoundEnd = SearchHeaderEnd(recvBuf, recvIdx)) == false && totalSize > 0) {
        recvSize = Recv(&recvBuf[recvIdx], totalSize);
        if(recvSize <= 0)
            break;
        recvIdx += recvSize;
        totalSize -= recvSize;
    }

    if(recvSize <= 0) {
		show_errno(0,"recvFailed");
        return;
    }
	
    if(m_exitWork) {
        return;
    }

    if(bFoundEnd == false) {
        Response404();
    } else {
        HandleResponse(recvBuf);
    }
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
int help_info(int argc ,char *argv[]){
	return 0;
}
#include <getopt.h>
int main(int argc,char *argv[]){
	int opt = 0;
	char * recDdevide=NULL,* plyDdevide=NULL;
	while ((opt = getopt_long_only(argc, argv, "i:o:l:p:dh",NULL,NULL)) != -1) {
		switch (opt) {
		case 'l':
			lzUtils_logInit(optarg,NULL);
			break;
		case 'p':
			lzUtils_logInit(NULL,optarg);
			break;
		case 'i':
			recDdevide=optarg;
			break;
		case 'o':
			plyDdevide=optarg;
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