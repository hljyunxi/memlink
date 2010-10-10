#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/select.h>
#include <unistd.h>
#include "utils.h"
#include "logfile.h"
#include "common.h"

int 
timeout_wait(int fd, int timeout, int writing)
{
    if (timeout <= 0)
        return MEMLINK_TRUE;

    if (fd < 0) {
        return MEMLINK_ERR;
    }

    fd_set fds; 
    struct timeval tv;
    int n;

    tv.tv_sec  = (int)timeout;
    tv.tv_usec = (int)((timeout - tv.tv_sec) * 1e6);

    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    if (writing)
        n = select(fd+1, NULL, &fds, NULL, &tv);
    else 
        n = select(fd+1, &fds, NULL, NULL, &tv);

    if (n < 0) 
        return MEMLINK_ERR;

    if (n == 0)
        return MEMLINK_FALSE;

    return MEMLINK_TRUE; 
}

int
timeout_wait_read(int fd, int timeout)
{
	return timeout_wait(fd, timeout, 0);
}

int
timeout_wait_write(int fd, int timeout)
{
	return timeout_wait(fd, timeout, 1);
}

/**
 * readn - try to read n bytes with the use of a loop
 *
 * Return the bytes read. On error, -1 is returned.
 */
ssize_t 
readn(int fd, void *vptr, size_t n, int timeout)
{
    size_t  nleft;
    ssize_t nread;
    char    *ptr;

    ptr = vptr;
    nleft = n;

    while (nleft > 0) {
		if (timeout > 0 && timeout_wait_read(fd, timeout) != MEMLINK_TRUE) {
			break;
		}
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

/**
 * writen - write n bytes with the use of a loop
 *
 */
ssize_t
writen(int fd, const void *vptr, size_t n, int timeout)
{
    size_t  nleft;
    ssize_t nwritten;
    const char *ptr;

    ptr = vptr;
    nleft = n;

    while (nleft > 0) {
        //DINFO("try write %d via fd %d\n", (int)nleft, fd);
		if (timeout > 0 && timeout_wait_write(fd, timeout) != MEMLINK_TRUE) {
			break;
		}

        if ((nwritten = write(fd, ptr, nleft)) <= 0) {
            if (nwritten < 0 && errno == EINTR){
                nwritten = 0;
            }else{
                DERROR("writen error: %s\n", strerror(errno));
                //MEMLINK_EXIT;
                return -1;
            }
        }
        //DINFO("nwritten: %d\n", (int)nwritten);
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


int
timediff(struct timeval *start, struct timeval *end)
{
    return 0;
}


