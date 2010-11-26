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
// 
#define MEMLINK_ERR_RANGE_SIZE		-23
// 发送数据错误
#define MEMLINK_ERR_SEND            -24
// 接收数据错误
#define MEMLINK_ERR_RECV            -25
// 命令执行超时
#define MEMLINK_ERR_TIMEOUT         -26

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
#define CMD_REPLY_HEAD_LEN     (sizeof(int)+sizeof(short))

// format + version + log version + ismaster + size
#define DUMP_HEAD_LEN	    (sizeof(short)+sizeof(int)+sizeof(int)+sizeof(char)+sizeof(long long))
#define SYNCLOG_HEAD_LEN	(sizeof(short)+sizeof(int)+sizeof(char)+sizeof(int))
#define SYNCLOG_INDEXNUM	1000000
#define SYNCPOS_LEN		    (sizeof(int)+sizeof(int))

#define MEMLINK_TAG_DEL     1
#define MEMLINK_TAG_RESTORE 0

#define CMD_GETDUMP_OK			1
#define CMD_GETDUMP_CHANGE		2
#define CMD_GETDUMP_SIZE_ERR	3

#define CMD_SYNC_OK		0
#define CMD_SYNC_FAILED	1


#endif
