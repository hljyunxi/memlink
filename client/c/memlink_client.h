#ifndef MEMLINK_CLIENT_H
#define MEMLINK_CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define MEMLINK_READER  1
#define MEMLINK_WRITER  2
#define MEMLINK_ALL     3

// 命令执行超时
#define MEMLINK_ERR_TIMEOUT			-1000
// 发送数据错误
#define MEMLINK_ERR_SEND			-100
// 接收数据错误
#define MEMLINK_ERR_RECV			-200
// 客户端错误
#define MEMLINK_ERR_CLIENT			-10
// 服务器端错误
#define	MEMLINK_ERR_SERVER			-11
// 服务器端临时错误
#define MEMLINK_ERR_SERVER_TEMP		-12
// 客户端发送的命令号错误
#define MEMLINK_ERR_CLIENT_CMD		-13
// key不存在
#define MEMLINK_ERR_NOKEY			-14	
// key已经存在
#define MEMLINK_ERR_EKEY			-15
// 网络错误
#define MEMLINK_ERR_CONNECT			-16
// 其他错误
#define MEMLINK_ERR					-1
// 执行成功
#define MEMLINK_OK					0

typedef struct _memlink_client
{
    char    host[16];
    int     read_port;
    int     write_port;
    int     readfd;
    int     writefd;
    int     timeout;
}MemLink;

MemLink*    memlink_create(char *host, int readport, int writeport, int timeout);
void        memlink_destroy(MemLink *m);
void		memlink_close(MemLink *m);

int         memlink_cmd_dump(MemLink *m);
int			memlink_cmd_clean(MemLink *m, char *key);
int			memlink_cmd_stat(MemLink *m, char *key);
int			memlink_cmd_create(MemLink *m, char *key, int valuelen, char *maskstr);
int			memlink_cmd_del(MemLink *m, char *key, char *value, int valuelen);
int			memlink_cmd_insert(MemLink *m, char *key, char *value, int valuelen, char *maskstr, unsigned int pos);
int			memlink_cmd_update(MemLink *m, char *key, char *value, int valuelen, unsigned int pos);
int			memlink_cmd_mask(MemLink *m, char *key, char *value, int valuelen, char *maskstr);
int			memlink_cmd_tag(MemLink *m, char *key, char *value, int valuelen, int tag);
int			memlink_cmd_range(MemLink *m, char *key, char *maskstr, unsigned int frompos, unsigned int len);

#endif


