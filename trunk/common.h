#ifndef MEMLINK_COMMON_H
#define MEMLINK_COMMON_H

// 命令执行超时
#define MEMLINK_ERR_TIMEOUT         -1000
// 发送数据错误
#define MEMLINK_ERR_SEND            -100
// 接收数据错误
#define MEMLINK_ERR_RECV            -200
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

#endif
