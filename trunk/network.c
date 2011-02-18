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
tcp_socket_server(int port)
{
    int fd;
    int ret;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        DERROR("create socket error: %s\n", strerror(errno));
        return -1;
    }

    tcp_server_setopt(fd);
    
	struct sockaddr_in  sin;

    sin.sin_family = AF_INET;
    sin.sin_port = htons((short)port);
    sin.sin_addr.s_addr = htonl(INADDR_ANY);

	DINFO("bind to: %d\n", port);
    ret = bind(fd, (struct sockaddr*)&sin, sizeof(sin));
    if (ret == -1) {
        DERROR("bind error: %s\n", strerror(errno));
        close(fd);
        return -3;
    }

    ret = listen(fd, 128);
    if (ret == -1) {
        DERROR("listen error: %s\n", strerror(errno));
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
        DERROR("create socket error: %s\n", strerror(errno));
        return -1;
    }

	struct linger ling = {1, 0};
    ret = setsockopt(fd, SOL_SOCKET, SO_LINGER, (void *)&ling, sizeof(ling));
    if (ret != 0) {
        DERROR("setsockopt LINGER error: %s\n", strerror(errno));
		return -1;
	}

    int flags = 1;
    ret = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void*)&flags, sizeof(flags));
    if (ret != 0) {
        DERROR("setsockopt NODELAY error: %s\n", strerror(errno));
		return -1;
	}

    ret = setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&flags, sizeof(flags));
    if (ret != 0) {
        DERROR("setsockopt KEEPALIVE error: %s\n", strerror(errno));
        return -1;
    }
    
	struct sockaddr_in  sin;

    sin.sin_family = AF_INET;
    sin.sin_port = htons((short)port);
    if (NULL == host) {
        sin.sin_addr.s_addr = htonl(INADDR_ANY);
    }else{
        sin.sin_addr.s_addr = inet_addr(host);
    }

	DINFO("connect to %s:%d\n", host, port);
    ret = connect(fd, (struct sockaddr*)&sin, sizeof(sin));
    if (ret == -1) {
        DERROR("connect error: %s\n", strerror(errno));
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
        DERROR("setting O_NONBLOCK error: %s\n", strerror(errno));
        close(fd);
        return -2;
    }
    
    flags = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void *)&flags, sizeof(flags));

    ret = setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&flags, sizeof(flags));
    if (ret != 0) {
        DERROR("setsockopt KEEPALIVE error: %s\n", strerror(errno));
    }

    struct linger ling = {0, 0};
    ret = setsockopt(fd, SOL_SOCKET, SO_LINGER, (void *)&ling, sizeof(ling));
    if (ret != 0) {
        DERROR("setsockopt LINGER error: %s\n", strerror(errno));
        perror("setsockopt");
    }

    ret = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void *)&flags, sizeof(flags));
    if (ret != 0) {
        DERROR("setsockopt NODELAY error: %s\n", strerror(errno));
    }

    return 0;
}
/**
 * @}
 */
