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


#define NOT_SEND 0
#define SEND_LOG 1 
#define SEND_DUMP 2

/**
 * Connection for sync.
 */
typedef struct _syncConn 
{
    CONN_MEMBER

    struct event sync_write_evt;
    struct event sync_interval_evt;
    struct event sync_read_evt;
    struct timeval interval;

    int status;                 
    int sync_fd;                // file descriptor for sync log or dump
    char log_name[PATH_MAX];    // sync log path
    unsigned int log_ver;       // sync log version
    unsigned int log_index_pos; // sync log index position
    void (*fill_wbuf)(int fd, short event, void *arg);
} SyncConn;

SThread* sthread_create();
void sthread_destroy(SThread *st);

int  sdata_ready(Conn *conn, char *data, int datalen);
void sync_conn_destroy(Conn *conn);
#endif
