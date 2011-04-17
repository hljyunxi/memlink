#ifndef MEMLINK_INFO_H
#define MEMLINK_INFO_H

#include "common.h"
#include "conn.h"
#include <time.h>

typedef struct _rw_conn_info
{
	int  fd;
	char client_ip[16];
	int  port;
	int  cmd_count;

	struct timeval start;
}RwConnInfo;

typedef struct _sync_conn_info
{
	int  fd;
	char client_ip[16];
	int  port;
	int  cmd_count;
	int  logver;
	int  logline;
	int  delay;

	unsigned char status;
	struct timeval start;	
}SyncConnInfo;

int info_sys_stat(MemLinkStatSys *stat);
int read_conn_info(Conn *conn);
int write_conn_info(Conn *conn);
int sync_conn_info(Conn *conn);

#endif
