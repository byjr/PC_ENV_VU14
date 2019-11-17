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
#include <vector>
#ifdef DEBUG
#include <arpa/inet.h>
#endif

#include <lzUtils/base.h>
#include <lzUtils/common/fd_op.h>
#include "SocServer.h"


#define DEFAULT_SERVER_PORT    10086

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

class ReqParam{
public:	
	std::string key;
	std::string value;	
};
class ReqContainer{
public:	
	std::string m_method;
	std::string m_cmd;
	std::vector<ReqParam> m_params;
	bool parse(std::string &req);
	bool paraParse(std::string &tail);
};
bool ReqContainer::paraParse(std::string &tail){
	std::string paraStr;
	bool isParseFinished = false;
	int idx = tail.find_first_of('&');
	if(idx < 0){
		idx = tail.find_first_of(' ');
		if(idx < 0){
			idx = tail.find("\r\n\r\n");
		}
		isParseFinished = true;
	}
	paraStr = tail.substr(0,idx);
	ReqParam para;
	int paraIdx = paraStr.find_first_of('=');
	if(paraIdx < 0){
		s_err("para format err('=' is not find).");
		return false;
	}
	para.key = paraStr.substr(0,paraIdx);
	para.value = paraStr.substr(paraIdx+1);
	m_params.push_back(para);
	if(isParseFinished){
		s_inf("m_method=%s",m_method.data());
		s_inf("m_cmd=%s",m_cmd.data());
		return true;
	}
	std::string newStr = tail.substr(idx+1);
	return paraParse(newStr);
}
bool ReqContainer::parse(std::string &req){
	std::string tail = req;
	int idx = tail.find(" /");
	if(idx < 0){
		return false;
	}	
	m_method = tail.substr(0,idx);
	tail = tail.substr(idx+2);
	idx = tail.find_first_of('?');
	if(idx < 0){
		idx = tail.find_first_of(' ');
		if(idx < 0){
			idx = tail.find("\r\n\r\n");
			m_cmd = tail.substr(0,idx);
			return true;
		}
		m_cmd = tail.substr(0,idx);
		return true;
	}
	m_cmd = tail.substr(0,idx);
	std::string newStr = tail.substr(idx+1);
	return paraParse(newStr);
}

