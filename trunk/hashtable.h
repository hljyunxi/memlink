#ifndef MEMLINK_HASHTABLE_H
#define MEMLINK_HASHTABLE_H

#include <stdio.h>
#include "mem.h"
#include "serial.h"

/// HashTable中桶的数量
#define HASHTABLE_BUNKNUM           1000000
/// 最大允许的mask项数
#define HASHTABLE_MASK_MAX_LEN      16

#define MEMLINK_ITEM_REMOVED        1
#define MEMLINK_ITEM_TAGDEL         2
#define MEMLINK_ITEM_VISIBLE        3


typedef struct _hashnode
{
    char            *key;
    DataBlock       *data; // DataBlock link
    struct _hashnode *next;
    unsigned char   valuesize;
    //unsigned char   valuetype;
    unsigned char   masksize;
    unsigned char   masknum; // maskformat count
    unsigned char   *maskformat; // mask format:  3:4:5 => [3, 4, 5]
    unsigned int    used; // used data item
    unsigned int    all;  // all data item;
}HashNode;


typedef struct _hashtable
{
    HashNode    *bunks[HASHTABLE_BUNKNUM]; 
    
}HashTable;

HashTable*      hashtable_create();
void            hashtable_destroy(HashTable *ht);
int             hashtable_add_info(HashTable *ht, char *key, int valuesize, char *maskstr);
int             hashtable_add_info_mask(HashTable *ht, char *key, int valuesize, 
                                        unsigned int *maskarray, char masknum);
HashNode*       hashtable_find(HashTable *ht, char *key);
int             hashtable_find_value(HashTable *ht, char *key, void *value, 
                                     HashNode **node, DataBlock **dbk, char **data);
unsigned int    hashtable_hash(char *key, int len);

//int             hashtable_add_first(HashTable *ht, char *key, void *value, char *maskstr);
//int             hashtable_add_first_mask(HashTable *ht, char *key, void *value, 
//                                         unsigned int *maskarray, char masknum);

int             hashtable_add(HashTable *ht, char *key, void *value, char *maskstr, int pos);
int             hashtable_add_mask(HashTable *ht, char *key, void *value, 
                                   unsigned int *maskarray, char masknum, int pos);
int             hashtable_add_mask_bin(HashTable *ht, char *key, void *value, 
                                       void *mask, int pos);

int             hashtable_update(HashTable *ht, char *key, void *value, int pos);
int             hashtable_del(HashTable *ht, char *key, void *value);
int             hashtable_tag(HashTable *ht, char *key, void *value, unsigned char tag);
int             hashtable_mask(HashTable *ht, char *key, void *value, unsigned int *maskarray, int masknum);
int             hashtable_range(HashTable *ht, char *key, unsigned int *maskarray, int masknum, 
                                int frompos, int len,
                                char *data, int *datanum, unsigned char *valuesize, unsigned char *masksize);
//int             hashtable_range_mask_bin(HashTable *ht, char *key, void *mask, int frompos, int len);
int             hashtable_clean(HashTable *ht, char *key);
int             hashtable_stat(HashTable *ht, char *key, HashTableStat *stat);
int             hashtable_count(HashTable *ht, char *key, int *visible_count, int *mask_count);
int             hashtable_print(HashTable *ht, char *key);


#endif
