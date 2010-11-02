#ifndef MEMLINK_WTHREAD_H
#define MEMLINK_WTHREAD_H

#include <stdio.h>
#include "conn.h"

typedef struct _wthread
{
    int                 sock;
    struct event_base   *base;
    struct event        event; // listen socket event
    struct event        dumpevt; // dump event
    volatile int        indump; // is dumping now
}WThread;

WThread*    wthread_create();
void        wthread_destroy(WThread *wt);
void*       wthread_loop(void *arg);

void        client_read(int fd, short event, void *arg);
void        client_write(int fd, short event, void *arg);
int         data_reply(Conn *conn, short retcode, char *retdata, int retlen);
int         wdata_apply(char *data, int datalen, int writelog);
int			change_event(Conn *conn, int newflag, int isnew);

#endif
