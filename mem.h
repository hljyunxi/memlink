#ifndef MEMLINK_MEM_H
#define MEMLINK_MEM_H

#include <stdio.h>

#define MEMLINK_MEM_NUM     100

typedef struct _data_block
{
    unsigned short      visible_count; // visible item count
    unsigned short      mask_count;  // masked item count, invisible
    struct _data_block  *next;
    char                data[0];
}DataBlock;

typedef struct _mem_item
{
    int         memsize;
    DataBlock   *data;
}MemItem;

typedef struct _mempool
{
    MemItem     *freemem;
    int         idxnum;
    int         idxused;
}MemPool;

extern MemPool  *g_mpool;

MemPool*    mempool_create();
DataBlock*  mempool_get(MemPool *mp, int blocksize);
int         mempool_put(MemPool *mp, DataBlock *dbk, int blocksize);
int         mempool_expand(MemPool *mp);
void        mempool_free(MemPool *mp, int blocksize);
void        mempool_destroy(MemPool *mp);

#endif
