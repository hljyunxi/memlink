#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#ifdef __linux
#include <linux/if.h>
#endif
#include <sys/un.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include "wthread.h"
#include "serial.h"
#include "network.h"
#include "myconfig.h"
#include "logfile.h"
#include "dumpfile.h"
#include "zzmalloc.h"

/*
static int
change_event(Conn *conn, int newflag)
{
    struct event      *event = &conn->revt;
    struct event_base *base  = event->ev_base;

    if (event->ev_flags == newflag)
        return 0;

    if (event_del(event) == -1) {
        DERROR("event del error.\n");
        return -1;
    }
    event_set(event, conn->sock, newflag, client_read, (void *)conn);
    event_base_set(base, event);
    //c->ev_flags = new_flags;
    if (event_add(event, 0) == -1) {
        DERROR("event add error.\n");
        return -2;
    }
    return 0;
}

int
make_reply_data(char **retdata, short retcode, char *msg, char *replydata, int rlen)
{
    int mlen = 0;
    unsigned short datalen = 0;
    char *wdata;

    if (msg) {
        mlen = strlen(msg);
    }

    datalen = sizeof(short) + sizeof(short) + sizeof(char) + mlen + rlen;

    wdata = (char*)malloc(datalen);
    if (NULL == wdata) {
        DERROR("datareply malloc error: %s\n", strerror(errno));
        MEMLINK_EXIT;
        return -1;
    }
    int count = 0; 

    unsigned short dlen = datalen - 2;
    memcpy(wdata, &dlen, sizeof(short));
    count += sizeof(short);
    memcpy(wdata + count, &retcode, sizeof(short));
    count += sizeof(short);
   
    unsigned char msglen = mlen;
    memcpy(wdata + count, &msglen, sizeof(char));
    count += sizeof(char);
    if (msglen > 0) {
        memcpy(wdata + count, msg, msglen);
        count += msglen;
    }
    memcpy(wdata + count, replydata, rlen);
    count += rlen;

    *retdata = wdata;

    return datalen;
}*/

/**
 * 回复数据
 */
int
data_reply(Conn *conn, short retcode, char *msg, char *retdata, int retlen)
{
    //int datalen = 0; 
    //char *wdata;
   

    int mlen = 0;  // msg string len
    unsigned short datalen = 0;
    char *wdata;

    if (msg) {
        mlen = strlen(msg);
    }

    // package length + retcode + msg len + msg + retdata
    datalen = sizeof(short) + sizeof(short) + sizeof(char) + mlen + retlen;
    DINFO("datalen: %d\n", datalen); 
    wdata = (char*)malloc(datalen);
    if (NULL == wdata) {
        DERROR("datareply malloc error: %s\n", strerror(errno));
        MEMLINK_EXIT;
        return -1;
    }
    int count = 0; 

    unsigned short dlen = datalen - sizeof(short);
    memcpy(wdata, &dlen, sizeof(short));
    count += sizeof(short);
    memcpy(wdata + count, &retcode, sizeof(short));
    count += sizeof(short);
   
    unsigned char msglen = mlen;
    memcpy(wdata + count, &msglen, sizeof(char));
    count += sizeof(char);
    if (msglen > 0) {
        memcpy(wdata + count, msg, msglen);
        count += msglen;
    }
    if (retlen > 0) {
        memcpy(wdata + count, retdata, retlen);
        count += retlen;
    }

    //*retdata = wdata;

    /*
    datalen = make_reply_data(&wdata, retcode, msg, retdata, retlen);
    if (datalen <= 0) {
        DERROR("make_reply_data error!\n");
        return -1;
    }*/

    if (conn->wbuf) {
        zz_free(conn->wbuf);
        conn->wlen = conn->wpos = 0;
    }
    conn->wbuf = wdata;
    conn->wlen = datalen;
   
    event_set(&conn->wevt, conn->sock, EV_WRITE|EV_PERSIST, client_write, conn);
    event_base_set(conn->revt.ev_base, &conn->wevt);

    event_add(&conn->wevt, 0);

    return 0;
}

