#ifndef MEMLINK_HASHTABLE_H
#define MEMLINK_HASHTABLE_H

#include <stdio.h>
#include "mem.h"
#include "serial.h"
#include "common.h"
#include "conn.h"

typedef struct _hashnode
{
    char            *key;
    DataBlock       *data; // DataBlock link
    DataBlock       *data_tail; // DataBlock link tail
    struct _hashnode *next;
    unsigned char   type; // key type: list/queue
    unsigned char   sortfield; // which sort? 0:value, 1-255:mask[0-xx]
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
void			hashtable_clear_all(HashTable *ht);
int				hashtable_remove_key(HashTable *ht, char *key);
int				hashtable_remove_list(HashTable *ht, char *key);
int             hashtable_key_create(HashTable *ht, char *key, int valuesize, char *maskstr);
int             hashtable_key_create_mask(HashTable *ht, char *key, int valuesize, 
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

int             hashtable_move(HashTable *ht, char *key, void *value, int pos);
int             hashtable_del(HashTable *ht, char *key, void *value);
int             hashtable_tag(HashTable *ht, char *key, void *value, unsigned char tag);
int             hashtable_mask(HashTable *ht, char *key, void *value, unsigned int *maskarray, int masknum);
int             hashtable_range(HashTable *ht, char *key, unsigned char kind, unsigned int *maskarray, int masknum, 
                                int frompos, int len, Conn *conn);
                                //int frompos, int len, char *data, int *datanum);
int             hashtable_clean(HashTable *ht, char *key);
int             hashtable_stat(HashTable *ht, char *key, HashTableStat *stat);
int				hashtable_stat_sys(HashTable *ht, HashTableStatSys *stat);
int             hashtable_count(HashTable *ht, char *key, unsigned int *maskarray, int masknum, 
                                int *visible_count, int *tagdel_count);
int             hashtable_lpush(HashTable *ht, char *key, void *value, unsigned int *maskarray, char masknum);
int             hashtable_rpush(HashTable *ht, char *key, void *value, unsigned int *maskarray, char masknum);

int             hashtable_lpop(HashTable *ht, char *key, int num, Conn *conn);
int             hashtable_rpop(HashTable *ht, char *key, int num, Conn *conn);

int             hashtable_print(HashTable *ht, char *key);

int				dataitem_have_data(HashNode *node, char *itemdata, unsigned char kind);
int				dataitem_check_data(HashNode *node, char *itemdata);

int             hashtable_del_by_mask(HashTable *ht, char *key, unsigned int *maskarray, int masknum);
#endif
