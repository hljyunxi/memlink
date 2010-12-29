#ifndef MEMLINK_MEM_H
#define MEMLINK_MEM_H

#include <stdio.h>

#define MEMLINK_MEM_NUM     100

#ifdef __APPLE__
#define fopen64 fopen
#endif

typedef struct _data_block
{
    unsigned short      visible_count; // visible item count
    unsigned short      tagdel_count;  // tag delete item count, invisible
	unsigned short		data_count; // data count in one block
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
    int         idxnum;  // freemem size
    int         idxused; // freemem used size
	int			blocks;
}MemPool;

//extern MemPool  *g_mpool;

MemPool*    mempool_create();
DataBlock*  mempool_get(MemPool *mp, int blocksize);
int         mempool_put(MemPool *mp, DataBlock *dbk, int blocksize);
int         mempool_expand(MemPool *mp);
void        mempool_free(MemPool *mp, int blocksize);
void        mempool_destroy(MemPool *mp);

#endif
