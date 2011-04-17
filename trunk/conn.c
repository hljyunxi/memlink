/**
 * 连接处理
 * @file conn.c
 * @author zhaowei
 * @ingroup memlink
 * @{
 */
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#ifdef __linux
#include <linux/if.h>
#endif
#include <sys/un.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include "conn.h"
#include "logfile.h"
#include "zzmalloc.h"
#include "serial.h"
#include "network.h"
#include "utils.h"

/**
 * conn_create - return the accepted connection.
 */
Conn*   
conn_create(int svrfd, int connsize)
{
    Conn    *conn;
    int     newfd;
    struct sockaddr_in  clientaddr;
    socklen_t           slen = sizeof(clientaddr);

    while (1) {
        newfd = accept(svrfd, (struct sockaddr*)&clientaddr, &slen);
        if (newfd == -1) {
            if (errno == EINTR) {
                continue;
            }else{
                DERROR("accept error: %s\n", strerror(errno));
                return NULL;
            }
        }
        break;
    }
    tcp_server_setopt(newfd);

    conn = (Conn*)zz_malloc(connsize);
    if (conn == NULL) {
        DERROR("conn malloc error.\n");
        MEMLINK_EXIT;
    }
    zz_check(conn);
	
    memset(conn, 0, connsize); 
	conn->rbuf = (char *)zz_malloc(CONN_MAX_READ_LEN);
	conn->rsize = CONN_MAX_READ_LEN;
    conn->sock = newfd;
	conn->destroy = conn_destroy;
	conn->wrote = conn_wrote;    
	inet_ntop(AF_INET, &(clientaddr.sin_addr), conn->client_ip, slen);	
	conn->client_port = ntohs((short)clientaddr.sin_port);
    gettimeofday(&conn->ctime, NULL);
    g_runtime->conn_num++;
    DINFO("accept newfd: %d, %s:%d\n", newfd, conn->client_ip, conn->client_port);
	
    zz_check(conn);

    return conn;
}

void
conn_destroy(Conn *conn)
{
	int i;
	ThreadServer *ts;
	WThread *wt;
	//SThread *st;
	RwConnInfo *conninfo = NULL;
	//SyncConnInfo *sconninfo;

    zz_check(conn);

    if (conn->wbuf) {
        zz_free(conn->wbuf);
    }
	if (conn->rbuf) {
		zz_free(conn->rbuf);
	}
	event_del(&conn->evt);
	
    g_runtime->conn_num--;
	
	if (conn->port == g_cf->read_port) {
		ts = (ThreadServer *)conn->thread;
		conninfo = ts->rw_conn_info;
		ts->conns--;
	} else if (conn->port == g_cf->write_port) {
		wt = (WThread *)conn->thread;
		conninfo = wt->rw_conn_info;
		wt->conns--;
	} 

	if (conninfo && (conn->port == g_cf->read_port || conn->port == g_cf->write_port)) {
		for (i = 0; i < g_cf->max_conn; i++) {
			if (conn->sock == conninfo[i].fd) {
				conninfo[i].fd = 0;
				break;
			}
		}

	}
 	close(conn->sock);
    zz_free(conn);
}

char*
conn_write_buffer(Conn *conn, int size)
{
    if (conn->wsize >= size) {
        conn->wlen = conn->wpos = 0;
        return conn->wbuf;
    }

    if (conn->wbuf != NULL)
        zz_free(conn->wbuf);

    conn->wbuf  = zz_malloc(size);
    conn->wsize = size;
    conn->wlen  = conn->wpos = 0;

    return conn->wbuf;
}

inline char*
conn_write_buffer_append(Conn *conn, void *data, int datalen)
{
    if (conn->wlen + datalen > conn->wsize) {
        int   newsize = (conn->wlen + datalen) * 2;
        char *newdata = zz_malloc(newsize);
    
        if (conn->wlen > 0) {
            memcpy(newdata, conn->wbuf, conn->wlen);
        }
        zz_free(conn->wbuf);
        conn->wsize = newsize;
        conn->wbuf  = newdata;
    }
    
    memcpy(conn->wbuf + conn->wlen, data, datalen);
    conn->wlen += datalen;

    return conn->wbuf;
}

// put datalen and return code to header
int
conn_write_buffer_head(Conn *conn, int retcode, int len)
{
    if (retcode == MEMLINK_OK) {
        conn->wlen = len; 
        len -= sizeof(int);
    }else{
        conn_write_buffer(conn, CMD_REPLY_HEAD_LEN);
        conn->wlen = CMD_REPLY_HEAD_LEN;
        len = CMD_REPLY_HEAD_LEN - sizeof(int);
    }    
    //char bufx[1024];
    //DINFO("wlen:%d, size:%d ret:%d %s\n", conn->wlen, idx, ret, formath(conn->wbuf, conn->wlen, bufx, 1024)); 
    memcpy(conn->wbuf, &len, sizeof(int));
    short retv = retcode; 
    memcpy(conn->wbuf + sizeof(int), &retv, sizeof(short));

    return MEMLINK_OK;
}


int
conn_wrote(Conn *conn)
{
	DINFO("change event to read.\n");
	int ret = change_event(conn, EV_READ|EV_PERSIST, g_cf->timeout, 0);
	if (ret < 0) {
		DERROR("change event error:%d close socket\n", ret);
		conn->destroy(conn);
	}
	return 0;
}

void 
conn_write(Conn *conn)
{
    int ret;

    while (1) {
        ret = write(conn->sock, &conn->wbuf[conn->wpos], conn->wlen - conn->wpos);
        DINFO("write: %d\n", ret);
        if (ret == -1) {
            if (errno == EINTR) {
                continue;
            }else if (errno != EAGAIN) {
                // maybe close conn?
                DERROR("write error! %s\n", strerror(errno)); 
                conn->destroy(conn);
                break;
            }
        }else{
            /*char buf[512];
            DINFO("conn write, ret:%d, wlen:%d, wpos:%d, %x, wbuf:%s\n", ret, 
                    conn->wlen, conn->wpos, conn->wbuf[conn->wpos], 
                    formath(&conn->wbuf[conn->wpos], ret, buf, 512));
            */
            conn->wpos += ret;
        }
        break;
    } 
}

/**
 * @}
 */
