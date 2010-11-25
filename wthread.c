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
#include <sys/time.h>
#include <netdb.h>
#include "wthread.h"
#include "serial.h"
#include "network.h"
#include "myconfig.h"
#include "logfile.h"
#include "dumpfile.h"
#include "rthread.h"
#include "sthread.h"
#include "zzmalloc.h"
#include "utils.h"
#include "common.h"

/**
 *
 * @param conn
 * @param newflag EV_READ or EV_WRITE
 * @param isnew 1 if the connection event has not been added before. 0 if not 
 * added.
 * @param timeout if positive, the event is added with a timeout.
 */
int
change_event(Conn *conn, int newflag, int timeout, int isnew)
{
    struct event      *event = &conn->evt;

    /* 
     * For old event, do nothing and return if the new flag is the same as the 
     * old flag.  Otherwise, unregistered the event.
     */
	if (isnew == 0) {
	    if (event->ev_flags == newflag)
			return 0;

		if (event_del(event) == -1) {
			DERROR("event del error.\n");
			return -1;
		}
	}

	if (newflag & EV_READ) {
		event_set(event, conn->sock, newflag, client_read, (void *)conn);
	}else if (newflag & EV_WRITE) {
		event_set(event, conn->sock, newflag, client_write, (void *)conn);
	}
    event_base_set(conn->base, event);
  
	if (timeout > 0) {
		struct timeval  tm;
		evutil_timerclear(&tm);
		tm.tv_sec = g_cf->timeout;
		if (event_add(event, &tm) == -1) {
			DERROR("event add error.\n");
			return -3;
		}
	}else{
		if (event_add(event, 0) == -1) {
			DERROR("event add error.\n");
			return -3;
		}
	}
    return 0;
}


/**
 * 回复数据
 *
 * Send respose to client.
 *
 * @param conn
 * @param retcode return code
 * @param return return data length
 * @param data return data
 *
 * ------------------------------------
 * | length (4B)| retcode (2B) | data |
 * ------------------------------------
 * Length is the count of bytes following it.
 */

int
data_set_reply(Conn *conn, short retcode, char *retdata, int retlen)
{
    //int mlen = 0;  // msg string len
    unsigned int datalen = 0;
    char *wdata;

    DINFO("retcode:%d, retlen:%d, retdata:%p\n", retcode, retlen, retdata);
	/*
    if (msg) {
        mlen = strlen(msg);
    }*/

    // package length + retcode + retdata
    datalen = CMD_REPLY_HEAD_LEN + retlen;
    DINFO("datalen: %d, retcode: %d, conn->wsize:%d\n", datalen, retcode, conn->wsize); 
    
    if (conn->wsize >= datalen) {
        wdata = conn->wbuf;
    }else{
        wdata = (char*)zz_malloc(datalen);
        if (NULL == wdata) {
            DERROR("datareply malloc error: %s\n", strerror(errno));
            MEMLINK_EXIT;
            return -1;
        }
        if (conn->wbuf) {
            zz_free(conn->wbuf);
        }
        conn->wbuf = wdata;
        conn->wsize = datalen;
    }
    conn->wlen = conn->wpos = 0;

    int count = 0; 

    unsigned int dlen = datalen - sizeof(int);
    memcpy(wdata, &dlen, sizeof(int));
    count += sizeof(int);

    memcpy(wdata + count, &retcode, sizeof(short));
    count += sizeof(short);
  
	/*
    unsigned char msglen = mlen;
    memcpy(wdata + count, &msglen, sizeof(char));
    count += sizeof(char);
    if (msglen > 0) {
        memcpy(wdata + count, msg, msglen);
        count += msglen;
    }*/

    DINFO("retlen: %d, retdata:%p, count:%d\n", retlen, retdata, count);
    if (retlen > 0) {
        memcpy(wdata + count, retdata, retlen);
        count += retlen;
    }

    /*
    if (conn->wbuf) {
        zz_free(conn->wbuf);
        conn->wlen = conn->wpos = 0;
    }
    conn->wbuf = wdata;
    */
    conn->wlen = datalen;
/*
#ifdef DEBUG
    char buf[10240] = {0};
    DINFO("reply %s\n", formath(conn->wbuf, conn->wlen, buf, 10240));
#endif
*/

	/*
	DINFO("change event to write.\n");
	int ret = change_event(conn, EV_WRITE|EV_PERSIST, g_cf->timeout, 0);
	if (ret < 0) {
		DERROR("change_event error: %d, close conn.\n", ret);
		conn->destroy(conn);
	}*/

    return 0;
}


