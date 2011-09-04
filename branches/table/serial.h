#ifndef MEMLINK_SERIAL_H
#define MEMLINK_SERIAL_H

#include <stdio.h>
#include "common.h"
#include "info.h"
#include "myconfig.h"

#define CMD_DUMP		    1
#define CMD_CLEAN		    2
#define CMD_STAT		    3
#define CMD_CREATE_TABLE    4
#define CMD_CREATE_NODE     5
#define CMD_RMTABLE			6
#define CMD_DEL			    7
#define CMD_INSERT		    8
#define CMD_MOVE		    9
#define CMD_ATTR		    10
#define CMD_TAG			    11
#define CMD_RANGE		    12
#define CMD_RMKEY           13
#define CMD_COUNT		    14
#define CMD_LPUSH		    15
#define CMD_LPOP		    16
#define CMD_RPUSH		    17
#define CMD_RPOP		    18

//add by lanwenhong
#define CMD_DEL_BY_ATTR     19

#define CMD_PING			20
#define CMD_STAT_SYS		21
// for sorted list
#define CMD_SL_INSERT       22
#define CMD_SL_DEL          23
#define CMD_SL_COUNT        24
#define CMD_SL_RANGE        25

#define CMD_INSERT_MKV      26 
#define CMD_READ_CONN_INFO  27
#define CMD_WRITE_CONN_INFO 28
#define CMD_SYNC_CONN_INFO  29
#define CMD_CONFIG_INFO     30

#define CMD_SET_CONFIG      31

#define CMD_CLEAN_ALL       32

#define CMD_WRITE			34
#define CMD_WRITE_RESULT	35
#define CMD_GETPORT			36
#define CMD_BACKUP_ACK      37

#define CMD_SYNC            100
#define CMD_GETDUMP		    101
#define CMD_HEARTBEAT       102

#define CMD_VOTE_MIN		200
#define CMD_VOTE            200
#define CMD_VOTE_WAIT       201
#define CMD_VOTE_MASTER     202
#define CMD_VOTE_BACKUP     203
#define CMD_VOTE_NONEED     204
#define CMD_VOTE_UPDATE     205
#define CMD_VOTE_DETECT     206
#define CMD_VOTE_MAX		206

#define cmd_lpush_unpack    cmd_push_unpack
#define cmd_rpush_unpack    cmd_push_unpack

#define cmd_lpop_unpack     cmd_pop_unpack
#define cmd_rpop_unpack     cmd_pop_unpack

int attr_string2array(char *attrstr, unsigned int *result);
int attr_array2binary(unsigned char *attrformat, unsigned int *attrarray, char attrnum, char *attr);
int attr_string2binary(unsigned char *attrformat, char *attrstr, char *attr);
int attr_binary2string(unsigned char *attrformat, int attrnum, char *attr, int attrlen, char *attrstr);
int attr_binary2array(unsigned char *attrformat, int attrnum, char *attr, unsigned int *attrarray);
int attr_array2flag(unsigned char *attrformat, unsigned int *attrarray, char attrnum, char *attr);

int cmd_dump_pack(char *data);
int cmd_dump_unpack(char *data);

int cmd_clean_pack(char *data, char *key);
int cmd_clean_unpack(char *data, char *key);

int cmd_rmkey_pack(char *data, char *key);
int cmd_rmkey_unpack(char *data, char *key);

int cmd_rmtable_pack(char *data, char *key);
int cmd_rmtable_unpack(char *data, char *key);

int cmd_count_pack(char *data, char *key, unsigned char attrnum, unsigned int *attrarray);
int cmd_count_unpack(char *data, char *key, unsigned char *attrnum, unsigned int *attrarray);

int cmd_stat_pack(char *data, char *key);
int cmd_stat_unpack(char *data, char *key);

int cmd_stat_sys_pack(char *data);
int cmd_stat_sys_unpack(char *data);

int cmd_create_table_pack(char *data, char *name, unsigned char valuelen, 
                    unsigned char attrnum, unsigned int *attrformat,
                    unsigned char listtype, unsigned char valuetype);
int cmd_create_table_unpack(char *data, char *name, unsigned char *valuelen, 
                      unsigned char *attrnum, unsigned int *attrformat,
                      unsigned char *listtype, unsigned char *valuetype);

int cmd_create_node_pack(char *data, char *name, char *key);
int cmd_create_node_unpack(char *data, char *name, char *key);

int cmd_del_pack(char *data, char *key, char *value, unsigned char valuelen);
int cmd_del_unpack(char *data, char *key, char *value, unsigned char *valuelen);

int cmd_insert_pack(char *data, char *key, char *value, unsigned char valuelen, 
                    unsigned char attrnum, unsigned int *attrarray, int pos);  
int cmd_insert_unpack(char *data, char *key, char *value, unsigned char *valuelen,
                      unsigned char *attrnum, unsigned int *attrarray, int *pos);

int cmd_move_pack(char *data, char *key, char *value, unsigned char valuelen, int pos);
int cmd_move_unpack(char *data, char *key, char *value, unsigned char *valuelen, int *pos);

int cmd_attr_pack(char *data, char *key, char *value, unsigned char valuelen, 
                  unsigned char attrnum, unsigned int *attrarray);
int cmd_attr_unpack(char *data, char *key, char *value, unsigned char *valuelen, 
                    unsigned char *attrnum, unsigned int *attrarray);

int cmd_tag_pack(char *data, char *key, char *value, unsigned char valuelen, unsigned char tag);
int cmd_tag_unpack(char *data, char *key, char *value, unsigned char *valuelen, unsigned char *tag);

