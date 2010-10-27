#ifndef MEMLINK_SSLAVE_H
#define MEMLINK_SSLAVE_H

#include <stdio.h>

typedef struct _sslave 
{
    int sock;
	int timeout;
} SSlave;

SSlave* sslave_create();
void	sslave_destroy(SSlave *slave);

#endif