int
data_reply(Conn *conn, short retcode, char *retdata, int retlen)
{
	
	int ret = data_set_reply(conn, retcode, retdata, retlen);
	if (ret < 0)
		return ret;

	DINFO("change event to write.\n");
	ret = change_event(conn, EV_WRITE|EV_PERSIST, g_cf->timeout, 0);
	if (ret < 0) {
		DERROR("change_event error: %d, close conn.\n", ret);
		conn->destroy(conn);
	}

    return 0;
}

void*
wdata_do_clean(void *args)
{
	HashNode	*node = (HashNode*)args;
	int			ret;

	pthread_mutex_lock(&g_runtime->mutex);
	//g_runtime->inclean = 1;
	//snprintf(g_runtime->cleankey, 512, "%s", node->key);
	
	DINFO("start clean ...\n");
	ret = hashtable_clean(g_runtime->ht, node->key);
	if (ret != 0) {
		DERROR("wdata_do_clean error: %d\n", ret);
	}
	//g_runtime->cleankey[0] = 0;
	//g_runtime->inclean = 0;
	pthread_mutex_unlock(&g_runtime->mutex);

	return NULL;
}

void
wdata_check_clean(char *key)
{
	HashNode	*node;
	
	/*if (g_runtime->inclean == 1) {
		return;
	}*/

	if (key	== NULL || key[0] == 0) 
		return;

	node = hashtable_find(g_runtime->ht, key);
	if (NULL == node)
		return;

	DINFO("check clean cond, used:%d, all:%d, blocks:%d\n", node->used, node->all, node->all / g_cf->block_data_count);
	// not do clean, when blocks is small than 3
	if (node->all / g_cf->block_data_count < 3) {
		return;
	}

	double rate = 1.0 - (double)node->used / node->all;
	DINFO("check clean rate: %f\n", rate);
	
	if (g_cf->block_clean_cond < 0.01 || g_cf->block_clean_cond > rate) {
		return;
	}

    pthread_t       threadid;
    pthread_attr_t  attr;
    int             ret;
    
    ret = pthread_attr_init(&attr);
    if (ret != 0) {
        DERROR("pthread_attr_init error: %s\n", strerror(errno));
        MEMLINK_EXIT;
    }

    ret = pthread_create(&threadid, &attr, wdata_do_clean, node);
    if (ret != 0) {
        DERROR("pthread_create error: %s\n", strerror(errno));
        MEMLINK_EXIT;
    }

	ret = pthread_detach(threadid);
	if (ret != 0) {
		DERROR("pthread_detach error; %s\n", strerror(errno));
		MEMLINK_EXIT;
	}
}

/**
 * Execute the write command.
 *
 * @param data command data
 * @param datalen command data length
 * @param writelog non-zero to write sync log; otherwise, not to write sync 
 * log.
 */
