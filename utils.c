#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include "utils.h"
#include "logfile.h"

ssize_t 
readn(int fd, void *vptr, size_t n)
{
    size_t  nleft;
    ssize_t nread;
    char    *ptr;

    ptr = vptr;
    nleft = n;

    while (nleft > 0) {
        if ((nread = read(fd, ptr, nleft)) < 0) {
            if (errno == EINTR) {
                nread = 0;
            }else {
                DERROR("readn error: %s\n", strerror(errno));
                //MEMLINK_EXIT;
                return -1;
            }
        }else if (nread == 0)
            break;
        nleft -= nread;
        ptr += nread;
    }

    return (n - nleft);
}

ssize_t
writen(int fd, const void *vptr, size_t n)
{
    size_t  nleft;
    ssize_t nwritten;
    const char *ptr;

    ptr = vptr;
    nleft = n;

    while (nleft > 0) {
        DINFO("try write %d via fd %d\n", (int)nleft, fd);
        if ((nwritten = write(fd, ptr, nleft)) <= 0) {
            if (nwritten < 0 && errno == EINTR){
                nwritten = 0;
            }else{
                DERROR("writen error: %s\n", strerror(errno));
                //MEMLINK_EXIT;
                return -1;
            }
        }
        DINFO("nwritten: %d\n", (int)nwritten);
        nleft -= nwritten;
        ptr += nwritten;
    }
    return n;
}

void 
printb(char *data, int datalen)
{
    int i, j;
    unsigned char c;

    for (i = 0; i < datalen; i++) {
        c = 0x80;
        for (j = 0; j < 8; j++) {
            if (c & data[datalen - i - 1]) {
                printf("1");
            }else{
                printf("0");
            }   
            c = c >> 1;
        }   
        printf(" ");
    }   
    printf("\n");
}

void 
printh(char *data, int datalen)
{
    int i;
    unsigned char c;

    for (i = datalen - 1; i >= 0; i--) {
        c = data[i];
        printf("%02x ", c);
    }   
    printf("\n");
}

char*
formatb(char *data, int datalen, char *buf, int blen)
{
    int i, j;
    unsigned char c;
    int idx = 0;
    int maxlen = blen - 1;

    buf[maxlen] = 0;

    for (i = 0; i < datalen; i++) {
        c = 0x80;
        for (j = 0; j < 8; j++) {
            if (c & data[datalen - i - 1]) {
                buf[idx] = '1';
            }else{
                buf[idx] = '0';
            }   
            idx ++;
            if (idx >= maxlen) {
                return buf;
            }
            c = c >> 1;
        }   
        buf[idx] = ' ';
        idx ++;

        if (idx >= maxlen) {
            return buf;
        }
    }   

    return buf;
}


char*
formath(char *data, int datalen, char *buf, int blen)
{
    int i;
    unsigned char c;
    int idx = 0;

    for (i = datalen - 1; i >= 0; i--) {
        c = data[i];
        snprintf(buf + idx, blen-idx, "%02x ", c);
        idx += 3;
    }   

    return buf;
}



