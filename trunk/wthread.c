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
#include "rthread.h"
#include "zzmalloc.h"
#include "utils.h"

int
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
    
    if (event_add(event, 0) == -1) {
        DERROR("event add error.\n");
        return -3;
    }
    return 0;
}

/**
 * 回复数据
 */
int
data_reply(Conn *conn, short retcode, char *msg, char *retdata, int retlen)
{
    int mlen = 0;  // msg string len
    unsigned short datalen = 0;
    char *wdata;

    if (msg) {
        mlen = strlen(msg);
    }

    // package length + retcode + msg len + msg + retdata
    datalen = sizeof(short) + sizeof(short) + sizeof(char) + mlen + retlen;
    DINFO("datalen: %d, retcode: %d\n", datalen, retcode); 
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

int 
wdata_apply(char *data, int datalen, int writelog)
{
    char key[1024]; 
    char value[1024];
    //char maskstr[128];
    char cmd;
    int  ret = 0;
    unsigned char   valuelen;
    unsigned char   masknum;
    unsigned int    maskformat[HASHTABLE_MASK_MAX_LEN];
    unsigned int    maskarray[HASHTABLE_MASK_MAX_LEN];
    unsigned int    pos;
    unsigned char   tag;
    int             vnum;

    memcpy(&cmd, data + sizeof(short), sizeof(char));
    char buf[256] = {0};
    DINFO("data ready cmd: %d, data: %s\n", cmd, formath(data, datalen, buf, 256));

    //pthread_mutex_lock(&g_runtime->mutex);
    switch(cmd) {
        case CMD_DUMP:
            DINFO("<<< cmd DUMP >>>\n");
            //pthread_mutex_unlock(&g_runtime->mutex);
            //ret = dumpfile_call();
            ret = dumpfile(g_runtime->ht);
            break;
        case CMD_CLEAN:
            DINFO("<<< cmd CLEAN >>>\n");
            cmd_clean_unpack(data, key);
            DINFO("clean unpack key: %s\n", key);
            ret = hashtable_clean(g_runtime->ht, key); 
            break;
        case CMD_CREATE:
            DINFO("<<< cmd CREATE >>>\n");

            cmd_create_unpack(data, key, &valuelen, &masknum, maskformat);
            DINFO("unpack key: %s, valuelen: %d, masknum: %d, maskarray: %d,%d,%d\n", key, valuelen, masknum, maskformat[0], maskformat[1], maskformat[2]);
            vnum = valuelen;
            ret = hashtable_add_info_mask(g_runtime->ht, key, vnum, maskformat, masknum);
            DINFO("hashtabl_add_info return: %d\n", ret);
            if (ret >= 0 && writelog) {
                int sret = synclog_write(g_runtime->synclog, data, datalen);
                if (sret < 0) {
                    DERROR("synclog_write error: %d\n", sret);
                    MEMLINK_EXIT;
                }
            }
            break;
        case CMD_DEL:
            DINFO("<<< cmd DEL >>>\n");
            cmd_del_unpack(data, key, value, &valuelen);
            DINFO("unpack del, key: %s, value: %s, valuelen: %d\n", key, value, valuelen);
            ret = hashtable_del(g_runtime->ht, key, value);
            DINFO("hashtable_del: %d\n", ret);
            if (ret >= 0 && writelog) {
                int sret = synclog_write(g_runtime->synclog, data, datalen);
                if (sret < 0) {
                    DERROR("synclog_write error: %d\n", sret);
                    MEMLINK_EXIT;
                }
            }

            break;
        case CMD_INSERT:
            DINFO("<<< cmd INSERT >>>\n");
            cmd_insert_unpack(data, key, value, &valuelen, &masknum, maskarray, &pos);
            DINFO("unpak mask: %d, array:%d,%d,%d\n", masknum, maskarray[0], maskarray[1], maskarray[2]);
            
            ret = hashtable_add_mask(g_runtime->ht, key, value, maskarray, masknum, pos);
            DINFO("hashtable_add_mask: %d\n", ret);

            hashtable_print(g_runtime->ht, key);
            if (ret >= 0 && writelog) {
                int sret = synclog_write(g_runtime->synclog, data, datalen);
                if (sret < 0) {
                    DERROR("synclog_write error: %d\n", sret);
                    MEMLINK_EXIT;
                }
            }

            break;
        case CMD_UPDATE:
            DINFO("<<< cmd UPDATE >>>\n");
            cmd_update_unpack(data, key, value, &valuelen, &pos);
            DINFO("unpack update, key:%s, value:%s, pos:%d\n", key, value, pos);
            ret = hashtable_update(g_runtime->ht, key, value, pos);
            DINFO("hashtable_update: %d\n", ret);
            hashtable_print(g_runtime->ht, key);
            if (ret >= 0 && writelog) {
                int sret = synclog_write(g_runtime->synclog, data, datalen);
                if (sret < 0) {
                    DERROR("synclog_write error: %d\n", sret);
                    MEMLINK_EXIT;
                }
            }

            break;
        case CMD_MASK:
            DINFO("<<< cmd MASK >>>\n");
            cmd_mask_unpack(data, key, value, &valuelen, &masknum, maskarray);
            DINFO("unpack mask key: %s, valuelen: %d, masknum: %d, maskarray: %d,%d,%d\n", key, valuelen, 
                    masknum, maskarray[0], maskarray[1], maskarray[2]);
            ret = hashtable_mask(g_runtime->ht, key, value, maskarray, masknum);
            DINFO("hashtable_update: %d\n", ret);
            hashtable_print(g_runtime->ht, key);
            if (ret >= 0 && writelog) {
                int sret = synclog_write(g_runtime->synclog, data, datalen);
                if (sret < 0) {
                    DERROR("synclog_write error: %d\n", sret);
                    MEMLINK_EXIT;
                }
            }

            break;
        case CMD_TAG:
            DINFO("<<< cmd TAG >>>\n");
            cmd_tag_unpack(data, key, value, &valuelen, &tag);
            DINFO("unpack update, key:%s, value:%s, tag:%d\n", key, value, tag);
            ret = hashtable_tag(g_runtime->ht, key, value, tag);
            DINFO("hashtable_update: %d\n", ret);
            hashtable_print(g_runtime->ht, key);
            if (ret >= 0 && writelog) {
                int sret = synclog_write(g_runtime->synclog, data, datalen);
                if (sret < 0) {
                    DERROR("synclog_write error: %d\n", sret);
                    MEMLINK_EXIT;
                }
            }

            break;
        default:
            ret = 300;
            break;
    }

    return ret;
}

static int
wdata_ready(Conn *conn, char *data, int datalen)
{
    pthread_mutex_lock(&g_runtime->mutex);
    int ret = wdata_apply(data, datalen, 1);
    pthread_mutex_unlock(&g_runtime->mutex);

    if (ret < 0) {
        ret = 500;
    }else  if (ret == 0){
        ret = 200;
    }

    ret = data_reply(conn, ret, NULL, NULL, 0);
    DINFO("data_reply return: %d\n", ret);

    return 0;
}

/**
 * wthread_read - callback for incoming client write connection
 * @fd: file descriptor for listening socket
 * @event:
 * @arg: thread base
 */
void
wthread_read(int fd, short event, void *arg)
{
    WThread *wt = (WThread*)arg;
    Conn    *conn;
    
    DINFO("wthread_read ...\n");
    conn = conn_create(fd);
    if (conn) {
        int ret = 0;
        conn->port = g_cf->write_port;
        //struct timeval tm;
        DINFO("new conn: %d\n", conn->sock);

        //evutil_timerclear(&tm);
        //tm.tv_sec = g_cf->timeout;

        event_set(&conn->revt, conn->sock, EV_READ|EV_PERSIST, client_read, conn);
        ret = event_base_set(wt->base, &conn->revt);
        DINFO("event_base_set: %d\n", ret);

        //event_add(&conn->revt, &tm);
        ret = event_add(&conn->revt, 0);
        DINFO("event_add: %d\n", ret);
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
    Conn *conn = (Conn*)arg;
    int     ret;
    unsigned short   datalen = 0;

    /*
     * Called more than one time for the same command and aready receive the 
     * 2-byte command length.
     */
    if (conn->rlen >= 2) {
        memcpy(&datalen, conn->rbuf, sizeof(short)); 
    }
    DINFO("client read datalen: %d\n", datalen);
    DINFO("conn rlen: %d\n", conn->rlen);

    while (1) {
        int rlen = datalen;
        // If command length is unavailable, use max length.
        if (rlen == 0) {
            rlen = CONN_MAX_READ_LEN - conn->rlen;
        }
        DINFO("try read len: %d\n", rlen);
        ret = read(fd, &conn->rbuf[conn->rlen], rlen);
        DINFO("read return: %d\n", ret);
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
            DINFO("2 conn rlen: %d\n", conn->rlen);
        }

        break;
    }

    DINFO("conn rbuf len: %d\n", conn->rlen);
    while (conn->rlen >= 2) {
        memcpy(&datalen, conn->rbuf, sizeof(short));
        DINFO("check datalen: %d, rlen: %d\n", datalen, conn->rlen);
        int mlen = datalen + sizeof(short);

        if (conn->rlen >= mlen) {
            if (conn->port == g_cf->write_port) {
                wdata_ready(conn, conn->rbuf, mlen);
            }else if (conn->port == g_cf->read_port) {
                rdata_ready(conn, conn->rbuf, mlen);
            }else{
                DERROR("conn port error: %d\n", conn->port);
                conn_destroy(conn);
                return;
            }
        
            memmove(conn->rbuf, conn->rbuf + mlen, conn->rlen - mlen);
            conn->rlen -= mlen;
        }else{
            break;
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
    DINFO("client write: %d\n", conn->wlen); 
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
    event_add(&wt->event, 0);

    struct timeval tm;
    evtimer_set(&wt->dumpevt, dumpfile_call_loop, &wt->dumpevt);
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


