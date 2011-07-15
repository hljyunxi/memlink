#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include "net.h"

/* check whethersocket is connected
 * return 0 socket is valid
 * return -1 socket is invalid casused by peer close the connection
 *
 */ 
int checksock(int socket)
{
    if(socket<=0)
        return -1;
    int epfd = epoll_create(1);
    if(epfd<0){
        fprintf(stderr,"%s epoll_create epfd=%d,errno=%d\n",__FUNCTION__,epfd,errno);
        return -1;
    }
    struct epoll_event ev;
    ev.data.fd= socket;
    ev.events = EPOLLIN;
    epoll_ctl(epfd,EPOLL_CTL_ADD,socket,&ev);
    struct epoll_event events[2];
    int ret = epoll_wait(epfd,events,2,0);
    if(ret<0){
        fprintf(stderr,"%s epoll_wait ret=%d,errno=%d\n",__FUNCTION__,ret,errno);
        close(epfd);
        return -1;
    }
    if((ret>0) && (events[0].events&EPOLLIN) && (events[0].data.fd==socket)){
        fprintf(stderr,"socket %d readable,it recv RST\n",socket);
        close(epfd);
        return -1;
    }
    close(epfd);
    return 0;
}

int connect_ts(int socket, const char *ip, int port,int ms)
{
    struct sockaddr_in to;
    to.sin_family = AF_INET;
    to.sin_addr.s_addr = inet_addr(ip);
    to.sin_port = htons(port);
    return connect_t(socket,(struct sockaddr *)&to,sizeof(to),ms);
}

int connect_t(int socket, const struct sockaddr *addr,socklen_t addrlen,int ms)
{
    int epfd = epoll_create(1);
    if(epfd<0){
        fprintf(stderr,"%s epoll_create epfd=%d,errno=%d\n",__FUNCTION__,epfd,errno);
        return -1;
    }
    struct epoll_event ev;
    ev.data.fd = socket;
    ev.events = EPOLLOUT;
    int flags=fcntl(socket,F_GETFL,0);
    fcntl(socket,F_SETFL,flags|O_NONBLOCK);
    epoll_ctl(epfd,EPOLL_CTL_ADD,socket,&ev);
    
    int ret = connect(socket,addr,addrlen);
    if(ret == -1 && errno!= EINPROGRESS){
        fprintf(stderr,"%s connect ret=%d,errno=%d\n",ret,errno);
        goto err;
    }
    struct epoll_event events[2];
    ret = epoll_wait(epfd,events,2,ms);
    if(ret <=0){
        fprintf(stderr,"%s epoll_wait ret=%d,errno=%d\n",__FUNCTION__,ret,errno);
        goto err;
    }
    if((ret>0)&& (events[0].events&EPOLLOUT) && (events[0].data.fd==socket)){
        int error = 0;
        socklen_t len = sizeof(int);
        if( 0==getsockopt(socket,SOL_SOCKET,SO_ERROR,&error,&len)){
            if(0==error){
                fcntl(socket,F_SETFL,flags);
                close(epfd);
                return 0;
            }
        }
        fprintf(stderr,"%s connect ok,but errno=%d\n",__FUNCTION__,error);
    }
err:
    fcntl(socket,F_SETFL,flags);
    close(epfd);
    return -1;
}

ssize_t send_t(int socket, const void *buffer, size_t length, int flags,int ms)
{
    int epfd = epoll_create(1);
    if(epfd<0){
        fprintf(stderr,"%s epoll_create epfd=%d,errno=%d\n",__FUNCTION__,epfd,errno);
        return -1;
    }
    struct epoll_event ev;
    ev.data.fd= socket;
    ev.events = EPOLLOUT;
    epoll_ctl(epfd,EPOLL_CTL_ADD,socket,&ev);
    struct epoll_event events[2];
    int ret = epoll_wait(epfd,events,2,ms);
    if(ret<=0){
        fprintf(stderr,"%s epoll_wait ret=%d,errno=%d\n",__FUNCTION__,ret,errno);
        close(epfd);
        return ret;
    }
    if((ret>0) &&(events[0].events&EPOLLOUT) && (events[0].data.fd == socket)){
        close(epfd);
        return send(socket,buffer,length,flags);
    }
    
    close(epfd);
    return -1;
}

ssize_t recv_t(int socket, void *buffer, size_t length, int flags,int ms)
{
    int epfd = epoll_create(1);
    if(epfd<0){
        fprintf(stderr,"%s epoll_create epfd=%d,errno=%d\n",__FUNCTION__,epfd,errno);
        return -1;
    }
    struct epoll_event ev;
    ev.data.fd= socket;
    ev.events = EPOLLIN;
    epoll_ctl(epfd,EPOLL_CTL_ADD,socket,&ev);
    struct epoll_event events[2];
    int ret = epoll_wait(epfd,events,2,ms);
    if(ret<=0){
        fprintf(stderr,"%s epoll_wait ret=%d,errno=%d\n",__FUNCTION__,ret,errno);
        close(epfd);
        return ret;
    }
    if((ret>0) && (events[0].events&EPOLLIN) && (events[0].data.fd == socket)){
        close(epfd);
        return recv(socket,buffer,length,flags);
    }

    close(epfd);
    return -1;
}

