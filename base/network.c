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
//#include "myconfig.h"
#include "network.h"
#include "defines.h"


int 
tcp_socket_server(char *host, int port)
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

    tcp_server_setopt(fd);
    
    struct sockaddr_in  sin;

    sin.sin_family = AF_INET;
    sin.sin_port = htons((short)port);
    if (host) {
        sin.sin_addr.s_addr = inet_addr(host);
    }else{
        sin.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    memset(&(sin.sin_zero), 0, sizeof(sin.sin_zero)); 

    DINFO("bind to: %d\n", port);
    ret = bind(fd, (struct sockaddr*)&sin, sizeof(sin));
    if (ret == -1) {
        char errbuf[1024];
        strerror_r(errno, errbuf, 1024);
        DERROR("bind error: %s\n",  errbuf);
        close(fd);
        return -1;
    }

    ret = listen(fd, 128);
    if (ret == -1) {
        char errbuf[1024];
        strerror_r(errno, errbuf, 1024);
        DERROR("listen error: %s\n",  errbuf);
        close(fd);
        return -1;
    }

    return fd;
}

int
udp_sock_server(char *host, int port)
{
    int fd;
    int ret;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) {
        char errbuf[1024];
        strerror_r(errno, errbuf, 1024);
        DERROR("create socket error: %s\n",  errbuf);
        return -1;
    }
    
    set_noblock(fd);

    struct sockaddr_in sin;

    sin.sin_family = AF_INET;
    sin.sin_port = htons((short)port);
    if (host == NULL || host[0] == 0) {
        sin.sin_addr.s_addr = htonl(INADDR_ANY);
    }else{
        sin.sin_addr.s_addr = inet_addr(host);
    }
    memset(&(sin.sin_zero), 0, sizeof(sin.sin_zero)); 

    DINFO("bind to: %d\n", port);
    ret = bind(fd, (struct sockaddr *)&sin, sizeof(sin));
    if (ret == -1) {
        char errbuf[1024];
        strerror_r(errno, errbuf, 1024);
        DERROR("bind error: %s\n",  errbuf);
        close(fd);
        return -1;
    }
    DINFO("----------fd: %d\n", fd);    
    return fd;
}

int
tcp_socket_connect(char *host, int port, int timeout, int block)
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
    
    if (block == FALSE)
        set_noblock(fd);

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

    DINFO("connect to %s:%d\n", host, port);
    do {
        ret = connect(fd, (struct sockaddr*)&sin, sizeof(sin));
    } while (ret == -1 && errno == EINTR);
    
    DINFO("ret: %d\n", ret);
    if (ret == -1) {
        if (errno == EINPROGRESS || errno == EALREADY || errno == EWOULDBLOCK || errno == 0 || errno == EISCONN)
            return fd;
        char errbuf[1024];
        strerror_r(errno, errbuf, 1024);
        DWARNING("connect error: %s\n",  errbuf);
        close(fd);
        return -1;
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
        return -1;
    }
    
    flags = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void *)&flags, sizeof(flags));

    ret = setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&flags, sizeof(flags));
    if (ret != 0) {
        char errbuf[1024];
        strerror_r(errno, errbuf, 1024);
        DERROR("setsockopt KEEPALIVE error: %s\n",  errbuf);
        return -1;
    }

    struct linger ling = {0, 0};
    ret = setsockopt(fd, SOL_SOCKET, SO_LINGER, (void *)&ling, sizeof(ling));
    if (ret != 0) {
        char errbuf[1024];
        strerror_r(errno, errbuf, 1024);
        DERROR("setsockopt LINGER error: %s\n",  errbuf);
        return -1;
    }

    ret = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void *)&flags, sizeof(flags));
    if (ret != 0) {
        char errbuf[1024];
        strerror_r(errno, errbuf, 1024);
        DERROR("setsockopt NODELAY error: %s\n",  errbuf);
        return -1;
    }

    return 0;
}

int
set_noblock(int fd)
{
    int flags;

    if ((flags = fcntl(fd, F_GETFL, 0)) < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        char errbuf[1024];
        strerror_r(errno, errbuf, 1024);
        DERROR("setting O_NONBLOCK error: %s\n",  errbuf);
        close(fd);
        return FAILED;
    }
    
    return OK;
}

int
set_block(int fd)
{
    int flag;

    flag = fcntl(fd, F_GETFL, 0);
    if (flag < 0) {
        char errbuf[1024];
        strerror_r(errno, errbuf, 1024);
        DERROR("fcntl F_GETFL error: %s\n",  errbuf);
        close(fd);
        return FAILED;
    }
    flag &= (~O_NONBLOCK);
    if (fcntl(fd, F_SETFL, flag) < 0) {
        char errbuf[1024];
        strerror_r(errno, errbuf, 1024);
        DERROR("fcntl F_SETFL error: %s\n",  errbuf);
        close(fd);
        return FAILED;
    }
    
    return OK;
}

/**
 * @}
 */
