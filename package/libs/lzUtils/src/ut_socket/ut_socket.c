#include <stdlib.h>
#include "ut_socket.h"
#include "../common/misc.h"
#include "../slog/slog.h"
int un_bind(int sfd,char *path){
	unlink(path);
	struct sockaddr_un addr={AF_UNIX};
	strncpy(addr.sun_path,path,sizeof(addr.sun_path));
	int ret=my_bind(sfd,(struct sockaddr*)&addr,sizeof(addr));
	if(ret)return -1;
	return 0;
}
int un_connect(int sfd,const char *path){
	struct sockaddr_un addr={AF_UNIX};
	strncpy(addr.sun_path,path,sizeof(addr.sun_path));
	int ret=my_connect(sfd,(struct sockaddr*)&addr,sizeof(addr));
	if(ret)return -1;
	return 0;
}
int in_bind(int sfd,char *ip,in_port_t port){
	struct sockaddr_in addr={AF_INET};
	addr.sin_port=htons(port);
	addr.sin_addr.s_addr=inet_addr(ip);
	int ret=my_bind(sfd,(struct sockaddr*)&addr,sizeof(addr));
	if(ret)return -1;
	return 0;
}
int in_connect(int sfd,char *ip,in_port_t port){
	struct sockaddr_in addr={AF_INET};
	addr.sin_port=htons(port);
	addr.sin_addr.s_addr=inet_addr(ip);
	int ret=my_connect(sfd,(struct sockaddr*)&addr,sizeof(addr));
	if(ret)return -1;
	return 0;
}
int un_tcp_cli_create(const char *path){
	int sfd=ut_socket(AF_UNIX,SOCK_STREAM|SOCK_CLOEXEC,0);
	if(-1==sfd)return -1;
	int ret=un_connect(sfd,path);
	if(-1==ret)return -2;
	return sfd;
}
int in_tcp_cli_create(char *ip,in_port_t port){
	int sfd=ut_socket(AF_INET,SOCK_STREAM|SOCK_CLOEXEC,0);
	if(-1==sfd)return -1;
	int ret=in_connect(sfd,ip,port);
	if(-1==ret)return -2;
	return sfd;
}
int in_udp_cli_create(char *ip,in_port_t port){
	int sfd=ut_socket(AF_INET,SOCK_DGRAM,0);
	if(-1==sfd)return -1;
	int ret=in_connect(sfd,ip,port);
	if(-1==ret)return -2;
	return sfd;
}
int un_udp_cli_create(char *src,char *dst){
	int sfd=ut_socket(AF_UNIX,SOCK_DGRAM,0);
	if(-1==sfd)return -1;
	int ret=un_bind(sfd,src);
	if(-1==ret)return -2;
	if(NULL==dst) return sfd;
	ret=un_connect(sfd,dst);
	if(-1==ret)return -3;
	return sfd;
}
void in_epoll_tcp_server(net_serv_t *pServ){
	int sfd=ut_socket(AF_INET,SOCK_STREAM|SOCK_CLOEXEC,0);
	if(-1==sfd)exit(-1);
	pServ->fd=sfd;
	int ret=1;
	ret=setsockopt(sfd,SOL_SOCKET,SO_REUSEADDR,
		&ret,sizeof(ret));
	if(-1==ret)exit(-1);
		s_inf("%s,%d",pServ->addr,pServ->port);
	ret=in_bind(sfd,pServ->addr,pServ->port);
	if(-1==ret)exit(-1);
	ret=my_listen(sfd,SOMAXCONN);
	if(-1==ret)exit(-1);
	int epfd=my_epoll_create(0);
	if(epfd<0)exit(-1);
	add_ep_evt(epfd,sfd,EPOLLIN,NULL);
	struct epoll_event events[EVENTS_SIZE]={0};
	do{
		int nReady=my_epoll_wait(epfd,events,EVENTS_SIZE,-1,NULL);
		if(-1==nReady){
			if(4==errno)continue;
			exit(-1);
		}
		if(0==nReady)continue;
		struct sockaddr_in peerAddr={AF_INET};
		socklen_t peerLen=sizeof(peerAddr);
		int i=0;
		for(i=0;i<nReady;i++){
			int fd=events[i].data.fd;
			if(fd==sfd){
				if(nReady+1<=EVENTS_SIZE){
					int conn=my_accept(sfd,(struct sockaddr *)&peerAddr,&peerLen);
					if(0==conn)continue;
					add_ep_evt(epfd,conn,EPOLLIN,NULL);
				}
			}else if(EPOLLIN==events[i].events){
				ret=pServ->pRcbk(fd);
				if(0==ret){
					mod_ep_evt(epfd,fd,EPOLLOUT,NULL);
				}else if(1==ret){
					del_ep_evt(epfd,fd,EPOLLIN,NULL);
					s_inf("peer close fd:%d",fd);
				}
			}else if(EPOLLOUT==events[i].events){
				ret=pServ->pWcbk(fd);
				if(0==ret){
					mod_ep_evt(epfd,fd,EPOLLIN,NULL);
				}else if(1==ret){
					del_ep_evt(epfd,fd,EPOLLOUT,NULL);
					s_inf("peer close fd:%d",fd);
				}
			}
		}
	}while(1);
}
void in_epoll_udp_server(net_serv_t tbl[],size_t count){
	int epfd=my_epoll_create(0);
	if(epfd<0)exit(-1);
	size_t i=0,ret=0;
	for(i=0;i<count;i++){
		tbl[i].fd=ut_socket(AF_INET,SOCK_DGRAM,0);
		if(-1==tbl[i].fd)exit(-1);
		int ret=setsockopt(tbl[i].fd,SOL_SOCKET,SO_REUSEADDR,
			&ret,sizeof(ret));
		if(ret<0)exit(-1);
		ret=in_bind(tbl[i].fd,tbl[i].addr,tbl[i].port);
		if(-1==ret)exit(-1);
		add_ep_evt(epfd,tbl[i].fd,EPOLLIN,&tbl[i]);
	}
	struct epoll_event events[EVENTS_SIZE]={0};
	do{
		int nReady=my_epoll_wait(epfd,events,EVENTS_SIZE,-1,NULL);
		if(-1==nReady){
			if(4==errno)continue;
			exit(-1);
		}
		if(0==nReady)continue;
		struct sockaddr_in peerAddr={AF_INET};
		socklen_t peerLen=sizeof(peerAddr);
		int i=0;
		for(i=0;i<nReady;i++){
			net_serv_t* pServ=(net_serv_t*)events[i].data.ptr;
			int fd=pServ->fd;
			if(EPOLLIN==events[i].events){
				ret=pServ->pRcbk(fd);
				if(0==ret){
					mod_ep_evt(epfd,fd,EPOLLOUT,pServ);
				}else if(1==ret){
					del_ep_evt(epfd,fd,EPOLLIN,pServ);
					s_inf("peer close fd:%d",fd);
				}
			}else if(EPOLLOUT==events[i].events){
				ret=pServ->pWcbk(fd);
				if(0==ret){
					mod_ep_evt(epfd,fd,EPOLLIN,pServ);
				}else if(1==ret){
					del_ep_evt(epfd,fd,EPOLLOUT,pServ);
					s_inf("peer close fd:%d",fd);
				}
			}
		}
	}while(1);
}
void un_epoll_udp_server(net_serv_t tbl[],size_t count){
	int epfd=my_epoll_create(0);
	if(epfd<0)exit(-1);
	size_t i=0,ret=0;
	for(i=0;i<count;i++){
		tbl[i].fd=ut_socket(AF_UNIX,SOCK_DGRAM,0);
		if(-1==tbl[i].fd)exit(-1);
		unlink(tbl[i].addr);
		ret=un_bind(tbl[i].fd,tbl[i].addr);
		if(-1==ret)exit(-1);
		add_ep_evt(epfd,tbl[i].fd,EPOLLIN,&tbl[i]);
	}
	struct epoll_event events[EVENTS_SIZE]={0};
	do{
		int nReady=my_epoll_wait(epfd,events,EVENTS_SIZE,-1,NULL);
		if(-1==nReady){
			if(4==errno)continue;
			exit(-1);
		}
		if(0==nReady)continue;
		struct sockaddr_un peerAddr={AF_UNIX};
		socklen_t peerLen=sizeof(peerAddr);
		int i=0;
		for(i=0;i<nReady;i++){
			net_serv_t* pServ=(net_serv_t*)events[i].data.ptr;
			int fd=pServ->fd;
			if(EPOLLIN==events[i].events){
				ret=pServ->pRcbk(fd);
				if(0==ret){
					mod_ep_evt(epfd,fd,EPOLLOUT,pServ);
				}else if(1==ret){
					del_ep_evt(epfd,fd,EPOLLIN,pServ);
					s_inf("peer close fd:%d",fd);
				}
			}else if(EPOLLOUT==events[i].events){
				ret=pServ->pWcbk(fd);
				if(0==ret){
					mod_ep_evt(epfd,fd,EPOLLIN,pServ);
				}else if(1==ret){
					del_ep_evt(epfd,fd,EPOLLOUT,pServ);
					s_inf("peer close fd:%d",fd);
				}
			}
		}
	}while(1);
}
void un_epoll_tcp_server(net_serv_t *pServ){
	int sfd=ut_socket(AF_UNIX,SOCK_STREAM|SOCK_CLOEXEC,0);
	if(-1==sfd)exit(-1);
	pServ->fd=sfd;
	unlink(pServ->addr);
	int ret=un_bind(sfd,pServ->addr);
	if(-1==ret)exit(-1);
	ret=my_listen(sfd,SOMAXCONN);
	if(-1==ret)exit(-1);
	int epfd=my_epoll_create(0);
	if(epfd<0)exit(-1);
	add_ep_evt(epfd,sfd,EPOLLIN,NULL);
	struct epoll_event events[EVENTS_SIZE]={0};
	do{
		int nReady=my_epoll_wait(epfd,events,EVENTS_SIZE,-1,NULL);
		if(-1==nReady){
			if(4==errno)continue;
			exit(-1);
		}
		if(0==nReady)continue;
		struct sockaddr_un peerAddr={AF_UNIX};
		socklen_t peerLen=sizeof(peerAddr);
		int i=0;
		for(i=0;i<nReady;i++){
			int fd=events[i].data.fd;
			if(fd==sfd){
				if(nReady+1<=EVENTS_SIZE){
					int conn=my_accept(sfd,(struct sockaddr *)&peerAddr,&peerLen);
					if(0==conn)continue;
					add_ep_evt(epfd,conn,EPOLLIN,NULL);
				}
			}else if(EPOLLIN==events[i].events){
				ret=pServ->pRcbk(fd);
				if(0==ret){
					mod_ep_evt(epfd,fd,EPOLLOUT,NULL);
				}else if(1==ret){
					del_ep_evt(epfd,fd,EPOLLIN,NULL);
					s_inf("peer close fd:%d",fd);
				}
			}else if(EPOLLOUT==events[i].events){
				ret=pServ->pWcbk(fd);
				if(0==ret){
					mod_ep_evt(epfd,fd,EPOLLIN,NULL);
				}else if(1==ret){
					del_ep_evt(epfd,fd,EPOLLOUT,NULL);
					s_inf("peer close fd:%d",fd);
				}
			}
		}
	}while(1);
}
void un_select_tcp_server(net_serv_t *pServ){
	size_t i=0;
	int conn_a[FD_SETSIZE]={0};	
	for(i=0;i<getCount(conn_a);i++)conn_a[i]=-1;
	int sfd=ut_socket(AF_UNIX,SOCK_STREAM|SOCK_CLOEXEC,0);
	if(-1==sfd)exit(-1);
	unlink(pServ->addr);
	int ret=un_bind(sfd,pServ->addr);
	if(-1==ret)exit(-1);
	ret=my_listen(sfd,SOMAXCONN);
	if(-1==ret)exit(-1);
	int maxfd=sfd;
	fd_set rset={0},allset={0};
	FD_SET(maxfd,&allset);
	do{
		rset=allset;
		int nready=my_select(maxfd+1,&rset,NULL,NULL,NULL,NULL);
		if(nready<0){
			if(EINTR==errno)continue;//防止收到信号时退出
			show_errno(0,"my_select");
			exit(-1);
		}
		if(!nready)continue;
		int conn=0;
		while(FD_ISSET(sfd,&rset)){
			nready--;
			struct sockaddr_un peeraddr={0};
			socklen_t peerlen=sizeof(peeraddr);
			conn=my_accept(sfd,(struct sockaddr *)&peeraddr,&peerlen);
			if(conn<0)break;
			FD_SET(conn,&allset);
			if(conn>maxfd)maxfd=conn;
			for(i=0;i<getCount(conn_a);i++){
				if(-1==conn_a[i])conn_a[i]=conn;
			}
			break;
		}
		if(!nready)continue;
		for(i=0;i<FD_SETSIZE;i++){
			if(-1==conn_a[i])continue;
			while(FD_ISSET(conn_a[i],&rset)){
				nready--;
				ret=pServ->pRcbk(conn_a[i]);
				if(ret<0)break;
				if(1==ret){
					s_war("client close:%d",conn_a[i]);
					FD_CLR(conn_a[i],&allset);
					conn_a[i]=-1;
					break;
				}
				ret=pServ->pWcbk(conn_a[i]);	
				if(ret<0)break;
				break;
			}
			if(!nready)break;
		}
	}while(1);
}
void in_select_tcp_server(net_serv_t *pServ){
	size_t i=0;
	int conn_a[FD_SETSIZE]={0};
	for(i=0;i<getCount(conn_a);i++)conn_a[i]=-1;
	int sfd=ut_socket(AF_INET,SOCK_STREAM|SOCK_CLOEXEC,0);
	if(-1==sfd)exit(-1);
	pServ->fd=sfd;
	int ret=setsockopt(sfd,SOL_SOCKET,SO_REUSEADDR,
		&ret,sizeof(ret));
	if(-1==ret)exit(-1);
	ret=in_bind(sfd,pServ->addr,pServ->port);
	if(-1==ret)exit(-1);
	ret=my_listen(sfd,SOMAXCONN);
	if(-1==ret)exit(-1);
	int maxfd=sfd;
	fd_set rset={0},allset={0};
	FD_SET(maxfd,&allset);
	do{
		rset=allset;
		int nready=my_select(maxfd+1,&rset,NULL,NULL,NULL,NULL);
		if(nready<0){
			if(EINTR==errno)continue;//防止收到信号时退出
			show_errno(0,"my_select");
			exit(-1);
		}
		if(!nready)continue;
		int conn=0;
		while(FD_ISSET(sfd,&rset)){
			nready--;
			struct sockaddr_in peeraddr={0};
			socklen_t peerlen=sizeof(peeraddr);
			conn=my_accept(sfd,(struct sockaddr *)&peeraddr,
				&peerlen);
			if(conn<0)break;
			s_inf("(peer)conn:%d ip:%s port:0x%4x",conn,
				inet_ntoa(peeraddr.sin_addr),peeraddr.sin_port);
			FD_SET(conn,&allset);
			if(conn>maxfd)maxfd=conn;
			for(i=0;i<getCount(conn_a);i++){
				if(-1==conn_a[i]){
					conn_a[i]=conn;break;
				}
			}
			break;
		}
		if(!nready)continue;
		for(i=0;i<FD_SETSIZE;i++){
			if(-1==conn_a[i])continue;
			while(FD_ISSET(conn_a[i],&rset)){
				nready--;
				ret=pServ->pRcbk(conn_a[i]);
				if(ret<0)break;
				if(1==ret){
					s_war("client close:%d",conn_a[i]);
					FD_CLR(conn_a[i],&allset);
					conn_a[i]=-1;
					break;
				}
				ret=pServ->pWcbk(conn_a[i]);	
				if(ret<0)break;
				break;
			}
			if(!nready)break;
		}
	}while(1);
}
void in_select_udp_server(net_serv_t tbl[],size_t count){
	size_t i=0,ret=0;
	fd_set rset={0},allset={0};
	for(i=0;i<count;i++){
		tbl[i].fd=ut_socket(AF_INET,SOCK_DGRAM,0);
		if(-1==tbl[i].fd)exit(-1);
		int ret=setsockopt(tbl[i].fd,SOL_SOCKET,SO_REUSEADDR,
			&ret,sizeof(ret));
		if(ret<0)exit(-1);
		ret=in_bind(tbl[i].fd,tbl[i].addr,tbl[i].port);
		if(-1==ret)exit(-1);
		FD_SET(tbl[i].fd,&allset);
	}
	int maxfd=tbl[count-1].fd;
	FD_SET(maxfd,&allset);
	do{
		memcpy(&rset,&allset,sizeof(rset));
		int nready=my_select(maxfd+1,&rset,NULL,NULL,NULL,NULL);
		if(nready<0){
			if(EINTR==errno)continue;//防止收到信号时退出
			exit(-1);
		}
		if(!nready)continue;
		for(i=0;i<count;i++){
			if(-1==tbl[i].fd)continue;
			while(FD_ISSET(tbl[i].fd,&rset)){
				nready--;
				ret=tbl[i].pRcbk(tbl[i].fd);
				if(ret<0)break;
				if(1==ret){
					s_war("client close:%d",tbl[i].fd);
					FD_CLR(tbl[i].fd,&allset);
					tbl[i].fd=-1;
					break;
				}
				ret=tbl[i].pWcbk(tbl[i].fd);
				if(ret<0)break;
				break;
			}
			if(!nready)break;
		}
	}while(1);
}
void un_select_udp_server(net_serv_t tbl[],size_t count){
	size_t i=0,ret=0;
	fd_set rset={0},allset={0};
	for(i=0;i<count;i++){
		tbl[i].fd=ut_socket(AF_UNIX,SOCK_DGRAM,0);
		if(-1==tbl[i].fd)exit(-1);
		unlink(tbl[i].addr);
		ret=un_bind(tbl[i].fd,tbl[i].addr);
		if(-1==ret)exit(-1);
		FD_SET(tbl[i].fd,&allset);
	}
	int maxfd=tbl[count-1].fd;
	FD_SET(maxfd,&allset);
	do{
		memcpy(&rset,&allset,sizeof(rset));
		int nready=my_select(maxfd+1,&rset,NULL,NULL,NULL,NULL);
		if(nready<0){
			if(EINTR==errno)continue;//防止收到信号时退出
			exit(-1);
		}
		if(!nready)continue;
		for(i=0;i<count;i++){
			if(-1==tbl[i].fd)continue;
			while(FD_ISSET(tbl[i].fd,&rset)){
				nready--;
				ret=tbl[i].pRcbk(tbl[i].fd);
				if(ret<0)break;
				if(1==ret){
					s_war("client close:%d",tbl[i].fd);
					FD_CLR(tbl[i].fd,&allset);
					tbl[i].fd=-1;
					break;
				}
				ret=tbl[i].pWcbk(tbl[i].fd);
				if(ret<0)break;
				break;
			}
			if(!nready)break;
		}
	}while(1);
}