int 
wdata_apply(char *data, int datalen, int writelog)
{
    char key[512] = {0}; 
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
            ret = dumpfile(g_runtime->ht);
            break;
        case CMD_CLEAN:
            DINFO("<<< cmd CLEAN >>>\n");
            ret = cmd_clean_unpack(data, key);
			if (ret != 0) {
				DERROR("unpack clean error! ret: %d\n", ret);
				break;
			}
#ifdef DEBUG
            hashtable_print(g_runtime->ht, key);
            DINFO("clean unpack key: %s\n", key);
#endif
            ret = hashtable_clean(g_runtime->ht, key); 
#ifdef DEBUG
            DINFO("clean return:%d\n", ret); 
            hashtable_print(g_runtime->ht, key);
#endif
            break;
        case CMD_CREATE:
            DINFO("<<< cmd CREATE >>>\n");

            ret = cmd_create_unpack(data, key, &valuelen, &masknum, maskformat);
			if (ret != 0) {
				DERROR("unpack create error! ret: %d\n", ret);
				break;
			}

            DINFO("unpack key: %s, valuelen: %d, masknum: %d, maskarray: %d,%d,%d\n", key, valuelen, masknum, maskformat[0], maskformat[1], maskformat[2]);
			if (masknum > HASHTABLE_MASK_MAX_LEN) {
				DERROR("create mask too long: %d, max:%d\n", masknum, HASHTABLE_MASK_MAX_LEN);
				ret = MEMLINK_ERR_MASK;
			}else{
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
			}
            break;
        case CMD_DEL:
            DINFO("<<< cmd DEL >>>\n");
            ret = cmd_del_unpack(data, key, value, &valuelen);
			if (ret != 0) {
				DERROR("unpack del error! ret: %d\n", ret);
				break;
			}

            DINFO("unpack del, key: %s, value: %s, valuelen: %d\n", key, value, valuelen);
            ret = hashtable_del(g_runtime->ht, key, value);
            DINFO("hashtable_del: %d\n", ret);
#ifdef DEBUG
            hashtable_print(g_runtime->ht, key);
#endif
            if (ret >= 0 && writelog) {
                int sret = synclog_write(g_runtime->synclog, data, datalen);
                if (sret < 0) {
                    DERROR("synclog_write error: %d\n", sret);
                    MEMLINK_EXIT;
                }
            }

            break;
        case CMD_INSERT: {
            DINFO("<<< cmd INSERT >>>\n");
            ret = cmd_insert_unpack(data, key, value, &valuelen, &masknum, maskarray, &pos);
			if (ret != 0) {
				DERROR("unpack insert error! ret: %d\n", ret);
				break;
			}

            DINFO("unpack key:%s, value:%s, pos:%d, mask: %d, array:%d,%d,%d\n", key, value, pos, masknum, maskarray[0], maskarray[1], maskarray[2]);
            ret = hashtable_add_mask(g_runtime->ht, key, value, maskarray, masknum, pos);
            DINFO("hashtable_add_mask: %d\n", ret);
           
			/*
            int i;
            for (i = 0; i < masknum; i++) {
                DINFO("mask, i:%d, mask:%d\n", i, maskarray[i]);
            }*/
            //hashtable_print(g_runtime->ht, key);

            if (ret >= 0 && writelog) {
                int sret = synclog_write(g_runtime->synclog, data, datalen);
                if (sret < 0) {
                    DERROR("synclog_write error: %d\n", sret);
                    MEMLINK_EXIT;
                }
            }

            break;
        }
        case CMD_UPDATE:
            DINFO("<<< cmd UPDATE >>>\n");
            ret = cmd_update_unpack(data, key, value, &valuelen, &pos);
			if (ret != 0) {
				DERROR("unpack update error! ret: %d\n", ret);
				break;
			}

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
            ret = cmd_mask_unpack(data, key, value, &valuelen, &masknum, maskarray);
			if (ret != 0) {
				DERROR("unpack mask error! ret: %d\n", ret);
				break;
			}

            DINFO("unpack mask key: %s, valuelen: %d, masknum: %d, maskarray: %d,%d,%d\n", key, valuelen, 
                    masknum, maskarray[0], maskarray[1], maskarray[2]);
            ret = hashtable_mask(g_runtime->ht, key, value, maskarray, masknum);
            DINFO("hashtable_mask: %d\n", ret);
#ifdef DEBUG
            hashtable_print(g_runtime->ht, key);
#endif
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
			if (ret != 0) {
				DERROR("unpack tag error! ret: %d\n", ret);
				break;
			}

            DINFO("unpack tag, key:%s, value:%s, tag:%d\n", key, value, tag);
            ret = hashtable_tag(g_runtime->ht, key, value, tag);
            DINFO("hashtable_tag: %d\n", ret);

#ifdef DEBUG
            hashtable_print(g_runtime->ht, key);
#endif
            if (ret >= 0 && writelog) {
                int sret = synclog_write(g_runtime->synclog, data, datalen);
                if (sret < 0) {
                    DERROR("synclog_write error: %d\n", sret);
                    MEMLINK_EXIT;
                }
            }

            break;
        case CMD_RMKEY:
            DINFO("<<< cmd RMKEY >>>\n");
            ret = cmd_rmkey_unpack(data, key);
			if (ret != 0) {
				DERROR("unpack tag error! ret: %d\n", ret);
				break;
			}

            DINFO("unpack rmkey, key:%s\n", key);
            ret = hashtable_remove_key(g_runtime->ht, key);
            DINFO("hashtable_remove_key ret: %d\n", ret);
            if (ret >= 0 && writelog) {
                int sret = synclog_write(g_runtime->synclog, data, datalen);
                if (sret < 0) {
                    DERROR("synclog_write error: %d\n", sret);
                    MEMLINK_EXIT;
                }
            }
            break;
        default:
            ret = MEMLINK_ERR_CLIENT_CMD;
            break;
    }
	
	wdata_check_clean(key);

    return ret;
}

