/**
 * 接受网络连接的主线程
 * @file server.c
 * @author zhaowei
 * @ingroup memlink
 * @{
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <event.h>
#include "logfile.h"
#include "myconfig.h"
#include "server.h"
#include "wthread.h"
#include "rthread.h"
#include "network.h"
#include "zzmalloc.h"
#include "utils.h"
#include "info.h"

MainServer*
mainserver_create()
{
    MainServer  *ms;

    ms = (MainServer*)zz_malloc(sizeof(MainServer));
    if (NULL == ms) {
        DERROR("malloc MainServer error!\n");
        MEMLINK_EXIT;
    }
    memset(ms, 0, sizeof(MainServer));
    
    int i, ret;
    for (i = 0; i < g_cf->thread_num; i++) {
        ret = thserver_init(&ms->threads[i]);
        if (ret < 0) {
            DERROR("thserver_create error! %d\n", ret);
            MEMLINK_EXIT;
        }

    }
    ms->sock = tcp_socket_server(g_cf->ip,g_cf->read_port);  
    if (ms->sock < 0) {
        DERROR("tcp_socket_server at port %d error: %d\n", g_cf->read_port, ms->sock);
        MEMLINK_EXIT;
    }
 
    ms->base = event_init(); 
    event_set(&ms->event, ms->sock, EV_READ|EV_PERSIST, mainserver_read, ms);
    event_add(&ms->event, 0);

    return ms;
}


void 
mainserver_destroy(MainServer *ms)
{
    zz_free(ms);
}

/**
 * mainserver_read - callback for incoming client read connection event.
 * @fd: listening socket file descriptor
 * @event:
 * @arg: main server
 *
 * Accept a client client conneciton, select a thread server and trigger an 
 * event for the select server.
 */
void
mainserver_read(int fd, short event, void *arg)
{
    MainServer  *ms = (MainServer*)arg;
    Conn        *conn;

    conn = conn_create(fd, sizeof(Conn));
    if (NULL == conn) {
        return;
    }
    conn->port  = g_cf->read_port;
    conn->ready = rdata_ready;

    if (conn_check_max(conn) != MEMLINK_OK) {
        DNOTE("too many read conn.\n");
        conn->destroy(conn);
        return;
    }

    int             n   = ms->lastth;
    ThreadServer    *ts = &ms->threads[n];
    ms->lastth = (ms->lastth + 1) % g_cf->thread_num;
    
    zz_check(ts->cq);

    queue_append(ts->cq, conn); 
    DINFO("send socket %d to notify ...\n", conn->sock);
    if (write(ts->notify_send_fd, "", 1) == -1) {
        char errbuf[1024];
        strerror_r(errno, errbuf, 1024);
        DERROR("Writing to thread %d notify pipe error: %s\n", n,  errbuf);
        //conn->destroy(conn);
    }
}

void
mainserver_loop(MainServer *ms)
{
    event_base_loop(ms->base, 0);
}

/**
  * Callback for enqueue event. Read date from the pipe and add event for 
  * incoming data in client connection.
  *
  * @param fd file descriptor for the pipe receive end.
  * @param event
  * @param arg thread server
  */
static void
thserver_notify(int fd, short event, void *arg)
{
    ThreadServer    *ts   = (ThreadServer*)arg;
    QueueItem       *itemhead = queue_get(ts->cq);
    QueueItem        *item = itemhead;
    Conn            *conn;
    int                ret;
    int             conn_limit = g_cf->max_read_conn;

    DINFO("thserver_notify: %d\n", fd);

    /*if (conn_limit <= 0) {
        conn_limit = g_cf->max_conn;
    }*/

    char buf;
    int  i;
    RwConnInfo *coninfo = ts->rw_conn_info;

    ret = read(ts->notify_recv_fd, &buf, 1);
    while (item) {
        conn = item->conn; 
        DINFO("notify fd: %d\n", conn->sock);
        ts->conns++;
        for (i = 0; i < conn_limit; i++) {
            coninfo = &(ts->rw_conn_info[i]);
            if (coninfo->fd == 0) {
                coninfo->fd = conn->sock;
                strcpy(coninfo->client_ip, conn->client_ip);
                coninfo->port = conn->client_port;
                memcpy(&coninfo->start, &conn->ctime, sizeof(struct timeval));
                //coninfo->conn_start = time(0);
                break;
            }
        }
        conn->thread = ts;
        conn->base   = ts->base;
        ret = change_event(conn, EV_READ|EV_PERSIST, g_cf->timeout, 1);
        if (ret < 0) {
            DERROR("change event error: %d, close conn\n", ret);
            conn->destroy(conn);
        }
        item = item->next;
    }

    if (itemhead) {
        /*QueueItem *qh = itemhead;
        while (qh) {
            DINFO("try free: %p\n", qh);
            qh = qh->next;
        }*/
        queue_free(ts->cq, itemhead); 
    }
}

static void*
thserver_run(void *arg)
{
    ThreadServer *ts = (ThreadServer*)arg;
    DINFO("thserver_run loop ...\n");
    event_base_loop(ts->base, 0);

    return NULL;
}

int
thserver_init(ThreadServer *ts)
{
    ts->base = event_base_new();

    int conn_limit = g_cf->max_read_conn > 0 ? g_cf->max_read_conn : g_cf->max_conn;
    ts->rw_conn_info = (RwConnInfo *)zz_malloc(sizeof(RwConnInfo) * conn_limit);
    if (ts->rw_conn_info == NULL) {
        DERROR("memlink malloc read connect info error.\n");
        MEMLINK_EXIT;
    }
    memset(ts->rw_conn_info, 0, sizeof(RwConnInfo) * conn_limit);

    ts->cq = queue_create();
    if (NULL == ts->cq) {
        DERROR("queue_create error!\n");
        MEMLINK_EXIT;
    }
    
    int fds[2];     
    
    if (pipe(fds) == -1) {
        char errbuf[1024];
        strerror_r(errno, errbuf, 1024);
        DERROR("create pipe error! %s\n",  errbuf);
        MEMLINK_EXIT;
    }
    
    ts->notify_recv_fd = fds[0];
    ts->notify_send_fd = fds[1];

    event_set(&ts->notify_event, ts->notify_recv_fd, EV_READ|EV_PERSIST, 
                thserver_notify, ts);
    event_base_set(ts->base, &ts->notify_event);
    event_add(&ts->notify_event, 0);

    pthread_attr_t  attr;
    int ret;

    ret = pthread_attr_init(&attr);
    if (ret != 0) {
        char errbuf[1024];
        strerror_r(errno, errbuf, 1024);
        DERROR("pthread_attr_init error: %s\n",  errbuf);
        MEMLINK_EXIT;
    }

    ret = pthread_create(&ts->threadid, &attr, thserver_run, ts);
    if (ret != 0) {
        char errbuf[1024];
        strerror_r(errno, errbuf, 1024);
        DERROR("pthread_create error: %s\n",  errbuf);
        MEMLINK_EXIT;
    }

    ret = pthread_detach(ts->threadid);
    if (ret != 0) {
        char errbuf[1024];
        strerror_r(errno, errbuf, 1024);
		DERROR("pthread_detach error: %s\n",  errbuf);
        MEMLINK_EXIT;
    }

    return 0;
}

/**
 * @}
 */