static int
wdata_ready(Conn *conn, char *data, int datalen)
{
    char key[1024]; 
    char value[1024];
    char maskstr[128];
    char cmd;
    int  ret = 0;
    unsigned char   valuelen;
    unsigned char   masknum;
    char            maskformat[HASHTABLE_MASK_MAX_LEN];
    unsigned int    maskarray[HASHTABLE_MASK_MAX_LEN];
    unsigned int    pos;
    unsigned char   tag;
    int             vnum;

    memcpy(&cmd, data + sizeof(short), sizeof(char));
    DINFO("data ready cmd: %d\n", cmd);

    pthread_mutex_lock(&g_runtime->mutex);
    switch(cmd) {
        case CMD_DUMP:
            pthread_mutex_unlock(&g_runtime->mutex);
            dumpfile_start(conn->sock, 0, conn);
            break;
        case CMD_CLEAN:
            cmd_clean_unpack(data, key);
            ret = hashtable_clean(g_runtime->ht, key); 
            break;
        case CMD_CREATE:
            DINFO("unpack create ...\n");
            cmd_create_unpack(data, key, &valuelen, &masknum, maskformat);
            DINFO("unpack valuelen: %d, masknum: %d\n", valuelen, masknum);
            vnum = valuelen;
            ret = hashtable_add_info(g_runtime->ht, key, vnum, maskstr);
            DINFO("hashtabl_add_info return: %d\n", ret);
            break;
        case CMD_DEL:
            cmd_del_unpack(data, key, value, &valuelen);
            ret = hashtable_del(g_runtime->ht, key, value);
            break;
        case CMD_INSERT:
            cmd_insert_unpack(data, key, value, &valuelen, &masknum, maskarray, &pos);
            ret = hashtable_add_mask(g_runtime->ht, key, value, maskarray, masknum, pos);
            break;
        case CMD_UPDATE:
            cmd_update_unpack(data, key, value, &valuelen, &pos);
            ret = hashtable_update(g_runtime->ht, key, value, pos);
            break;
        case CMD_MASK:
            cmd_mask_unpack(data, key, value, &valuelen, &masknum, maskarray);
            ret = hashtable_mask(g_runtime->ht, key, value, maskstr);
            break;
        case CMD_TAG:
            cmd_tag_unpack(data, key, value, &valuelen, &tag);
            ret = hashtable_tag(g_runtime->ht, key, value, tag);
            break;
        default:
            ret = 300;
            goto wdata_ready_over;
    }
   
    if (ret < 0) {
        ret = 500;
    }else{
        ret = 200;
    }
wdata_ready_over:
    pthread_mutex_unlock(&g_runtime->mutex);

    ret = data_reply(conn, ret, NULL, NULL, 0);
    DINFO("data_reply return: %d\n", ret);

    return 0;
}

void
wthread_read(int fd, short event, void *arg)
{
    ThreadBase *tb = (ThreadBase*)arg;
    Conn    *conn;

    conn = conn_create(fd);
    if (conn) {
        struct timeval tm;

        evutil_timerclear(&tm);
        tm.tv_sec = g_cf->timeout;

        event_set(&conn->revt, conn->sock, EV_READ|EV_PERSIST, client_read, conn);
        event_base_set(tb->base, &conn->revt);

        event_add(&conn->revt, &tm);
    }
}

/*
int
client_buffer_read(int fd, char *data, int *dlen, func_data_ready func, void *conn)
{
    int     ret;
    unsigned short   datalen = 0;

    if (*dlen >= 2) {
        memcpy(&datalen, data, sizeof(short));
    }

    while (1) {
        int rlen = datalen;
        if (rlen == 0) {
            rlen = CONN_MAX_READ_LEN - *dlen;
        }
        ret = read(fd, &data[*dlen], rlen);
        if (ret == -1) {
            if (errno == EINTR) {
                continue;
            }else if (errno != EAGAIN) {
                // maybe close conn?
                DERROR("%d read error: %s\n", fd, strerror(errno));
                return -1;
            }
        }else{
            *dlen += ret;
        }

        break;
    }

    while (*dlen >= 2) {
        memcpy(&datalen, data, sizeof(short));

        if (*dlen >= datalen + sizeof(short)) {
            func(conn, data, datalen);
        
            int mlen = datalen + sizeof(short);
            memmove(data, data + mlen, *dlen - mlen);
            *dlen -= mlen;
        }
    }
    
    return *dlen;
}
*/

