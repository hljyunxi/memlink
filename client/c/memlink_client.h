#ifndef MEMLINK_CLIENT_H
#define MEMLINK_CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define MEMLINK_READER  1
#define MEMLINK_WRITER  2
#define MEMLINK_ALL     3

#define MEMLINK_ERR_TIMEOUT     -1000
#define MEMLINK_ERR             -1
#define MEMLINK_OK              0

typedef struct _memlink_client
{
    char    host[16];
    int     read_port;
    int     write_port;
    int     readfd;
    int     writefd;
    int     timeout;
}MemLink;

MemLink*    memlink_create(char *host, int readport, int writeport, int timeout);
void        memlink_destroy(MemLink *m);
int         memlink_cmd_dump(MemLink *m);


/*
#ifdef DEBUG
#define DEBUG(format,args...) do{\
        fprintf(stderr, format, ##args);\
    }while(0)
#else
#define DEBUG(format,args...)
#endif
*/

#endif


