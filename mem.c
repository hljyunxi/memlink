#include <stdlib.h>
#include <string.h>
#include "zzmalloc.h"
#include "logfile.h"
#include "mem.h"

MemPool *g_mpool;

MemPool*    
mempool_create()
{
    MemPool *mp;

    mp = (MemPool*)zz_malloc(sizeof(MemPool));
    if (NULL == mp) {
        DERROR("malloc MemPool error!\n");
        return NULL;
    }
    memset(mp, 0, sizeof(MemPool));

    mp->idxnum = MEMLINK_MEM_NUM;
    mp->freemem = (MemItem*)zz_malloc(sizeof(MemItem) * mp->idxnum);
    if (NULL == mp) {
        DERROR("malloc MemItem error!\n");
        zz_free(mp);
        return NULL;
    }
    memset(mp->freemem, 0, sizeof(MemItem) * mp->idxnum);
   
    g_mpool = mp;

    return mp;
}

DataBlock*
mempool_get(MemPool *mp, int blocksize)
{
    int i;
    DataBlock *dbk = NULL, *dbn;

    for (i = 0; i < mp->idxused; i++) {
        if (mp->freemem[i].memsize == blocksize) {
            dbk = mp->freemem[i].data;
            break;
        }
    }

    if (NULL == dbk) {
        int len = sizeof(DataBlock) + g_cf->block_data_count * blocksize;
        dbk = (DataBlock*)zz_malloc(len);
        if (NULL == dbk) {
            DERROR("malloc DataBlock error!\n");
            return NULL;
        }
        memset(dbk, 0, len);

        return dbk;
    }
        
    
    dbn = dbk->next;
    mp->freemem[i].data = dbn; 

    memset(dbk, 0, sizeof(DataBlock) + g_cf->block_data_count * blocksize);

    return dbk;
}

int
mempool_put(MemPool *mp, DataBlock *dbk, int blocksize)
{
    int i;

    for (i = 0; i < mp->idxused; i++) {
        if (mp->freemem[i].memsize == blocksize) {
            dbk->next = mp->freemem[i].data;
            mp->freemem[i].data = dbk; 
            return 0;
        }
    }
    
    if (i < mp->idxnum) {
        mp->freemem[i].data = dbk;
        mp->freemem[i].memsize = blocksize;
        mp->idxused += 1;
    }else{
        if (mempool_expand(mp) == -1)
            return -2;
        
        mp->freemem[mp->idxused].memsize = blocksize;
        mp->freemem[mp->idxused].data = dbk;

        mp->idxused += 1;
    }

    return 0;
}

int
mempool_expand(MemPool *mp)
{
    int newnum = mp->idxnum * 2;           
    MemItem  *newitems = (MemItem*)zz_malloc(sizeof(MemItem) * newnum);
    if (NULL == newitems) {
		DERROR("malloc error!\n");
		MEMLINK_EXIT;
        return -1;
    }
    
    memcpy(newitems, mp->freemem, sizeof(MemItem) * mp->idxused);

    zz_free(mp->freemem);
    mp->freemem = newitems;
    mp->idxnum = newnum;

    return 0;
}

void 
mempool_free(MemPool *mp, int blocksize)
{
    int i;

    for (i = 0; i < mp->idxused; i++) {
        if (mp->freemem[i].memsize == blocksize) {
            DataBlock *dbk = mp->freemem[i].data;
            DataBlock *tmp; 

            while (dbk) {
                tmp = dbk;
                dbk = dbk->next;

                zz_free(tmp);
            }
            
            mp->freemem[i].data = NULL;
        }
    }
}

void
mempool_destroy(MemPool *mp)
{
    if (NULL == mp)
        return;
   
    int i;

    for (i = 0; i < mp->idxused; i++) {
        DataBlock *dbk = mp->freemem[i].data;
        DataBlock *tmp; 

        while (dbk) {
            tmp = dbk;
            dbk = dbk->next;

            zz_free(tmp);
        }

        mp->freemem[i].data = NULL;
    }

    zz_free(mp->freemem);
    zz_free(mp);
}



