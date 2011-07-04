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
    struct epoll_event ev;
    ev.data.fd= socket;
    ev.events = EPOLLIN;
    epoll_ctl(epfd,EPOLL_CTL_ADD,socket,&ev);
    struct epoll_event events[2];
    int ret = epoll_wait(epfd,events,2,0);
    if(ret==-1){
        return -1;
    }
    if( (events[0].events&EPOLLIN) && (events[0].data.fd==socket)){
        return -1;
    }
    return 0;
}

int connect_ts(int socket, const const char *ip, int port,int ms)
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
    struct epoll_event ev;
    ev.data.fd = socket;
    ev.events = EPOLLOUT;
    int flags=fcntl(socket,F_GETFL,0);
    fcntl(socket,F_SETFL,flags|O_NONBLOCK);
    epoll_ctl(epfd,EPOLL_CTL_ADD,socket,&ev);
    
    int ret = connect(socket,addr,addrlen);
    if(ret == -1 && errno!= EINPROGRESS){
        printf("first connect err %d\n",errno);
        goto err;
    }
    struct epoll_event events[2];
    ret = epoll_wait(epfd,events,2,ms);
    if(ret == -1){
        printf("epoll_wait %d\n",ret);
        goto err;
    }
    if( (events[0].events&EPOLLOUT) && (events[0].data.fd==socket)){
        int error = 0;
        socklen_t len = sizeof(int);
        if( 0==getsockopt(socket,SOL_SOCKET,SO_ERROR,&error,&len)){
            if(0==error){
                fcntl(socket,F_SETFL,flags);
                return 0;
            }
        }
    printf("getsockopt err %d\n",error);
    }
err:
    fcntl(socket,F_SETFL,flags);
    return -1;
}

ssize_t send_t(int socket, const void *buffer, size_t length, int flags,int ms)
{
    int epfd = epoll_create(1);
    struct epoll_event ev;
    ev.data.fd= socket;
    ev.events = EPOLLOUT;
    epoll_ctl(epfd,EPOLL_CTL_ADD,socket,&ev);
    struct epoll_event events[2];
    int ret = epoll_wait(epfd,events,2,ms);
    if(ret<=0){
        return ret;
    }
    if((events[0].events&EPOLLOUT) && (events[0].data.fd == socket)){
        return send(socket,buffer,length,flags);
    }
    
    return -1;
}

ssize_t recv_t(int socket, void *buffer, size_t length, int flags,int ms)
{
    int epfd = epoll_create(1);
    struct epoll_event ev;
    ev.data.fd= socket;
    ev.events = EPOLLIN;
    epoll_ctl(epfd,EPOLL_CTL_ADD,socket,&ev);
    struct epoll_event events[2];
    int ret = epoll_wait(epfd,events,2,ms);
    if(ret<=0){
        return ret;
    }
    if((events[0].events&EPOLLIN) && (events[0].data.fd == socket)){
        return recv(socket,buffer,length,flags);
    }
    
    return -1;
}

