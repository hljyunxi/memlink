#ifndef MEMLINK_SSLAVE_H
#define MEMLINK_SSLAVE_H

#include <stdio.h>

typedef struct _sslave 
{
    int sock;
	int timeout;
    unsigned int logver; // last logver
    unsigned int logline; // last logline
    long long    dumpsize;
    long long    dumpfile_size;
} SSlave;

SSlave* sslave_create();
void	sslave_destroy(SSlave *slave);
void	sslave_close(SSlave *slave);

#endif
