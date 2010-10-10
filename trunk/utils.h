#ifndef MEMLINK_UTILS_H
#define MEMLINK_UTILS_H

#include <stdio.h>
#include <sys/time.h>

int		timeout_wait(int fd, int timeout, int writing);
int		timeout_wait_read(int fd, int timeout);
int		timeout_wait_write(int fd, int timeout);

ssize_t readn(int fd, void *vptr, size_t n, int timeout);
ssize_t writen(int fd, const void *vptr, size_t n, int timeout);
void    printb(char *data, int datalen);
void    printh(char *data, int datalen);
char*   formatb(char *data, int datalen, char *buf, int blen);
char*   formath(char *data, int datalen, char *buf, int blen);

int     timediff(struct timeval *start, struct timeval *end);

#endif