void SockClient::HandleResponse(std::string &req) {
	ReqContainer reqCtn;
	if(!reqCtn.parse(req)){
		ResponseText("urlParseErr");
		s_err("url_parse_err");
		return;
	}
	s_war("method=%s",reqCtn.m_method.data());
	s_war("cmd=%s",reqCtn.m_cmd.data());
	for(auto para:reqCtn.m_params){
		s_war("key=%s",para.key.data());
		s_war("value=%s",para.value.data());
	}
	if(reqCtn.m_method.compare("GET")==0){
		if(reqCtn.m_cmd.compare("getVideo")==0){
			size_t pids = get_pids_by_name("req_process.sh",NULL,1);
			if(pids > 0){
				ResponseText("serverRecording");
				s_err("serverRecording cancle!");
				return ;
			}
			std::string url,time;
			for(auto para:reqCtn.m_params){
				if(para.key.compare("ftp")==0){
					url = para.value.data();
				}
				if(para.key.compare("time")==0){
					time = para.value.data();
				}			
			}
			s_inf("/root/bin/req_process.sh -u %s -t %s -r",url.data(),time.data());			
			getCmdResult("/root/bin/req_process.sh -u %s -t %s -r",url.data(),time.data());
			s_inf("reqProcessDone");
		}
	}
}
void SockClient::ResponseText(const char* txt){
	stringstream responseString;
	responseString << "serverReply";
	responseString << "?status=";
	responseString << txt;
	responseString << "\r\n\r\n";
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
bool SockClient::cmdResultParse(std::string& res){
	int idx = res.find("recStarted");
	if(idx >= 0){
		ResponseText("recStarted");
		s_inf("recStarted");
	}
	idx = res.find("cvtFailed");
	if(idx >= 0){
		ResponseText("cvtFailed");
		s_err("cvtFailed");
	}
	idx = res.find("upSucceed");
	if(idx >= 0){
		ResponseText("upSucceed");
		s_inf("upSucceed");		
	}
	idx = res.find("upFailed");
	if(idx >= 0){
		ResponseText("upFailed");
		s_err("upFailed");
	}
	return false;
}
std::string SockClient::getCmdResult(const char *fmt, ...){
	va_list args;
	va_start(args, (char *)fmt);
	char *cmd = NULL;
	vasprintf(&cmd, fmt, args);
	va_end(args);
	FILE *fp = popen(cmd,"r");
	if(!fp){
		s_err("popen");
		return "";
	}
	fcntl(fileno(fp), F_SETFD, FD_CLOEXEC);
	string cmdRes;
	do{
		char buf[1024]="";
		char *res = fgets(buf,sizeof(buf),fp);
		if(!res){
			break;
		}
		cmdRes += buf;
		cmdResultParse(cmdRes);
	}while(!feof(fp));
	fclose(fp);
	if(cmd)free(cmd);
	return cmdRes;
}
void SockClient::HandleRequestThread() {
	std::string req;
	char buf[1024]="";
	bool is_find_corect_end = false;
	for(int i = 0; i< 50;i++){
		memset(buf,0,sizeof(buf));
		int res = Recv(buf,sizeof(buf));
		if(res > 0){
			req += buf;
		}
		res = req.find("\r\n\r\n");
		if(res >= 0){
			s_inf("find out the http end!");			
			is_find_corect_end = true;
			break;
		}
	}
	if(req.size() < 16){
		return;
	}
	if(!is_find_corect_end){
		s_err("can't find http end!");
		ResponseText("getUrlErr");
		return;
	}
    if(m_exitWork) {
		ResponseText("serverExit");
        return;
    }	
	HandleResponse(req);
    m_responseStep = RESPONSE_FINISH;
    closeSocket();
}

void SockClient::Stop() {
    m_exitWork = true;
    if(m_threadResponse.joinable()) {
        m_threadResponse.join();
    }
}

bool SockClient::HandleRequest() {
    m_responseStep = RESPONSE_START;
    m_threadResponse = thread(&SockClient::HandleRequestThread, this);
    return true;
}

int SocServer::Bind() {
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

int SocServer::Listen() {
    if (listen(m_sockfd, SOCK_MAX_CONN) != 0)
        return WEBSERVER_ERROR::ERROR_LISTEN;

    return WEBSERVER_ERROR::NO_ERROR;
}



int SocServer::Run() {
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
                SockClient *newClient = new SockClient(client_sockfd);
                m_clients.push_back(newClient);
                FD_SET(client_sockfd, &m_readfds);
                max_sockfd = max(max_sockfd, client_sockfd);
            }
        } else {
            list<SockClient*>::iterator it = m_clients.begin();
            for (; it != m_clients.end(); ++it) {
                if ((*it)->IsWaitingRequest() && (*it)->isReadable()) {
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

void SocServer::Stop() {
    m_exitWork = true;
    if (m_mainLoopThread.joinable()) {
        m_mainLoopThread.join();
    }

    list<SockClient*>::iterator it = m_clients.begin();
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
SocServer::~SocServer(){
	
}
SocServer::SocServer() {s_trc(__func__);
    if (m_bindPort == 0) {
        m_bindPort = 10086;
    }
    m_mainLoopThread = thread(&SocServer::Run, this);
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
int SocServer_main(int argc,char *argv[]){
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
			// resouce_path=optarg;
			break;
		default: /* '?' */
			return help_info(argc ,argv);
	   }
	}	
//打印编译时间
	showCompileTmie(argv[0],s_war);
	
	SocServer * webaerv = new SocServer();
	if(!webaerv){
		return -1;
	}
	while(1){
		sleep(1);
	}
	delete webaerv;
}