#ifndef MEMLINK_WRTHREAD_H
#define MEMLINK_WRTHREAD_H

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

void    wrconn_destroy(Conn *conn);

typedef struct _thread
{
    int                 sock;
    struct event_base   *base;
    struct event        event;
}ThreadBase;

typedef struct _wrthread
{
    int                 sock;
    struct event_base   *base;
    struct event        event; // listen socket event
    struct event        dumpevt; // dump event
    volatile int        indump; // is dumping now
}WRThread;

WRThread*   wrthread_create();
void        wrthread_destroy(WRThread *wt);
void        wrthread_loop(WRThread *wt);

void        client_read(int fd, short event, void *arg);
void        client_write(int fd, short event, void *arg);
void        thread_read(int fd, short event, void *arg);

typedef     int (*func_data_ready)(void*, char*, int);
int         make_reply_data(char **wdata, short retcode, char *msg, char *replydata, int rlen);
int         client_buffer_read(int fd, char *data, int *dlen, func_data_ready func, void *conn);


#endif
