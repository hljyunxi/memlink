#ifndef MEMLINK_SSLAVE_H
#define MEMLINK_SSLAVE_H

#include <stdio.h>

typedef struct _sslave 
{
    int sock;
	int timeout;

	unsigned int binlog_ver;
	unsigned int binlog_index;
	unsigned int binlog_min_ver;

    unsigned int logver; // last logver
    unsigned int logline; // last logline

	unsigned int dump_logver; // logver in dumpfile
    long long    dumpsize;
    long long    dumpfile_size;
} SSlave;

SSlave* sslave_create();
void	sslave_destroy(SSlave *slave);
void	sslave_close(SSlave *slave);

#endif
