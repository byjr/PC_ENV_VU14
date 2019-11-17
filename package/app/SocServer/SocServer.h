#ifndef __WEB_SERVER_H__
#define __WEB_SERVER_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <thread>
#include <netdb.h>
#include <list>
#include <unordered_map>
#include <lzUtils/base.h>

enum WEBSERVER_ERROR {
    NO_ERROR = 0,
    ERROR_GETADDR = -1,
    ERROR_BIND,
    ERROR_LISTEN,
    ERROR_SEND,
    ERROR_RECV,
    ERROR_SELECT,
    ERROR_NO_SOCKET,
    ERROR_NOT_READABLE,
    ERROR_NOT_WRITEABLE,
};
class TCPSocket {
protected:
    int m_sockfd;
    fd_set m_readfds;
    fd_set m_writefds;
    bool m_exitWork;

public:
    TCPSocket(){ m_exitWork = false; }
    ~TCPSocket();

    int Send(const char *buf, int len);
    int Recv(char * buf, int len);

    int GetSocketFD() { return m_sockfd; }
    bool isReadable();
    void closeSocket();
};

class SockClient : public TCPSocket {
    enum {
        RESPONSE_WAITING,
        RESPONSE_START,
        RESPONSE_FINISH,
    };
private:
    std::shared_ptr<std::istream> m_stream;
    std::thread m_threadResponse;
    int m_responseStep;

public:
    SockClient(int sockfd) {
        m_sockfd = sockfd;
        m_responseStep = RESPONSE_WAITING;
        FD_ZERO(&m_readfds);
        FD_ZERO(&m_writefds);
        FD_SET(m_sockfd, &m_readfds);
        FD_SET(m_sockfd, &m_writefds);
    }
	std::string getCmdResult(const char *fmt, ...);
	bool cmdResultParse(std::string& res);
	void ResponseText(const char* txt);	
	void HandleResponse(std::string &req) ;
    void HandleRequestThread();
    bool HandleRequest();
    void Stop();
    bool IsWaitingRequest() { return m_responseStep == RESPONSE_WAITING; }
    bool IsFinishResponse() { return m_responseStep == RESPONSE_FINISH; }
};

class SocServer : public TCPSocket {
private:
    int m_bindPort;
    std::list<SockClient*> m_clients;
    std::thread m_mainLoopThread;
    std::shared_ptr<std::istream> m_stream;

private:
    int Bind();
    int Listen();
public:
    SocServer();
    ~SocServer();	
    int Run();
    void Stop();
};

#endif
