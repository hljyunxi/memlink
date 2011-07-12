/**
 * unix socket package
 * @file network.c
 * @author zhaowei
 * @ingroup memlink
 * @{
 */
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include<arpa/inet.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include "logfile.h"
#include "myconfig.h"
#include "network.h"


int 
tcp_socket_server(char * host,int port)
{
    int fd;
    int ret;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        char errbuf[1024];
        strerror_r(errno, errbuf, 1024);
        DFATALERR("create socket error: %s\n",  errbuf);
        return -1;
    }

    tcp_server_setopt(fd);
    
    struct sockaddr_in  sin;

    sin.sin_family = AF_INET;
    sin.sin_port = htons((short)port);
    if (NULL == host || 0 == host[0]) {
        sin.sin_addr.s_addr = htonl(INADDR_ANY);
    }else{
        sin.sin_addr.s_addr = inet_addr(host);
    }
    memset(&(sin.sin_zero), 0, sizeof(sin.sin_zero));

    DINFO("bind %s:%d\n", host,port);
    ret = bind(fd, (struct sockaddr*)&sin, sizeof(sin));
    if (ret == -1) {
        char errbuf[1024];
        strerror_r(errno, errbuf, 1024);
        DFATALERR("fd %d bind error:%d %s\n", fd,errno, errbuf);
        close(fd);
        return -3;
    }

    ret = listen(fd, 128);
    if (ret == -1) {
        char errbuf[1024];
        strerror_r(errno, errbuf, 1024);
        DFATALERR("listen error: %s\n",  errbuf);
        close(fd);
        return -4;
    }

    return fd;
}


int
tcp_socket_connect(char *host, int port, int timeout)
{
    int fd;
    int ret;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        char errbuf[1024];
        strerror_r(errno, errbuf, 1024);
        DERROR("create socket error: %s\n",  errbuf);
        return -1;
    }

    struct linger ling = {1, 0};
    ret = setsockopt(fd, SOL_SOCKET, SO_LINGER, (void *)&ling, sizeof(ling));
    if (ret != 0) {
        char errbuf[1024];
        strerror_r(errno, errbuf, 1024);
        DERROR("setsockopt LINGER error: %s\n",  errbuf);
        return -1;
    }

    int flags = 1;
    ret = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void*)&flags, sizeof(flags));
    if (ret != 0) {
        char errbuf[1024];
        strerror_r(errno, errbuf, 1024);
        DERROR("setsockopt NODELAY error: %s\n",  errbuf);
        return -1;
    }

    ret = setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&flags, sizeof(flags));
    if (ret != 0) {
        char errbuf[1024];
        strerror_r(errno, errbuf, 1024);
        DERROR("setsockopt KEEPALIVE error: %s\n",  errbuf);
        return -1;
    }
    
    struct sockaddr_in  sin;

    sin.sin_family = AF_INET;
    sin.sin_port = htons((short)port);
    if (NULL == host || 0 == host[0]) {
        sin.sin_addr.s_addr = htonl(INADDR_ANY);
    }else{
        sin.sin_addr.s_addr = inet_addr(host);
    }
    memset(&(sin.sin_zero), 0, sizeof(sin.sin_zero));

    DINFO("connect to %s:%d\n", host, port);
    ret = connect(fd, (struct sockaddr*)&sin, sizeof(sin));
    if (ret == -1) {
        char errbuf[1024];
        strerror_r(errno, errbuf, 1024);
        DERROR("connect error: %s\n",  errbuf);
        close(fd);
        return -3;
    }

    return fd;
}


int
tcp_server_setopt(int fd)
{
    int ret;
    int flags;

    if ((flags = fcntl(fd, F_GETFL, 0)) < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        char errbuf[1024];
        strerror_r(errno, errbuf, 1024);
        DERROR("setting O_NONBLOCK error: %s\n",  errbuf);
        close(fd);
        return -2;
    }
    
    flags = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void *)&flags, sizeof(flags));

    ret = setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&flags, sizeof(flags));
    if (ret != 0) {
        char errbuf[1024];
        strerror_r(errno, errbuf, 1024);
        DERROR("setsockopt KEEPALIVE error: %s\n",  errbuf);
    }

    struct linger ling = {0, 0};
    ret = setsockopt(fd, SOL_SOCKET, SO_LINGER, (void *)&ling, sizeof(ling));
    if (ret != 0) {
        char errbuf[1024];
        strerror_r(errno, errbuf, 1024);
        DERROR("setsockopt LINGER error: %s\n",  errbuf);
        perror("setsockopt");
    }

    ret = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void *)&flags, sizeof(flags));
    if (ret != 0) {
        char errbuf[1024];
        strerror_r(errno, errbuf, 1024);
        DERROR("setsockopt NODELAY error: %s\n",  errbuf);
    }

    return 0;
}
/**
 * @}
 */
