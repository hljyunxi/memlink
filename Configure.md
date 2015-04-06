| 名称| 类型| 描述| 默认值|
|:------|:------|:------|:---------|
| block\_data\_count | 字符串 | 每个数据块包含的value个数。可以有1个或多个，多个用逗号分隔。如：20,10,5,2,1 | 50,20,10,5,1 |
| block\_data\_reduce | 浮点数 | 当数据块中有数据删除时，在一定的条件下要使用更小的数据块。此配置表示数据删除数目，达到比小一级的数据块还小一定的比率才更换小数据块|  0 |
| dump\_interval | 整型 | dump时间间隔，单位为分钟 | 60|
| block\_clean\_cond | 浮点数 | clean条件，空闲空间比率 | 0.5 |
| block\_clean\_start | 整型 | 数据块必须超过此配置数才能进行clean | 3 |
| block\_clean\_num | 整型| clean过程中，每多少块为一个阶段 | 100 |
| read\_port | 整型 | 读端口 | 11001 |
| write\_port | 整型 | 写端口 | 11002 |
| sync\_port | 整型 | 同步端口 | 11003 |
| data\_dir | 字符串| dump和binlog数据文件保存目录。最好为绝对路径 |  |
| log\_level | 整型 | 日志等级.0－4,0 无日志,1 只输出错误,2 只输出错误和警告,3 输出错误，警告，提示信息,4 所有| 3 |
| log\_name | 字符串 | 日志文件名。如果为字符串stdout，表示输出到标准输出 |  |
| timeout | 整型 | 网络操作超时时间 | 30 |
| thread\_num | 整型 | 读线程数 | 4 |
| write\_binlog | 布尔 | 是否写binlog。为yes/no | yes |
| max\_conn | 整型| 最大同时连接数 | 1000 |
| max\_read\_conn | 整型 | 最大读连接数 | 0 |
| max\_write\_conn | 整型 | 最大写连接数 | 0 |
| max\_sync\_conn | 整型 | 最大同步连接数 | 0 |
| max\_core | 整型 | 最大coredump文件大小 |  |
| is\_daemon | 布尔 | 是否为守护进程。为yes/no | yes |
| role | 字符串| 角色。可以为master/slave。master表示主，slave表示从 |  |
| master\_sync\_host | 字符串 | 主服务器ip |  |
| master\_sync\_port | 整型 | 主服务器同步端口 |  |
| sync\_interval | 整型 | 主服务器检查新数据的频率。单位为毫秒。| 60 |
| user | 字符串 | 进程用户。配置后进程将切换到该用户和组，不写不切换用户。|  |