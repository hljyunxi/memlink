#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

#include "logfile.h"
#include "zzmalloc.h"
#include "sslave.h"

/**
 *
 */
static int 
sslave_connect() 
{
    int ret;
    int sock;

    DINFO("memlink sync slave connecting...\n");    

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        DERROR("socket creation error: %s\n", strerror(errno));
        return -1;
    }

    struct linger ling = {0, 0};
    ret = setsockopt(sock, SOL_SOCKET, SO_LINGER, (void *)&ling, sizeof(ling));
    if (ret != 0)
        DERROR("setsockopt LINGER error: %s\n", strerror(errno));

    int flags = 1;
    ret = setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (void*)&flags, sizeof(flags));
    if (ret != 0)
        DERROR("setsockopt NODELAY error: %s\n", strerror(errno));

    ret = setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (void *)&flags, sizeof(flags));
    if (ret != 0) {
        DERROR("setsockopt KEEPALIVE error: %s\n", strerror(errno));
        return -1;
    }

    struct sockaddr_in sin;
    sin.sin_port = htons((short)g_cf->sync_port);
    sin.sin_addr.s_addr = inet_addr(g_cf->master_addr);

    ret = connect(sock, (struct sockaddr*)&sin, sizeof(sin));
    if (ret != 0) {
        DERROR("socket connect error: %s\n", strerror(errno));
        close(sock);
        return -1;
    }

    DINFO("memlink sync slave connected\n");
    return sock;
}

static void*
sslave_loop(void *arg) 
{
    SSlave *ss = (SSlave*) arg;
    DINFO("sync slave looping...\n");
    event_base_loop(ss->base, 0);

    return NULL;
}


// TODO
// code to process the sync log data from sync master

SSlave*
sslave_create() 
{
    SSlave *ss;

    ss = (SSlave*)zz_malloc(sizeof(SSlave));
    if (ss == NULL) {
        DERROR("sslave malloc error.\n");
        MEMLINK_EXIT;
    }
    memset(ss, 0, sizeof(SSlave));

    ss->sock = sslave_connect();
    if (ss->sock < 0) 
        MEMLINK_EXIT;

    ss->base = event_base_new();

    event_set(&ss->event, ss->sock, EV_READ | EV_PERSIST, client_read, ss);
    event_base_set(ss->base, &ss->event);
    event_add(&ss->event, 0);

    // TODO timeout event
    pthread_t threadid;
    pthread_attr_t attr;
    int ret;

    ret = pthread_attr_init(&attr);
    if (ret != 0) {
        DERROR("pthread_attr_init error: %s\n", strerror(errno));
        MEMLINK_EXIT;
    }

    ret = pthread_create(&threadid, &attr, sslave_loop, ss);
    if (ret != 0) {
        DERROR("pthread_create error: %s\n", strerror(errno));
        MEMLINK_EXIT;
    }

    DINFO("create sync slave thread ok!\n");

    return ss;
}

void sslave_destroy(SSlave *ss) 
{
    zz_free(ss);
}
