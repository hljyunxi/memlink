#include <stdlib.h>
#include <network.h>
#include <string.h>
#include <errno.h>

#include "sthread.h"
#include "wthread.h"
#include "logfile.h"
#include "myconfig.h"
#include "zzmalloc.h"
#include "utils.h"
#include "common.h"

void*
sthread_loop(void *arg) 
{
    SThread *st = (SThread*) arg;
    DINFO("sthread_loop...\n");
    event_base_loop(st->base, 0);
    return NULL;
}

static void 
sthread_read(int fd, short event, void *arg) 
{
    SThread *st = (SThread*) arg;
    Conn *conn;

    DINFO("sthread_read...\n");
    conn = conn_create(fd);

    if (conn) {
        int ret;
        conn->port = g_cf->sync_port;
        conn->base = st->base;

        DINFO("new conn: %d\n", conn->sock);
        DINFO("change event to read.");
        ret = change_event(conn, EV_READ | EV_PERSIST, 1);
        if (ret < 0) {
            DERROR("change_event error: %d, close conn.\n", ret);
            conn_destroy(conn);
        }
    }
}

SThread*
sthread_create() 
{
    SThread *st;
    
    st = (SThread*)zz_malloc(sizeof(WThread));
    if (st == NULL) {
        DERROR("sthread malloc error.\n");
        MEMLINK_EXIT;
    }
    memset(st, 0, sizeof(SThread));

    st->sock = tcp_socket_server(g_cf->sync_port);
    if (st->sock == -1) 
        MEMLINK_EXIT;
    
    DINFO("sync thread socket creation ok!\n");

    st->base = event_base_new();

    event_set(&st->event, st->sock, EV_READ | EV_PERSIST, sthread_read, st);
    event_base_set(st->base, &st->event);
    event_add(&st->event, 0);

    g_runtime->sthread = st;

    pthread_t threadid;
    pthread_attr_t attr;
    int ret;

    ret = pthread_attr_init(&attr);
    if (ret != 0) {
        DERROR("pthread_attr_init error: %s\n", strerror(errno));
        MEMLINK_EXIT;
    }
    ret = pthread_create(&threadid, &attr, sthread_loop, st);
    if (ret != 0) {
        DERROR("pthread_create error: %s\n", strerror(errno));
        MEMLINK_EXIT;
    }
    DINFO("create sync thread ok\n");

    return st;
}

int
sdata_ready(Conn *conn, char *data, int datalen) 
{
    int ret = 0;
    char cmd;

    memcpy(&cmd, data + sizeof(short), sizeof(char));
    char buf[256] = {0};
    DINFO("data ready cmd: %d, data: %s\n", cmd, formath(data, datalen, buf, 256));
    switch (cmd) {
        case CMD_SYNC:

            break;
        case CMD_SYNC_DUMP:

            break;
        default:
            ret = MEMLINK_ERR_CLIENT_CMD;
            break;
    }
}

void sthread_destroy(SThread *st) 
{
    zz_free(st);
}
