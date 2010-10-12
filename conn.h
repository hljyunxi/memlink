#ifndef MEMLINK_CONN_H
#define MEMLINK_CONN_H

#include <stdio.h>
#include <event.h>
#include <sys/time.h>

#define CONN_MAX_READ_LEN   1024
#define CONN_READ   1
#define CONN_WRITE  2
#define CONN_SYNC   3

typedef struct _conn
{
    int     sock;                    // socket file descriptor
    char    rbuf[CONN_MAX_READ_LEN]; // read buffer
    int     rlen;                    // length of data which have been read
    char    *wbuf;                   // write buffer
    int     wlen;                    // write buffer length
    int     wpos;                    // write buffer position
    int     port;                    // server port

	struct event		evt;
    struct event_base   *base;

    struct timeval      ctime;
}Conn;

Conn*   conn_create(int svrfd);
void    conn_destroy(Conn *conn);



#endif
