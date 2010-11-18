#ifndef MEMLINK_SSLAVE_H
#define MEMLINK_SSLAVE_H

#include <stdio.h>
#include "synclog.h"

#define SLAVE_STATUS_INIT	0
#define SLAVE_STATUS_SYNC	1
#define SLAVE_STATUS_DUMP	2
#define SLAVE_STATUS_RECV	3

typedef struct _sslave 
{
    int sock;
	int timeout;
	int status; // 0.init 1. sync 2.dump 3.recv new log

	int binlog_ver;
	int binlog_index;
	//int binlog_min_ver;  the same as g_runtime->dumplogver

	SyncLog		*binlog;

    unsigned int logver; // last logver
    unsigned int logline; // last logline

	unsigned int dump_logver; // logver in dumpfile
    long long    dumpsize; // size in dumpfile
    long long    dumpfile_size;

    int			 trycount; // count of get last sync position
	volatile int isrunning;
} SSlave;

SSlave* sslave_create();
void	sslave_go(SSlave *slave);
void	sslave_destroy(SSlave *slave);
void	sslave_close(SSlave *slave);

int		sslave_load_master_dump_info(SSlave *ss);

#endif
