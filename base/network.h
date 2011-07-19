#ifndef MEMLINK_NETWORK_H
#define MEMLINK_NETWORK_H

#include <stdio.h>

int tcp_socket_server(char *host, int port);
int	tcp_socket_connect(char *host, int port, int timeout, int block);
int tcp_server_setopt(int fd);
int udp_sock_server(char *host, int port);
int set_noblock(int fd);

#endif
