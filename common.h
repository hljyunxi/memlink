#ifndef MEMLINK_COMMON_H
#define MEMLINK_COMMON_H

// 客户端错误
#define MEMLINK_ERR_CLIENT          -10
// 服务器端错误
#define MEMLINK_ERR_SERVER          -11
// 服务器端临时错误
#define MEMLINK_ERR_SERVER_TEMP     -12
// 客户端发送的命令号错误
#define MEMLINK_ERR_CLIENT_CMD      -13
// key不存在
#define MEMLINK_ERR_NOKEY           -14 
// key已经存在
#define MEMLINK_ERR_EKEY            -15
// 网络错误
#define MEMLINK_ERR_CONNECT         -16
// 返回代码错误
#define MEMLINK_ERR_RETCODE         -17
// value不存在
#define MEMLINK_ERR_NOVAL           -18
// 内存错误
#define MEMLINK_ERR_MEM				-19
// mask错误
#define MEMLINK_ERR_MASK			-20
// 包错误
#define MEMLINK_ERR_PACKAGE			-21
// 该项已删除
#define MEMLINK_ERR_REMOVED         -22
// RANGE条目错误
#define MEMLINK_ERR_RANGE_SIZE		-23
// 发送数据错误
#define MEMLINK_ERR_SEND            -24
// 接收数据错误
#define MEMLINK_ERR_RECV            -25
// 命令执行超时
#define MEMLINK_ERR_TIMEOUT         -26
// key错误 
#define MEMLINK_ERR_KEY				-27
// 客户端传递参数错误 
#define MEMLINK_ERR_PARAM			-28
// 文件IO错误
#define MEMLINK_ERR_IO              -29
// list类型错误
#define MEMLINK_ERR_LIST_TYPE       -30

// 其他错误
#define MEMLINK_ERR                 -1
// 操作失败
#define MEMLINK_FAILED				-1
// 执行成功
#define MEMLINK_OK                  0
// 真
#define MEMLINK_TRUE				1
// 假
#define MEMLINK_FALSE				0

// GETDUMP 命令中大小错误
#define MEMLINK_ERR_DUMP_SIZE       -100
// GETDUMP 命令中dump文件错误
#define MEMLINK_ERR_DUMP_VER        -101


#define ROLE_MASTER		1
#define ROLE_SLAVE		0

// 命令执行返回信息头部长度
// datalen(4B) + retcode(2B)
#define CMD_REPLY_HEAD_LEN  (sizeof(int)+sizeof(short))
//#define CMD_REQ_HEAD_LEN	(sizeof(short)+sizeof(char))
#define CMD_REQ_HEAD_LEN    (sizeof(int)+sizeof(char))
//#define CMD_REQ_SIZE_LEN	sizeof(short)
#define CMD_REQ_SIZE_LEN    sizeof(int)

// format + version + log version + log position + size
#define DUMP_HEAD_LEN	    (sizeof(short)+sizeof(int)+sizeof(int)+sizeof(int)+sizeof(long long))
// format + version + index size
#define SYNCLOG_HEAD_LEN	(sizeof(short)+sizeof(int)+sizeof(int))
#define SYNCLOG_INDEXNUM	100000
#define SYNCPOS_LEN		    (sizeof(int)+sizeof(int))

#define MEMLINK_TAG_DEL     1
#define MEMLINK_TAG_RESTORE 0

#define CMD_GETDUMP_OK			1
#define CMD_GETDUMP_CHANGE		2
#define CMD_GETDUMP_SIZE_ERR	3

#define CMD_SYNC_OK		        0
#define CMD_SYNC_FAILED	        1

#define CMD_RANGE_MAX_SIZE			1024000

/// HashTable中桶的数量
#define HASHTABLE_BUNKNUM           1000000
/// 最大允许的mask项数
#define HASHTABLE_MASK_MAX_BIT      32
#define HASHTABLE_MASK_MAX_BYTE     4
#define HASHTABLE_MASK_MAX_ITEM     16
// key最大长度
#define HASHTABLE_KEY_MAX           255
// value最大长度
#define HASHTABLE_VALUE_MAX         255

// 可见和标记删除
#define MEMLINK_VALUE_ALL            0
// 可见
#define MEMLINK_VALUE_VISIBLE        1
// 标记删除
#define MEMLINK_VALUE_TAGDEL         2
// 真实删除
#define MEMLINK_VALUE_REMOVED        4

#define MEMLINK_LIST        1
#define MEMLINK_QUEUE       2
#define MEMLINK_SORT_LIST   3

typedef struct _memlink_insert_mvalue_item
{
    char            value[255];
    unsigned char   valuelen;
    char			maskstr[HASHTABLE_MASK_MAX_ITEM * 3];
    unsigned int    maskarray[HASHTABLE_MASK_MAX_ITEM];
    unsigned char   masknum;
    int             pos;
}MemLinkInsertVal;

typedef struct _memlink_stat
{
    unsigned char   valuesize;
    unsigned char   masksize;
    unsigned int    blocks; // all blocks
    unsigned int    data;   // all alloc data item
    unsigned int    data_used; // all data item used
    unsigned int    mem;       // all alloc mem
	unsigned int    visible;
	unsigned int    tagdel;
}MemLinkStat;

typedef MemLinkStat HashTableStat;

typedef struct _ht_stat_sys
{
    unsigned int keys;
    unsigned int values;
    unsigned int blocks;
    unsigned int data;
    unsigned int data_used;
    unsigned int block_values;
    unsigned int ht_mem;
    unsigned int pool_blocks;
}MemLinkStatSys;

typedef MemLinkStatSys	HashTableStatSys;

#endif
