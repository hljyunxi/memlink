#ifndef MEMLINK_SERIAL_H
#define MEMLINK_SERIAL_H

#include <stdio.h>

#define CMD_DUMP		1
#define CMD_CLEAN		2
#define CMD_STAT		3
#define CMD_CREATE		4
#define CMD_DEL			5
#define CMD_INSERT		6
#define CMD_UPDATE		7
#define CMD_MASK		8
#define CMD_TAG			9
#define CMD_RANGE		10
#define CMD_RMKEY       11
#define CMD_COUNT		12
#define CMD_LPUSH		13
#define CMD_LPOP		14
#define CMD_RPUSH		15
#define CMD_RPOP		16

#define CMD_SYNC        100 
#define CMD_GETDUMP		101

typedef struct _ht_stat_sys
{
	unsigned int keys;
	unsigned int values;
}HashTableStatSys;

typedef struct _ht_stat
{
    unsigned char   valuesize;
    unsigned char   masksize;
    unsigned int    blocks; // all blocks
    unsigned int    data;   // all alloc data item
    unsigned int    data_used; // all data item used
    unsigned int    mem;       // all alloc mem
}HashTableStat;


int mask_string2array(char *maskstr, unsigned int *result);
int mask_array2binary(unsigned char *maskformat, unsigned int *maskarray, char masknum, char *mask);
int mask_string2binary(unsigned char *maskformat, char *maskstr, char *mask);
int mask_binary2string(unsigned char *maskformat, int masknum, char *mask, int masklen, char *maskstr);
int mask_array2flag(unsigned char *maskformat, unsigned int *maskarray, char masknum, char *mask);

int cmd_dump_pack(char *data);
int cmd_dump_unpack(char *data);

int cmd_clean_pack(char *data, char *key);
int cmd_clean_unpack(char *data, char *key);

int cmd_rmkey_pack(char *data, char *key);
int cmd_rmkey_unpack(char *data, char *key);

int cmd_count_pack(char *data, char *key, unsigned char masknum, unsigned int *maskarray);
int cmd_count_unpack(char *data, char *key, unsigned char *masknum, unsigned int *maskarray);

int cmd_stat_pack(char *data, char *key);
int cmd_stat_unpack(char *data, char *key);

int cmd_create_pack(char *data, char *key, unsigned char valuelen, 
                    unsigned char masknum, unsigned int *maskformat);
int cmd_create_unpack(char *data, char *key, unsigned char *valuelen, 
                      unsigned char *masknum, unsigned int *maskformat);

int cmd_del_pack(char *data, char *key, char *value, unsigned char valuelen);
int cmd_del_unpack(char *data, char *key, char *value, unsigned char *valuelen);

int cmd_insert_pack(char *data, char *key, char *value, unsigned char valuelen, 
                    unsigned char masknum, unsigned int *maskarray, int pos);  
int cmd_insert_unpack(char *data, char *key, char *value, unsigned char *valuelen,
                      unsigned char *masknum, unsigned int *maskarray, int *pos);

int cmd_update_pack(char *data, char *key, char *value, unsigned char valuelen, int pos);
int cmd_update_unpack(char *data, char *key, char *value, unsigned char *valuelen, int *pos);

int cmd_mask_pack(char *data, char *key, char *value, unsigned char valuelen, 
                  unsigned char masknum, unsigned int *maskarray);
int cmd_mask_unpack(char *data, char *key, char *value, unsigned char *valuelen, 
                    unsigned char *masknum, unsigned int *maskarray);

int cmd_tag_pack(char *data, char *key, char *value, unsigned char valuelen, unsigned char tag);
int cmd_tag_unpack(char *data, char *key, char *value, unsigned char *valuelen, unsigned char *tag);

int cmd_range_pack(char *data, char *key, unsigned char masknum, unsigned int *maskarray, 
                   int frompos, int len);
int cmd_range_unpack(char *data, char *key, unsigned char *masknum, unsigned int*maskarray, 
                     int *frompos, int *len);

// for sync client
int cmd_sync_pack(char *data, unsigned int logver, unsigned int logpos);
int cmd_sync_unpack(char *data, unsigned int *logver, unsigned int *logpos);

int cmd_getdump_pack(char *data, unsigned int dumpver, unsigned long long size);
int cmd_getdump_unpack(char *data, unsigned int *dumpver, unsigned long long *size);


#endif