int cmd_range_pack(char *data, char *key, unsigned char kind, unsigned char attrnum, unsigned int *attrarray, 
                   int frompos, int len);
int cmd_range_unpack(char *data, char *key, unsigned char *kind, unsigned char *attrnum, unsigned int*attrarray, 
                     int *frompos, int *len);

int cmd_ping_pack(char *data);
int cmd_ping_unpack(char *data);

int cmd_push_pack(char *data, unsigned char cmd, char *key, char *value, unsigned char valuelen, 
                    unsigned char attrnum, unsigned int *attrarray);
int cmd_lpush_pack(char *data, char *key, char *value, unsigned char valuelen, 
                    unsigned char attrnum, unsigned int *attrarray);
int cmd_rpush_pack(char *data, char *key, char *value, unsigned char valuelen, 
                    unsigned char attrnum, unsigned int *attrarray);
int cmd_push_unpack(char *data, char *key, char *value, unsigned char *valuelen,
                    unsigned char *attrnum, unsigned int *attrarray);

int cmd_pop_pack(char *data, unsigned char cmd, char *key, int num);
int cmd_lpop_pack(char *data, char *key, int num);
int cmd_rpop_pack(char *data, char *key, int num);
int cmd_pop_unpack(char *data, char *key, int *num);

// for sync client
int cmd_sync_pack(char *data, unsigned int logver, unsigned int logpos, int bcount, char *md5);
int cmd_sync_unpack(char *data, unsigned int *logver, unsigned int *logpos, int *bcount, char *md5);

int cmd_getdump_pack(char *data, unsigned int dumpver, uint64_t size);
int cmd_getdump_unpack(char *data, unsigned int *dumpver, uint64_t *size);

//int cmd_insert_mvalue_pack(char *data, char *key, MemLinkInsertVal *items, int num);
//int cmd_insert_mvalue_unpack(char *data, char *key, MemLinkInsertVal **items, int *num);

//add by lanwenhong
int cmd_del_by_attr_pack(char *data, char *key, unsigned int *attrarray, unsigned char attrnum);
int cmd_del_by_attr_unpack(char *data, char *key, unsigned int *attrarray, unsigned char *attrnum);

int cmd_sortlist_count_pack(char *data, char *key, unsigned char attrnum, unsigned int *attrarray,
                        void *valmin, unsigned char vminlen, void *valmax, unsigned char vmaxlen);
int cmd_sortlist_count_unpack(char *data, char *key, unsigned char *attrnum, unsigned int *attrarray,
                        void *valmin, unsigned char *vminlen, void *valmax, unsigned char *vmaxlen);
int cmd_sortlist_del_pack(char *data, char *key, unsigned char kind, char *valmin, unsigned char vminlen, 
                        char *valmax, unsigned char vmaxlen, unsigned char attrnum, unsigned int *attrarray);
int cmd_sortlist_del_unpack(char *data, char *key, unsigned char *kind, char *valmin, unsigned char *vminlen,
                        char *valmax, unsigned char *vmaxlen, unsigned char *attrnum, unsigned int *attrarray);

int cmd_sortlist_range_pack(char *data, char *key, unsigned char kind, 
                        unsigned char attrnum, unsigned int *attrarray, 
                        void *valmin, unsigned char vminlen, void *valmax, unsigned char vmaxlen);
int cmd_sortlist_range_unpack(char *data, char *key, unsigned char *kind, 
                        unsigned char *attrnum, unsigned int *attrarray, 
                        void *valmin, unsigned char *vminlen, void *valmax, unsigned char *vmaxlen);

int cmd_insert_mkv_pack(char *data, MemLinkInsertMkv *mkv);
int cmd_insert_mkv_unpack_packagelen(char *data, unsigned int *package_len);
int cmd_insert_mkv_unpack_key(char *data, char *key, unsigned int *valcount, char **conutstart);
int cmd_insert_mkv_unpack_val(char *data, char *value, unsigned char *valuelen, unsigned char *attrnum, unsigned int *attrarray, int *pos);
int cmd_read_conn_info_pack(char *data);
int cmd_read_conn_info_unpack(char *data);
int cmd_write_conn_info_pack(char *data);
int cmd_write_conn_info_unpack(char *data);
int cmd_sync_conn_info_pack(char *data);
int cmd_sync_conn_info_unpack(char *data);

int cmd_config_info_pack(char *data);
int cmd_config_info_unpack(char *data);
int pack_config_struct(char *data, MyConfig *config);
int unpack_config_struct(char *data, MyConfig *config);
int cmd_set_config_dynamic_pack(char *data, char *key, char *value);
int cmd_set_config_dynamic_unpack(char *data, char *key, char *value);
int cmd_clean_all_pack(char *data);
int cmd_clean_all_unpack(char *data);
int cmd_vote_pack(char *data, uint64_t id, unsigned char result, uint64_t voteid, unsigned short port);
int cmd_heartbeat_pack(char *data, int port);
int cmd_heartbeat_unpack(char *data, int *port);
int cmd_backup_ack_pack(char *data, char state);
int cmd_backup_ack_unpack(char *data, unsigned char *cmd, short *ret, int *logver, int *logline);
int cmd_vote_pack(char *data, uint64_t id, unsigned char result, uint64_t voteid, unsigned short port);
int unpack_votehost(char *buf, char *ip, uint16_t *port);
int unpack_voteid(char *buf, uint64_t *vote_id);


#endif
