#ifndef MEMLINK_SERIAL_H
#define MEMLINK_SERIAL_H

#include <stdio.h>
#include "common.h"

#define CMD_DUMP		    1
#define CMD_CLEAN		    2
#define CMD_STAT		    3
#define CMD_CREATE		    4
#define CMD_DEL			    5
#define CMD_INSERT		    6
#define CMD_MOVE		    7
#define CMD_MASK		    8
#define CMD_TAG			    9
#define CMD_RANGE		    10
#define CMD_RMKEY           11
#define CMD_COUNT		    12
#define CMD_LPUSH		    13
#define CMD_LPOP		    14
#define CMD_RPUSH		    15
#define CMD_RPOP		    16
#define CMD_INSERT_MVALUE   17
#define CMD_INSERT_MKEY     18

	//add by lanwenhong
#define CMD_DEL_BY_MASK     19

#define CMD_PING			20
#define CMD_STAT_SYS		21
	// for sorted list
#define CMD_SL_INSERT       22
#define CMD_SL_DEL          23
#define CMD_SL_COUNT        24
#define CMD_SL_RANGE        25

#define CMD_INSERT_MKV      26 

#define CMD_SYNC            100 
#define CMD_GETDUMP		    101

#define cmd_lpush_unpack    cmd_push_unpack
#define cmd_rpush_unpack    cmd_push_unpack

#define cmd_lpop_unpack     cmd_pop_unpack
#define cmd_rpop_unpack     cmd_pop_unpack

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

int cmd_stat_sys_pack(char *data);
int cmd_stat_sys_unpack(char *data);

int cmd_create_pack(char *data, char *key, unsigned char valuelen, 
                    unsigned char masknum, unsigned int *maskformat,
                    unsigned char listtype, unsigned char valuetype);
int cmd_create_unpack(char *data, char *key, unsigned char *valuelen, 
                      unsigned char *masknum, unsigned int *maskformat,
                      unsigned char *listtype, unsigned char *valuetype);

int cmd_del_pack(char *data, char *key, char *value, unsigned char valuelen);
int cmd_del_unpack(char *data, char *key, char *value, unsigned char *valuelen);

int cmd_insert_pack(char *data, char *key, char *value, unsigned char valuelen, 
                    unsigned char masknum, unsigned int *maskarray, int pos);  
int cmd_insert_unpack(char *data, char *key, char *value, unsigned char *valuelen,
                      unsigned char *masknum, unsigned int *maskarray, int *pos);

int cmd_move_pack(char *data, char *key, char *value, unsigned char valuelen, int pos);
int cmd_move_unpack(char *data, char *key, char *value, unsigned char *valuelen, int *pos);

int cmd_mask_pack(char *data, char *key, char *value, unsigned char valuelen, 
                  unsigned char masknum, unsigned int *maskarray);
int cmd_mask_unpack(char *data, char *key, char *value, unsigned char *valuelen, 
                    unsigned char *masknum, unsigned int *maskarray);

int cmd_tag_pack(char *data, char *key, char *value, unsigned char valuelen, unsigned char tag);
int cmd_tag_unpack(char *data, char *key, char *value, unsigned char *valuelen, unsigned char *tag);

int cmd_range_pack(char *data, char *key, unsigned char kind, unsigned char masknum, unsigned int *maskarray, 
                   int frompos, int len);
int cmd_range_unpack(char *data, char *key, unsigned char *kind, unsigned char *masknum, unsigned int*maskarray, 
                     int *frompos, int *len);

int cmd_ping_pack(char *data);
int cmd_ping_unpack(char *data);

int cmd_push_pack(char *data, unsigned char cmd, char *key, char *value, unsigned char valuelen, 
                    unsigned char masknum, unsigned int *maskarray);
int cmd_lpush_pack(char *data, char *key, char *value, unsigned char valuelen, 
                    unsigned char masknum, unsigned int *maskarray);
int cmd_rpush_pack(char *data, char *key, char *value, unsigned char valuelen, 
                    unsigned char masknum, unsigned int *maskarray);
int cmd_push_unpack(char *data, char *key, char *value, unsigned char *valuelen,
                    unsigned char *masknum, unsigned int *maskarray);

int cmd_pop_pack(char *data, unsigned char cmd, char *key, int num);
int cmd_lpop_pack(char *data, char *key, int num);
int cmd_rpop_pack(char *data, char *key, int num);
int cmd_pop_unpack(char *data, char *key, int *num);

// for sync client
int cmd_sync_pack(char *data, unsigned int logver, unsigned int logpos);
int cmd_sync_unpack(char *data, unsigned int *logver, unsigned int *logpos);

int cmd_getdump_pack(char *data, unsigned int dumpver, unsigned long long size);
int cmd_getdump_unpack(char *data, unsigned int *dumpver, unsigned long long *size);

int cmd_insert_mvalue_pack(char *data, char *key, MemLinkInsertVal *items, int num);
int cmd_insert_mvalue_unpack(char *data, char *key, MemLinkInsertVal **items, int *num);

//add by lanwenhong
int cmd_del_by_mask_pack(char *data, char *key, unsigned int *maskarray, unsigned char masknum);
int cmd_del_by_mask_unpack(char *data, char *key, unsigned int *maskarray, unsigned char *masknum);

int cmd_sortlist_count_pack(char *data, char *key, unsigned char masknum, unsigned int *maskarray,
                        void *valmin, unsigned char vminlen, void *valmax, unsigned char vmaxlen);
int cmd_sortlist_count_unpack(char *data, char *key, unsigned char *masknum, unsigned int *maskarray,
                        void *valmin, unsigned char *vminlen, void *valmax, unsigned char *vmaxlen);
int cmd_sortlist_del_pack(char *data, char *key, char *valmin, unsigned char vminlen, 
                        char *valmax, unsigned char vmaxlen);
int cmd_sortlist_del_unpack(char *data, char *key, char *valmin, unsigned char *vminlen,
                        char *valmax, unsigned char *vmaxlen);

int cmd_sortlist_range_pack(char *data, char *key, unsigned char kind, 
                        unsigned char masknum, unsigned int *maskarray, 
                        void *valmin, unsigned char vminlen, void *valmax, unsigned char vmaxlen);
int cmd_sortlist_range_unpack(char *data, char *key, unsigned char *kind, 
                        unsigned char *masknum, unsigned int *maskarray, 
                        void *valmin, unsigned char *vminlen, void *valmax, unsigned char *vmaxlen);

int cmd_insert_mkv_pack(char *data, MemLinkInsertMkv *mkv);
int cmd_insert_mkv_unpack_packagelen(char *data, unsigned int *package_len);
int cmd_insert_mkv_unpack_key(char *data, char *key, unsigned int *valcount, char **conutstart);
int cmd_insert_mkv_unpack_val(char *data, char *value, unsigned char *valuelen, unsigned char *masknum, unsigned int *maskarray, int *pos);

#endif
