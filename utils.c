/**
 * 通用函数
 * @file utils.c
 * @author zhaowei
 * @ingroup memlink
 * @{
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/select.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <sys/stat.h>
#include <pthread.h>
#include <signal.h>
#include <pwd.h>
#include "utils.h"
#include "logfile.h"
#include "common.h"


int 
timeout_wait(int fd, int timeout, int writing)
{
    if (timeout <= 0)
        return MEMLINK_TRUE;

    if (fd < 0) {
        DERROR("fd error: %d\n", fd);
        return MEMLINK_ERR;
    }

    fd_set fds; 
    struct timeval tv;
    int n;

    while (1) {
        tv.tv_sec  = (int)timeout;
        tv.tv_usec = (int)((timeout - tv.tv_sec) * 1e6);

        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        if (writing)
            n = select(fd+1, NULL, &fds, NULL, &tv);
        else 
            n = select(fd+1, &fds, NULL, NULL, &tv);
        //DINFO("select return: %d\n", n);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            DERROR("select error: %d, %s\n", n, strerror(errno));
            return MEMLINK_ERR;
        }
        break;
    }

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
            DERROR("read timeout.\n");
			break;
		}
        if ((nread = read(fd, ptr, nleft)) < 0) {
            //DERROR("nread: %d, error: %s\n", nread, strerror(errno));
            if (errno == EINTR) {
                nread = 0;
            }else {
                DERROR("readn error: %s\n", strerror(errno));
                //MEMLINK_EXIT;
                return -1;
            }
        }else if (nread == 0) {
            DERROR("read 0, maybe conn close.\n");
            break;
        }
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
        //char buf[1024];
        //DINFO("nwritten: %d, %s\n", (int)nwritten, formath((char*)ptr, (int)nwritten, buf, 1024));
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
    buf[idx] = 0;

    return buf;
}


char*
formath(char *data, int datalen, char *buf, int blen)
{
    int i;
    unsigned char c;
    int idx = 0;
	int maxlen = blen - 4;

    for (i = datalen - 1; i >= 0; i--) {
		if (idx >= maxlen)
			break;
        c = data[i];
        snprintf(buf + idx, blen-idx, "%02x ", c);
        idx += 3;
    }   
    buf[idx] = 0;

    return buf;
}

void
timestart(struct timeval *start)
{
    gettimeofday(start, NULL);
}

unsigned int
timeend(struct timeval *start, struct timeval *end, char *info)
{
    gettimeofday(end, NULL);
    unsigned int td = timediff(start, end);

    DNOTE("=== %s time: %u us ===\n", info, td);
    return td;
}

unsigned int 
timediff(struct timeval *start, struct timeval *end)
{
	return 1000000 * (end->tv_sec - start->tv_sec) + (end->tv_usec - start->tv_usec);
}

/**
 * Test whether the path exists and is a regular file.
 * 
 * @return 1 if the path exists and is a regular file, 0 otherwise.
 */
int
isfile (char *path)
{
    struct stat buf;
    if (stat(path, &buf) != 0)
        return 0;
    if (!S_ISREG(buf.st_mode))
        return 0;
    return 1;
}


int
isdir (char *path)
{
    struct stat buf;
    if (stat(path, &buf) != 0)
        return 0;
    if (!S_ISDIR(buf.st_mode))
        return 0;
    return 1;
}

int
islink (char *path)
{
    struct stat buf;
    if (stat(path, &buf) != 0)
        return 0;
    if (!S_ISLNK(buf.st_mode))
        return 0;
    return 1;
}

/**
 * Test whether the path exists.
 * 
 * @return 1 if the path exists, 0 otherwise.
 */
int
isexists (char *path)
{
    struct stat buf;
    if (stat(path, &buf) != 0)
        return 0;
    if (!S_ISDIR(buf.st_mode) && !S_ISREG(buf.st_mode) && !S_ISLNK(buf.st_mode))
        return 0;
    return 1;
}

long long
file_size(char *path)
{
	struct stat buf;
	if (stat(path, &buf) != 0) {
		return -1;
	}
	if (S_ISREG(buf.st_mode)) {
		return buf.st_size;
	}
	return -2;
}

int 
compare_int ( const void *a , const void *b ) 
{ 
    return *(int *)a - *(int *)b; 
} 

size_t
ffwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	size_t ret = fwrite(ptr, size, nmemb, stream);
	if (ret != nmemb) {
		DERROR("fwrite error, write:%d, must:%d\n", (int)ret, (int)nmemb);
		MEMLINK_EXIT;
	}
	return ret;
}

size_t
ffread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	size_t ret = fread(ptr, size, nmemb, stream);
	if (ret != nmemb) {
		DERROR("fread error, read:%d, must:%d\n", (int)ret, (int)nmemb);
		MEMLINK_EXIT;
	}
	return ret;
}

int
change_group_user(char *user)
{
    if (user == NULL || user[0] == 0) {
        DNOTE("not set user/group.\n");
        return 0;
    }
    
    struct passwd *pw;

    pw = getpwnam(user);
    if (pw == NULL) {
        DERROR("can't find user %s to switch.\n", user);
        return -1;
    }
    int ret;
    ret = setgid(pw->pw_gid);
    if (ret < 0) {
        DERROR("failed to set group: %d, %s\n", pw->pw_gid, strerror(errno));
        return -1;
    }
    ret = setuid(pw->pw_uid);
    if (ret < 0) {
        DERROR("failed to set user %s, %s\n", user, strerror(errno));
        return -1;
    }
    
    return 0;
}

int
check_thread_alive(pthread_t id)
{
    int err;

    err = pthread_kill(id, 0);
    if (err == ESRCH)
        return 0;

    else
        return 1;
}

int
wait_thread_exit(pthread_t id)
{
    int err;

    pthread_kill(id, SIGUSR1);
    
    err = pthread_kill(id, 0);

    while (err != ESRCH) {
        err = pthread_kill(id, 0);
    }
    return 1;
}

/**
 * @}
 */