/**
 * Execute the write command and send response to client.
 *
 * @param conn
 * @param data command data
 * @param datalen the length of data parameter
 */
static int
wdata_ready(Conn *conn, char *data, int datalen)
{
    struct timeval start, end;

    gettimeofday(&start, NULL);
    pthread_mutex_lock(&g_runtime->mutex);
    int ret = wdata_apply(data, datalen, 1);
    pthread_mutex_unlock(&g_runtime->mutex);

    ret = data_reply(conn, ret, NULL, 0);
    gettimeofday(&end, NULL);
    DINFO("data_reply return: %d\n", ret);

    return 0;
}

/**
 * Callback for incoming client write connection.
 *
 * @param fd file descriptor for listening socket
 * @param event
 * @param arg thread base
 */
void
wthread_read(int fd, short event, void *arg)
{
    WThread *wt = (WThread*)arg;
    Conn    *conn;
    
    DINFO("wthread_read ...\n");
    conn = conn_create(fd, sizeof(Conn));
    if (conn) {
        int ret = 0;
        conn->port  = g_cf->write_port;
		conn->base  = wt->base;
		conn->ready = wdata_ready;

        DINFO("new conn: %d\n", conn->sock);
		DINFO("change event to read.\n");
		ret = change_event(conn, EV_READ|EV_PERSIST, g_cf->timeout, 1);
		if (ret < 0) {
			DERROR("change_event error: %d, close conn.\n", ret);
			conn->destroy(conn);
		}
    }
}

/**
 * Read client request, execute the command and send response. 
 *
 * @param fd
 * @param event
 * @param arg connection
 */
void
client_read(int fd, short event, void *arg)
{
    Conn *conn = (Conn*)arg;
    int     ret;
    unsigned short   datalen = 0;

	if (event & EV_TIMEOUT) {
		DWARNING("read timeout:%d, close\n", fd);
		conn->destroy(conn);
		return;
	}
    /*
     * Called more than one time for the same command and aready receive the 
     * 2-byte command length.
     */
    if (conn->rlen >= 2) {
        memcpy(&datalen, conn->rbuf, sizeof(short)); 
    }
    DINFO("client read datalen: %d, fd: %d, event:%x\n", datalen, fd, event);
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
                conn->destroy(conn);
                return;
            }else{
                DERROR("%d read EAGAIN, error %d: %s\n", fd, errno, strerror(errno));
            }
        }else if (ret == 0) {
            DINFO("read 0, close conn %d.\n", fd);
            conn->destroy(conn);
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
			conn->ready(conn, conn->rbuf, mlen);
            memmove(conn->rbuf, conn->rbuf + mlen, conn->rlen - mlen);
            conn->rlen -= mlen;
        }else{
            break;
        }
    }
}

/**
 * Send data inside the connection to client. If all the data has been sent, 
 * register read event for this connection.
 *
 * @param fd
 * @param event
 * @param arg connection
 */
void
client_write(int fd, short event, void *arg)
{
    Conn  *conn = (Conn*)arg;
	if (event & EV_TIMEOUT) {
		DWARNING("write timeout:%d, close\n", fd);
		conn->destroy(conn);
		return;
	}

    if (conn->wpos == conn->wlen) {
        conn->wlen = conn->wpos = 0;
		conn->wrote(conn);
        return;
    }
    conn_write(conn);
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

	ret = pthread_detach(threadid);
	if (ret != 0) {
		DERROR("pthread_detach error; %s\n", strerror(errno));
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
