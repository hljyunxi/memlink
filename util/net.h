
int checksock(int socket);
int connect_ts(int socket, const char *ip, int port,int ms);
int connect_t(int socket, const struct sockaddr *addr,socklen_t addrlen,int ms);
ssize_t send_t(int socket, const void *buffer, size_t length, int flags,int ms);
ssize_t sendto_t(int socket, const void *buffer, size_t length, int flags, const char *ip, int port,int ms);
ssize_t recv_t(int socket, void *buffer, size_t length, int flags,int ms);
ssize_t recvfrom_t(int socket, void *buffer, size_t length, int flags, const char *ip, int port,int ms);
