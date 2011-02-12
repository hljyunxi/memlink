#ifndef MEMLINK_DATABLOCK_H
#define MEMLINK_DATABLOCK_H

#include <stdio.h>
#include "hashtable.h"

int         dataitem_have_data(HashNode *node, char *itemdata, unsigned char kind);
int         dataitem_check_data(HashNode *node, char *itemdata);
char*       dataitem_lookup(HashNode *node, void *value, DataBlock **dbk);
int         dataitem_copy(HashNode *node, char *addr, void *value, void *mask);
int         dataitem_copy_mask(HashNode *node, char *addr, char *maskflag, char *mask);
int         dataitem_lookup_pos_mask(HashNode *node, int pos, unsigned char kind, 
						char *maskval, char *maskflag, DataBlock **dbk, int *dbkpos);
int         dataitem_skip2pos(HashNode *node, DataBlock *dbk, int skip, unsigned char kind);

int         datablock_print(HashNode *node, DataBlock *dbk);
int         datablock_del(HashNode *node, DataBlock *dbk, char *data);
int         datablock_del_restore(HashNode *node, DataBlock *dbk, char *data);
int         datablock_lookup_pos(HashNode *node, int pos, unsigned char kind, DataBlock **dbk);
int         datablock_lookup_valid_pos(HashNode *node, int pos, unsigned char kind, DataBlock **dbk);
DataBlock*  datablock_new_copy_small(HashNode *node, void *value, void *mask);
int         datablock_check_idle(HashNode *node, DataBlock *dbk, int skipn, void *value, void *mask);
int         datablock_check_idle_pos(HashNode *node, DataBlock *dbk, int pos, void *value, void *mask);
DataBlock*  datablock_new_copy(HashNode *node, DataBlock *dbk, int skipn, void *value, void *mask);
DataBlock*  datablock_new_copy_pos(HashNode *node, DataBlock *dbk, int pos, void *value, void *mask);
int         datablock_copy(DataBlock *tobk, DataBlock *frombk, int datalen);
int         datablock_free(DataBlock *headbk, DataBlock *tobk, int datalen);
int         datablock_free_inverse(DataBlock *startbk, DataBlock *endbk, int datalen);

int         mask_array2_binary_flag(unsigned char *maskformat, unsigned int *maskarray, 
                    int masknum, char *maskval, char *maskflag);

int         sortlist_lookup(HashNode *node, int step, void *value, int kind, DataBlock **dbk);

#endif
