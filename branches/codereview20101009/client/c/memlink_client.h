#ifndef MEMLINK_CLIENT_H
#define MEMLINK_CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "common.h"

#define MEMLINK_READER  1
#define MEMLINK_WRITER  2
#define MEMLINK_ALL     3

typedef struct _memlink_client
{
    char    host[16];
    int     read_port;
    int     write_port;
    int     readfd;
    int     writefd;
    int     timeout;
}MemLink;

typedef struct _memlink_stat
{
    unsigned char   valuesize;
    unsigned char   masksize;
    unsigned int    blocks; // all blocks
    unsigned int    data;   // all alloc data item
    unsigned int    data_used; // all data item used
    unsigned int    mem;       // all alloc mem
    unsigned int    mem_used;  // all used mem
}MemLinkStat;

typedef struct _memlink_item
{
    struct _memlink_item    *next;
    char                    value[256];
    char                    mask[256];
}MemLinkItem;

typedef struct _memlink_result
{
    int             count;
    int             valuesize;
    int             masksize;
    MemLinkItem     *root;
}MemLinkResult;

void        memlink_result_free(MemLinkResult *result);

MemLink*    memlink_create(char *host, int readport, int writeport, int timeout);
void        memlink_destroy(MemLink *m);
void		memlink_close(MemLink *m);

int         memlink_cmd_dump(MemLink *m);
int			memlink_cmd_clean(MemLink *m, char *key);
int			memlink_cmd_stat(MemLink *m, char *key, MemLinkStat *stat);
int			memlink_cmd_create(MemLink *m, char *key, int valuelen, char *maskstr);
int			memlink_cmd_del(MemLink *m, char *key, char *value, int valuelen);
int			memlink_cmd_insert(MemLink *m, char *key, char *value, int valuelen, 
                               char *maskstr, unsigned int pos);
int			memlink_cmd_update(MemLink *m, char *key, char *value, int valuelen, 
                               unsigned int pos);
int			memlink_cmd_mask(MemLink *m, char *key, char *value, int valuelen, 
                             char *maskstr);
int			memlink_cmd_tag(MemLink *m, char *key, char *value, int valuelen, int tag);
int			memlink_cmd_range(MemLink *m, char *key, char *maskstr, 
                              unsigned int frompos, unsigned int len,
                              MemLinkResult *result);

#endif


