#ifndef MEMLINK_STHREAD_H
#define MEMLINK_STHREAD_H

#include "conn.h"

typedef struct _sthread 
{
    int sock;
    struct event_base *base;
    struct event event;
} SThread;

SThread* sthread_create();
void sthread_destroy(SThread *st);
int  sdata_ready(Conn *conn, char *data, int datalen);

#endif
