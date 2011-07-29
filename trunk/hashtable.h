#ifndef MEMLINK_HASHTABLE_H
#define MEMLINK_HASHTABLE_H

#include <stdio.h>
#include "mem.h"
//#include "serial.h"
#include "common.h"
#include "conn.h"

typedef struct _memlink_hashnode
{
    char              *key;
    DataBlock         *data; // DataBlock link
    DataBlock         *data_tail; // DataBlock link tail
    struct _memlink_hashnode  *next;
    unsigned char     *attrformat; // attr format:  3:4:5 => [3, 4, 5]
    unsigned char     type:4; // key type: list/queue
    unsigned char     sortfield:4; // which sort? 0:value, 1-255:attr[0-xx]
    unsigned char     valuetype:4;
    unsigned char     attrnum:4; // attrformat count
    unsigned char     valuesize;
    unsigned char     attrsize;
    unsigned int      used; // used data item
    unsigned int      all;  // all data item;
}HashNode;


typedef struct _memlink_hashtable
{
    HashNode    *bunks[HASHTABLE_BUNKNUM]; 
    
}HashTable;

HashTable*      hashtable_create();
void            hashtable_destroy(HashTable *ht);
void			hashtable_clear_all(HashTable *ht);
int				hashtable_remove_key(HashTable *ht, char *key);
int				hashtable_remove_list(HashTable *ht, char *key);
int             hashtable_key_create(HashTable *ht, char *key, int valuesize, char *attrstr, 
                                    unsigned char listtype, unsigned char valuetype);
int             hashtable_key_create_attr(HashTable *ht, char *key, int valuesize, 
                                        unsigned int *attrarray, char attrnum, 
                                        unsigned char listtype, unsigned char valuetype);
HashNode*       hashtable_find(HashTable *ht, char *key);
int             hashtable_find_value(HashTable *ht, char *key, void *value, 
                                     HashNode **node, DataBlock **dbk, char **data);
unsigned int    hashtable_hash(char *key, int len);

int             hashtable_add(HashTable *ht, char *key, void *value, char *attrstr, int pos);
int             hashtable_add_attr(HashTable *ht, char *key, void *value, 
                                   unsigned int *attrarray, char attrnum, int pos);
int             hashtable_add_attr_bin(HashTable *ht, char *key, void *value, 
                                       void *attr, int pos);

int             hashtable_move(HashTable *ht, char *key, void *value, int pos);
int             hashtable_del(HashTable *ht, char *key, void *value);
int             hashtable_tag(HashTable *ht, char *key, void *value, unsigned char tag);
int             hashtable_attr(HashTable *ht, char *key, void *value, unsigned int *attrarray, int attrnum);
int             hashtable_range(HashTable *ht, char *key, unsigned char kind, unsigned int *attrarray, int attrnum, 
                                int frompos, int len, Conn *conn);
int             hashtable_clean(HashTable *ht, char *key);
int             hashtable_stat(HashTable *ht, char *key, HashTableStat *stat);
int				hashtable_stat_sys(HashTable *ht, HashTableStatSys *stat);
int             hashtable_count(HashTable *ht, char *key, unsigned int *attrarray, int attrnum, 
                                int *visible_count, int *tagdel_count);
int             hashtable_lpush(HashTable *ht, char *key, void *value, unsigned int *attrarray, char attrnum);
int             hashtable_rpush(HashTable *ht, char *key, void *value, unsigned int *attrarray, char attrnum);

int             hashtable_lpop(HashTable *ht, char *key, int num, Conn *conn);
int             hashtable_rpop(HashTable *ht, char *key, int num, Conn *conn);

int             hashtable_print(HashTable *ht, char *key);
int             hashtable_check(HashTable *ht, char *key);
int             hashnode_check(HashNode *node);

int             hashtable_del_by_attr(HashTable *ht, char *key, unsigned int *attrarray, int attrnum);

int             hashtable_sortlist_mdel(HashTable *ht, char *key, unsigned char kind, void *valmin, void *valmax, 
                                        unsigned int *attrarray, unsigned char attrnum);
int             hashtable_sortlist_count(HashTable *ht, char *key, unsigned int *attrarray, int attrnum, 
                                        void* valmin, void *valmax, int *visible_count, int *tagdel_count);
int             hashtable_sortlist_range(HashTable *ht, char *key, unsigned char kind, 
				                        unsigned int *attrarray, int attrnum, void *valmin,
                                        void *valmax, Conn *conn);
int				hashtable_sortlist_add_attr_bin(HashTable *ht, char *key, void *value, void *attr);
int				hashtable_sortlist_add_attr(HashTable *ht, char *key, void *value, unsigned int *attrarray, char attrnum);
int             hashtable_clean_all(HashTable *ht);

#endif
