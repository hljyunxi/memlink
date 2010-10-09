#include <stdlib.h>
#include <event.h>
#include <network.h>
#include <string.h>
#include <errno.h>

#include "sthread.h"
#include "wthread.h"
#include "logfile.h"
#include "myconfig.h"
#include "zzmalloc.h"

void*
sthread_loop(void *arg) 
{
    SThread *st = (SThread*) arg;
    DINFO("sthread_loop...\n");
    event_base_loop(st->base, 0);

    return NULL;
}

static void 
sthread_read(int fd, short event, void *arg) {
    //SThread *st = (SThread*) arg;
    Conn *conn;

    DINFO("sthread_read...\n");
    conn = conn_create(fd);

    if (conn) {
        //int ret = 0;
        conn->port = g_cf->sync_port;
        DINFO("new conn: %d\n", conn->sock);
	
		/*
        event_set(&conn->revt, conn->sock, EV_READ | EV_PERSIST, client_read, conn);
        ret = event_base_set(st->base, &conn->revt);
        DINFO("event_base_set: %d\n", ret);

        ret = event_add(&conn->revt, 0);
        DINFO("event_add: %d\n", ret);
		*/
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

    // TODO Timeout event
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

void sthread_destroy(SThread *st) 
{
    zz_free(st);
}
