#ifndef MEMLINK_CONN_H
#define MEMLINK_CONN_H

#include <stdio.h>
#include <event.h>

#define CONN_MAX_READ_LEN   1024
#define CONN_READ   1
#define CONN_WRITE  2
#define CONN_SYNC   3

typedef struct _conn
{
    int     sock;
    char    rbuf[CONN_MAX_READ_LEN];
    int     rlen;
    char    *wbuf;
    int     wlen;
    int     wpos;
    int     port; // server port

    //struct event revt;
    //struct event wevt;
	struct event		evt;
    struct event_base   *base;
}Conn;

Conn*   conn_create(int svrfd);
void    conn_destroy(Conn *conn);



#endif
