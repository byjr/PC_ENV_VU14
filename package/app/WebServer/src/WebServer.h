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

/// pipe path of audio stream for mpd
#define IVS_ATTACHMENT_PIPE "http://localhost:8888/attach.mp4"
#define IVS_STREAM_PIPE "http://localhost:8888/istream.mp3"
#define HTTP_MUSIC_STREAM_PORT 8888

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
struct mimetype {
    const char *extn;
    const char *mime;
    const int   stream_type;
};

#define STREAM_NONE       -1
#define STREAM_ATTACHMENT 0
#define STREAM_ISTREAM    1
#define STREAM_URI        2

static struct mimetype supported_mime_types[] = {
    { "attach.mp4",    " application/octet-stream",        STREAM_ATTACHMENT},
    { "attach.aac",    "audio/mp4a-latm",   STREAM_ATTACHMENT},
    { "istream.mp3",   "audio/mpeg",        STREAM_ISTREAM},
    { "istream.aac",   "audio/mp4a-latm",   STREAM_ISTREAM},
    { NULL, NULL,  STREAM_NONE}
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

class HttpClient : public TCPSocket {
    enum {
        RESPONSE_WAITING,
        RESPONSE_START,
        RESPONSE_FINISH,
    };
private:
    std::shared_ptr<std::istream> m_stream;
    std::thread m_threadResponse;
    int m_responseStep;

private:
    bool SearchHeaderEnd(char *searchBuf, int endIdx);
    void Response404();
    void ResponseMimeType(const char *mimeType);
    void SendStreaming(const char* filename);
    void SendStreaming(std::shared_ptr<std::istream> stream);

public:
    HttpClient(int sockfd) {
        m_sockfd = sockfd;
        m_responseStep = RESPONSE_WAITING;
        FD_ZERO(&m_readfds);
        FD_ZERO(&m_writefds);
        FD_SET(m_sockfd, &m_readfds);
        FD_SET(m_sockfd, &m_writefds);
    }

    void HandleRequestThread();
    bool HandleRequest();
    void HandleResponse(const char* requestHeader);
    void Stop();
    bool IsWaitingRequest() { return m_responseStep == RESPONSE_WAITING; }
    bool IsFinishResponse() { return m_responseStep == RESPONSE_FINISH; }
};

class WebServer : public TCPSocket {
private:
    int m_bindPort;
    std::list<HttpClient*> m_clients;
    std::thread m_mainLoopThread;
    std::shared_ptr<std::istream> m_stream;

private:
    int Bind();
    int Listen();
public:
    WebServer();
    ~WebServer();	
    int Run();
    void Stop();
};

#endif
