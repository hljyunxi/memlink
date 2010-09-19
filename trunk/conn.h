#ifndef MEMLINK_CONN_H
#define MEMLINK_CONN_H

#include <stdio.h>
#include <event.h>

#define CONN_MAX_READ_LEN   1024

typedef struct _conn
{
    int     sock;
    char    rbuf[CONN_MAX_READ_LEN];
    int     rlen;
    char    *wbuf;
    int     wlen;
    int     wpos;
    struct event revt;
    struct event wevt;
}Conn;

Conn*   conn_create(int svrfd);
void    conn_destroy(Conn *conn);



#endif