void
client_read(int fd, short event, void *arg)
{
    /*
    Conn *conn = (Conn*)arg;
    int rlen;

    rlen = client_buffer_read(fd, conn->rbuf, &conn->rlen, (func_data_ready)data_ready, conn);
    if (rlen < 0) {
        DERROR("client_buffer_read error! %d\n", rlen);
        return;
    }
    */

    Conn *conn = (Conn*)arg;
    int     ret;
    unsigned short   datalen = 0;

    if (conn->rlen >= 2) {
        memcpy(&datalen, conn->rbuf, sizeof(short));
    }

    while (1) {
        int rlen = datalen;
        if (rlen == 0) {
            rlen = CONN_MAX_READ_LEN - conn->rlen;
        }
        ret = read(fd, &conn->rbuf[conn->rlen], rlen);
        if (ret == -1) {
            if (errno == EINTR) {
                continue;
            }else if (errno != EAGAIN) {
                // maybe close conn?
                DERROR("%d read error: %s\n", fd, strerror(errno));
                conn_destroy(conn);
                return;
            }
        }else if (ret == 0) {
            DINFO("read 0, close conn.\n");
            conn_destroy(conn);
            return;
        }else{
            conn->rlen += ret;
        }

        break;
    }

    DINFO("conn rbuf len: %d\n", conn->rlen);
    while (conn->rlen >= 2) {
        memcpy(&datalen, conn->rbuf, sizeof(short));

        if (conn->rlen >= datalen + sizeof(short)) {
            wdata_ready(conn, conn->rbuf, datalen);
        
            int mlen = datalen + sizeof(short);
            memmove(conn->rbuf, conn->rbuf + mlen, conn->rlen - mlen);
            conn->rlen -= mlen;
        }
    }
}

void
client_write(int fd, short event, void *arg)
{
    Conn  *conn = (Conn*)arg;
    
    if (conn->wpos == conn->wlen) {
        /*if (change_event(conn, EV_READ|EV_PERSIST) < 0) {
            DERROR("change event error! close socket\n");
            wrconn_destroy(conn);
        }*/
        DINFO("%d write complete!\n", fd);
        event_del(&conn->wevt);
        conn->wlen = conn->wpos = 0;
        return;
    }
    
    int ret;

    while (1) {
        ret = write(fd, &conn->wbuf[conn->wpos], conn->wlen - conn->wpos);
        DINFO("write: %d\n", ret);
        if (ret == -1) {
            if (errno == EINTR) {
                continue;
            }else if (errno != EAGAIN) {
                // maybe close conn?
                DERROR("write error! %s\n", strerror(errno)); 
                conn_destroy(conn);
                break;
            }
        }else{
            conn->wpos += ret;
        }
        break;
    } 
    
}


WThread*
wthread_create()
{
    WThread *wt;

    wt = (WThread*)zz_malloc(sizeof(WThread));
    if (NULL == wt) {
        DERROR("wthread malloc error.\n");
        MEMLINK_EXIT;
    }
    memset(wt, 0, sizeof(WThread));

    wt->sock = tcp_socket_server(g_cf->write_port); 
    if (wt->sock == -1) {
        MEMLINK_EXIT;
    }
    
    DINFO("write socket create ok\n");

    wt->base = event_base_new();
    
    event_set(&wt->event, wt->sock, EV_READ | EV_PERSIST, wthread_read, wt);
    event_base_set(wt->base, &wt->event);

    struct timeval tm;
    evtimer_set(&wt->dumpevt, dumpfile_start, &wt->dumpevt);
    evutil_timerclear(&tm);
    tm.tv_sec = g_cf->dump_interval * 60; 
    event_base_set(wt->base, &wt->dumpevt);
    event_add(&wt->dumpevt, &tm);

    g_runtime->wthread = wt;

    pthread_t       threadid;
    pthread_attr_t  attr;
    int             ret;
    
    ret = pthread_attr_init(&attr);
    if (ret != 0) {
        DERROR("pthread_attr_init error: %s\n", strerror(errno));
        MEMLINK_EXIT;
    }

    ret = pthread_create(&threadid, &attr, wthread_loop, wt);
    if (ret != 0) {
        DERROR("pthread_create error: %s\n", strerror(errno));
        MEMLINK_EXIT;
    }

    DINFO("create WThread ok!\n");

    return wt;
}

void*
wthread_loop(void *arg)
{
    WThread *wt = (WThread*)arg;
    DINFO("wthread_loop ...\n");
    event_base_loop(wt->base, 0);

    return NULL;
}

void
wthread_destroy(WThread *wt)
{
    zz_free(wt);
}


