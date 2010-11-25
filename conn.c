#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/socket.h>
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
    DINFO("accept newfd: %d\n", newfd);

    tcp_server_setopt(newfd);

    //conn = (Conn*)zz_malloc(sizeof(Conn));
    conn = (Conn*)zz_malloc(connsize);
    if (conn == NULL) {
        DERROR("conn malloc error.\n");
        MEMLINK_EXIT;
    }
    //memset(conn, 0, sizeof(Conn)); 
    memset(conn, 0, connsize); 
    conn->sock = newfd;
	conn->destroy = conn_destroy;
	conn->wrote = conn_wrote;    
    gettimeofday(&conn->ctime, NULL);

    return conn;
}

void
conn_destroy(Conn *conn)
{
    if (conn->wbuf) {
        zz_free(conn->wbuf);
    }
	event_del(&conn->evt);
    close(conn->sock);
    zz_free(conn);
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


