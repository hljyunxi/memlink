#include "net.h"

int checksock(int socket)
{
    if(socket<=0)
        return -1;
    char buf[2];
    int nbread;
    int epfd = epoll_create(1);
    epoll_event ev;
    ev.data.fd= socket;
    ev.events = EPOLLIN;
    epoll_ctl(epfd,EPOLL_CTL_ADD,socket,&ev);
    epoll_event ev[2];
    ret = epoll_wait(epfd,ev,2,0);
    if(ret==-1){
        return -1;
    }
    if( (ev.events&EPOLLIN) && ev[0].data.fd==socket){
        printf("socket is invalide!\n")
        return -1;
        /* read one byte from socket */
        //nbread = recv(s, buf, 1, MSG_PEEK);
        //if (nbread <= 0)
        //    return -1;
    }
    return 0;
}


ssize_t send_t(int socket, const void *buffer, size_t length, int flags,int ms)
{
    int epfd = epoll_create(1);
    epoll_event ev;
    ev.data.fd= socket;
    ev.events = EPOLLOUT;
    epoll_ctl(epfd,EPOLL_CTL_ADD,socket,&ev);
    epoll_event ev[2];
    ret = epoll_wait(epfd,ev,2,ms);
    if(ret<=0){
        return ret;
    }
    if(ev.events&EPOLLOUT){
        return send(socket,buffer,length,flags);
    }
    
    return -1;
}

ssize_t recv_t(int socket, void *buffer, size_t length, int flags,int ms)
{
    int epfd = epoll_create(1);
    epoll_event ev;
    ev.data.fd= socket;
    ev.events = EPOLLIN;
    epoll_ctl(epfd,EPOLL_CTL_ADD,socket,&ev);
    epoll_event ev[2];
    ret = epoll_wait(epfd,ev,2,ms);
    if(ret<=0){
        return ret;
    }
    if(ev.events&EPOLLIN){
        return recv(socket,buffer,length,flags);
    }
    
    return -1;
}

