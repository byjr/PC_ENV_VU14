#ifndef UN_SOCKET_H_
#define UN_SOCKET_H_
#include <stdio.h>
#define __USE_GNU 1
#include <sys/types.h>/* See NOTES */
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#if 0 //接口概述
// __domain：AF_UNIX AF_INET 
// __type：SOCK_STREAM  SOCK_DGRAM SOCK_RAW | SOCK_NONBLOCK SOCK_CLOEXEC
// __protocol: 0
// 返回 成功: 0 ;失败 -1,并设置errno;
int socket(int domain, int type, int protocol);
// 只能用于亲缘进程间的通讯，即使共享内存也没用
int socketpair(int domain, int type, int protocol, int sv);
struct sockaddr {
   sa_family_t sa_family;
   char        sa_data[14];
}

int bind(int sockfd, const struct sockaddr *addr,socklen_t addrlen);
int listen(int sockfd, int backlog);
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
// flags:SOCK_NONBLOCK SOCK_CLOEXEC
int accept4(int sockfd,struct sockaddr *addr,socklen_t *addrlen,int flags);
int connect(int sockfd, const struct sockaddr *addr,socklen_t addrlen);
ssize_t send(int sockfd, const*buf,len, int flags);
ssize_t sendto(int sockfd, const*buf,len, int flags,const struct sockaddr *dest_addr, socklen_t addrlen);
ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags);
ssize_t recv(int sockfd,*buf,len, int flags);
ssize_t recvfrom(int sockfd,*buf,len, int flags,struct sockaddr *src_addr, socklen_t *addrlen);
ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags);
struct mmsghdr {
    struct msghdr msg_hdr; 
   int  msg_len; 
};
// __vlen:消息数
int sendmmsg(int sockfd, struct mmsghdr *msgvec,int vlen, int flags);
int recvmmsg(int sockfd, struct mmsghdr *msgvec,int vlen, int flags,*timeout);
// 获取本方地址
int getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
// 获取对方地址
int getpeername(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
//获取和设置选项
int getsockopt(int sockfd, int level, int optname,*optval, socklen_t *optlen);
int setsockopt(int sockfd, int level, int optname,const*optval, socklen_t optlen);
//关读，关写，关读写
int shutdown(int sockfd, int how);
// 是否处于带外（紧急）标记
//1.是;0否；-1.查询失败
int sockatmark(int sockfd);
//1.是;0否；-1.查询失败
// 文件是否为fdtype 类型
int isfdtype(int fd, int fdtype);

void FD_CLR(int fd, fd_set *set);
int  FD_ISSET(int fd, fd_set *set);
void FD_SET(int fd, fd_set *set);
void FD_ZERO(fd_set *set);
int select( int nfds, fd_set *readfds, fd_set *writefds,fd_set *exceptfds,\
				struct timeval *timeout);
int pselect(int nfds, fd_set *readfds, fd_set *writefds,fd_set *exceptfds,\
				const struct timespec *timeout,const sigset_t *sigmask);			
#endif
#define my_select(nFds,rSet,wSet,exSet,pTv,pSmsk) ({\
	int ret=pselect(nFds,rSet,wSet,exSet,pTv,pSmsk);\
	if(ret<0)show_errno(0,"my_select");\
	ret;\
})
#define ut_socket(domain, type, protocol) ({\
	int ret=socket(domain, type, protocol);\
	if(-1==ret)show_errno(0,"socket");\
	ret;\
})
#define ut_socketpair(domain, type, protocol, sv) ({\
	int ret=socketpair(domain, type, protocol,sv);\
	if(-1==ret)show_errno(0,"socketpair");\
	ret;\
})
#define my_bind(sockfd,p_addr,addrlen) ({\
	int ret=bind(sockfd,p_addr,addrlen);\
	if(-1==ret)show_errno(0,"bind");\
	ret;\
})
#define my_listen(sockfd, backlog) ({\
	int ret=listen(sockfd, backlog);\
	if(-1==ret)show_errno(0,"listen");\
	ret;\
})
#define my_accept(sockfd,p_sockaddr, p_addrlen) ({\
	int ret=accept(sockfd,p_sockaddr, p_addrlen);\
	if(-1==ret)show_errno(0,"accept");\
	ret;\
})
#define my_accept4(sockfd,p_sockaddr,p_addrlen,flags) ({\
	int ret=accept4(sockfd,p_sockaddr,p_addrlen,flags);\
	if(-1==ret)show_errno(0,"accept4");\
	ret;\
})
#define my_connect(sockfd,p_addr,addrlen) ({\
	int ret=connect(sockfd,p_addr,addrlen);\
	if(-1==ret)show_errno(0,"connect");\
	ret;\
})
#define my_send(sockfd, p_buf,len, flags) ({\
	ssize_t ret=send(sockfd, p_buf,len, flags);\
	if(-1==ret)show_errno(0,"send");\
	ret;\
})
#define my_sendto(sockfd, p_buf,len, flags,constp_sockaddr, addrlen) ({\
	ssize_t ret=sendto(sockfd, p_buf,len, flags,constp_sockaddr, addrlen);\
	if(-1==ret)show_errno(0,"sendto");\
	ret;\
})
#define my_sendmsg(sockfd,p_msg, flags) ({\
	ssize_t ret=sendmsg(sockfd,p_msg, flags);\
	if(-1==ret)show_errno(0,"sendmsg");\
	ret;\
})
#define my_recv(sockfd,p_buf,len, flags) ({\
	ssize_t ret=recv(sockfd,p_buf,len, flags);\
	if(-1==ret)show_errno(0,"recv");\
	ret;\
})
#define my_recvfrom(sockfd,p_buf,len, flags,p_sockaddr, p_addrlen) ({\
	ssize_t ret=recvfrom(sockfd,p_buf,len, flags,p_sockaddr, p_addrlen);\
	if(-1==ret)show_errno(0,"recvfrom");\
	ret;\
})
#define my_recvmsg(sockfd,p_msghdr, flags) ({\
	ssize_t ret=recvmsg(sockfd,p_msghdr, flags);\
	if(-1==ret)show_errno(0,"recvmsg");\
	ret;\
})
#define my_sendmmsg(sockfd,p_mmsghdr,vlen, flags) ({\
	int ret=sendmmsg(sockfd,p_mmsghdr,vlen, flags);\
	if(-1==ret)show_errno(0,"sendmmsg");\
	ret;\
})
#define my_recvmmsg(sockfd,p_mmsghdr,vlen, flags,p_timeout) ({\
	int ret=recvmmsg(sockfd,p_mmsghdr,vlen, flags,p_timeout);\
	if(-1==ret)show_errno(0,"recvmmsg");\
	ret;\
})
#define my_getsockname(sockfd,p_sockaddr, p_addrlen) ({\
	int ret=getsockname(sockfd,p_sockaddr, p_addrlen);\
	if(-1==ret)show_errno(0,"getsockname");\
	ret;\
})
#define my_getpeername(sockfd,p_sockaddr, p_addrlen) ({\
	int ret=getpeername(sockfd,p_sockaddr, p_addrlen);\
	if(-1==ret)show_errno(0,"getpeername");\
	ret;\
})
#define my_getsockopt(sockfd, level, optname,p_optval, p_optlen) ({\
	int ret=getsockopt(sockfd, level, optname,p_optval, p_optlen);\
	if(-1==ret)show_errno(0,"getsockopt");\
	ret;\
})
#define my_setsockopt(sockfd, level, optname,p_optval, optlen) ({\
	int ret=setsockopt(sockfd, level, optname,p_optval, optlen);\
	if(-1==ret)show_errno(0,"setsockopt");\
	ret;\
})
#define my_shutdown(sockfd, how) ({\
	int ret=shutdown(sockfd, how);\
	if(-1==ret)show_errno(0,"shutdown");\
	ret;\
})
#define my_sockatmark(sockfd) ({\
	int ret=sockatmark(sockfd);\
	if(-1==ret)show_errno(0,"sockatmark");\
	ret;\
})
#define my_isfdtype(fd, fdtype) ({\
	int ret=isfdtype(fd, fdtype);\
	if(-1==ret)show_errno(0,"isfdtype");\
	ret;\
})

