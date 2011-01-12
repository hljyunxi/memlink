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
    memset(conn, 0, connsize); 
    conn->sock = newfd;
	conn->destroy = conn_destroy;
	conn->wrote = conn_wrote;    
	inet_ntop(AF_INET, &(clientaddr.sin_addr), conn->client_ip, slen);	
	conn->client_port = ntohs((short)clientaddr.sin_port);
    gettimeofday(&conn->ctime, NULL);

    DINFO("accept newfd: %d, %s:%d\n", newfd, conn->client_ip, conn->client_port);

    return conn;
}

void
conn_destroy(Conn *conn)
{
    zz_check(conn);

    if (conn->wbuf) {
        zz_free(conn->wbuf);
    }
	event_del(&conn->evt);
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
