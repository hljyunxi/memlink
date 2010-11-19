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
 * Connection for sending sync log.
 */
typedef struct _syncConn 
{
    int sock; // socket file descriptor

    int log_fd; // sync log file descriptor
    char log_name[PATH_MAX];
    unsigned int log_ver; // sync log version
    unsigned int log_index_pos; // sync log index position

    struct event evt;
    struct event_base *base;
} SyncLogConn;

/**
 * Connection for sending dump file.
 */
typedef struct _dumpConn
{
    int sock;
    int dump_fd;

    struct event evt;
    struct event_base *base;
} DumpConn;

SThread* sthread_create();
void sthread_destroy(SThread *st);
int  sdata_ready(Conn *conn, char *data, int datalen);

SyncLogConn* synclog_conn_create();
void synclog_conn_destroy(SyncLogConn *conn);
DumpConn* dump_conn_create();
void dump_conn_destroy(DumpConn *conn);

#endif
