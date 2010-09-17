#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <string.h>

#include "logfile.h"
#include "myconfig.h"
#include "server.h"
#include "wthread.h"
#include "network.h"
#include "zzmalloc.h"

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
    int thnum = g_cf->thread_num > MEMLINK_MAX_THREADS ? MEMLINK_MAX_THREADS : g_cf->thread_num;

    for (i = 0; i < thnum; i++) {
        ret = thserver_init(&ms->threads[i]);
        if (ret < 0) {
            DERROR("thserver_create error! %d\n", ret);
            MEMLINK_EXIT;
        }
    }


    
    ms->sock = tcp_socket_server(g_cf->read_port);  
    if (ms->sock < 0) {
        DERROR("tcp_socket_server at port %d error: %d\n", g_cf->read_port, ms->sock);
        MEMLINK_EXIT;
    }
 
    ms->base = event_init(); 
    event_set(&ms->event, ms->sock, EV_READ|EV_PERSIST, thread_read, ms);
    event_add(&ms->event, 0);

    return ms;
}


void 
mainserver_destroy(MainServer *ms)
{
}

void
mainserver_loop(MainServer *ms)
{
    event_base_loop(ms->base, 0);
}

static void
thserver_notify(int fd, short event, void *arg)
{
    ThreadServer    *ts   = (ThreadServer*)arg;
    QueueItem       *item = queue_get(ts->cq);
    Conn            *conn;

    DINFO("thserver_notify: %d\n", fd);
    while (item) {
        conn = item->conn; 

        event_set(&conn->revt, conn->sock, EV_READ|EV_PERSIST, client_read, conn);
        event_base_set(ts->base, &conn->revt);
        event_add(&conn->revt, 0); 
        
        item = item->next;
    }
    
    if (item) {
        queue_free(ts->cq, item); 
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

    ts->cq = queue_create();
    if (NULL == ts->cq) {
        DERROR("queue_create error!\n");
        MEMLINK_EXIT;
    }
    
    int fds[2];     
    
    if (pipe(fds) == -1) {
        DERROR("create pipe error! %s\n", strerror(errno));
        MEMLINK_EXIT;
    }
    
    ts->notify_recv_fd = fds[0];
    ts->notify_send_fd = fds[1];

    event_set(&ts->notify_event, ts->notify_recv_fd, EV_READ|EV_PERSIST, thserver_notify, ts);
    event_base_set(ts->base, &ts->notify_event);
    event_add(&ts->notify_event, 0);

    pthread_attr_t  attr;
    int ret;

    ret = pthread_attr_init(&attr);
    if (ret != 0) {
        DERROR("pthread_attr_init error: %s\n", strerror(errno));
        MEMLINK_EXIT;
    }

    ret = pthread_create(&ts->threadid, &attr, thserver_run, ts);
    if (ret != 0) {
        DERROR("pthread_create error: %s\n", strerror(errno));
        MEMLINK_EXIT;
    }


    return 0;
}


