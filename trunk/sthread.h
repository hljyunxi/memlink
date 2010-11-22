#ifndef MEMLINK_STHREAD_H
#define MEMLINK_STHREAD_H

#include "conn.h"
#include <limits.h>

typedef struct _sthread 
{
    int sock;
    struct event_base *base;
    struct event event;
} SThread;

/**
 * Connection for sync.
 */
typedef struct _syncConn 
{
    CONN_MEMBER

    struct timeval interval;

    int sync_fd;                // file descriptor for sync log or dump
    char log_name[PATH_MAX];    // sync log path
    unsigned int log_ver;       // sync log version
    unsigned int log_index_pos; // sync log index position

} SyncConn;

SThread* sthread_create();
void sthread_destroy(SThread *st);

int  sdata_ready(Conn *conn, char *data, int datalen);
void sync_conn_destroy(Conn *conn);
#endif
