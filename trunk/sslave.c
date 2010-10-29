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
#include "utils.h"
#include "network.h"
#include "serial.h"
#include "sslave.h"

// TODO
// code to process the sync log data from sync master
int
sslave_forever(SSlave *ss)
{
	//char sndbuf[64];
	//int  sndsize = 0;
	// send sync

	//cmd_sync_pack(sndbuf);
	//writen(ss->sock, sndbuf, sndsize, ss->timeout);

	return 0;
}


/**
 * slave sync thread
 * 1.find local sync logver/logline 2.send sync command to server 3.get dump/sync message
 */
void*
sslave_run(void *args)
{
	SSlave	*ss = (SSlave*)args;
	int		ret;

	while (1) {
		if (ss->sock == 0) {
			ret = tcp_socket_connect(g_cf->master_sync_host, g_cf->master_sync_port, ss->timeout);
			if (ret <= 0) {
				DERROR("connect error! ret:%d, continue.\n", ret);
				sleep(1);
				continue;
			}
		}

		sslave_forever(ss);
	}

	return NULL;
}

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

	ss->timeout = g_cf->timeout;

    pthread_t threadid;
    pthread_attr_t attr;
    int ret;

    ret = pthread_attr_init(&attr);
    if (ret != 0) {
        DERROR("pthread_attr_init error: %s\n", strerror(errno));
        MEMLINK_EXIT;
    }

    ret = pthread_create(&threadid, &attr, sslave_run, ss);
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