int un_bind(int sfd,char* path);
int in_bind(int sfd,char *ip,in_port_t port);
int un_connect(int sfd,const char* path);
int in_connect(int sfd,char *ip,in_port_t port);

#define MSG_BUF_BYTE 1024
typedef int proc_t(int conn);
typedef struct net_serv_t{
	proc_t *pRcbk;
	proc_t *pWcbk;
	char *addr;
	in_port_t port;
	int fd;
}net_serv_t;

void un_select_tcp_server(net_serv_t *pServ);
void in_select_tcp_server(net_serv_t *pServ);
void in_select_udp_server(net_serv_t tbl[],size_t count);
void in_select_udp_server(net_serv_t tbl[],size_t count);
int un_tcp_cli_create(const char *path);
int in_tcp_cli_create(char *ip,in_port_t port);
int un_udp_cli_create(char *src,char *dst);
int in_udp_cli_create(char *ip,in_port_t port);

#define EVENTS_SIZE 10
#define my_epoll_create(flag) ({\
	int epFd=epoll_create1(flag);\
	if(-1==epFd)show_errno(0,"epoll_create1");\
	epFd;\
})
#define my_epoll_ctl(epFd,evFd,op,flags,pDat) ({\
    struct epoll_event ev;\
    ev.events=flags;\
	if(pDat){\
		ev.data.ptr = pDat;\
	}else{\
		ev.data.fd = evFd;\
	}\
	int ret=epoll_ctl(epFd,op,evFd,&ev);\
	if(-1==ret)show_errno(0,"epoll_ctl");\
	ret;\
})

#define add_ep_evt(epFd,fd,flags,pDat) my_epoll_ctl(epFd,fd,EPOLL_CTL_ADD,flags,pDat)
#define del_ep_evt(epFd,fd,flags,pDat) my_epoll_ctl(epFd,fd,EPOLL_CTL_DEL,flags,pDat)
#define mod_ep_evt(epFd,fd,flags,pDat) my_epoll_ctl(epFd,fd,EPOLL_CTL_MOD,flags,pDat)

#define my_epoll_wait(epFd,event_a,event_n,timeout,p_sigmask) ({\
	int n_ready=epoll_pwait(epFd,event_a,event_n,timeout,p_sigmask);\
	if(-1==n_ready)show_errno(0,"epoll_wait");\
	n_ready;\
})
void un_epoll_tcp_server(net_serv_t *pServ);
void un_epoll_udp_server(net_serv_t tbl[],size_t count);
void in_epoll_udp_server(net_serv_t tbl[],size_t count);
void in_epoll_tcp_server(net_serv_t *pServ);

#endif