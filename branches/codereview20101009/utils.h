#ifndef MEMLINK_UTILS_H
#define MEMLINK_UTILS_H

#include <stdio.h>

ssize_t readn(int fd, void *vptr, size_t n);
ssize_t writen(int fd, const void *vptr, size_t n);
void    printb(char *data, int datalen);
void    printh(char *data, int datalen);
char*   formatb(char *data, int datalen, char *buf, int blen);
char*   formath(char *data, int datalen, char *buf, int blen);

#endif
