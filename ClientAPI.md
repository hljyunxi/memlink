# Introduction #

> 目前，memlink支持的客户端库，有c,python,php,java这四种。python，php，java客户端实际上是利用c客户端来做成不同语言的模块。因此c以外语言的客户端同样可以达到很高的性能。


# Details #

命令描述：

| 命令名 | 类型 | 描述 |
|:----------|:-------|:-------|
| dump | 管理 | 立即复制一份内存数据到磁盘 |
| clean | 管理 | 重排某个key下的列表 |
| stat | 管理 | 统计信息 |
| create | 写 | 创建key |
| del | 写 | 删除key下的某个value |
| del\_by\_mask | 写 | 删除list下符合此mask条件的所有value |
| insert | 写 | 在key下的列表中插入一条 |
| move | 写 | 更新key下列表中某value在列表中的位置 |
| mask | 写 | 修改某个value的mask信息 |
| tag | 写 | 标记删除列表中的某个value或者恢复某个value |
| rmkey | 写 | 删除一个key，包括它的列表 |
| range | 读 | 获取指定key下的列表中的某个范围的value |
| count | 读 | 获取指定key下列表的条数 |




## C/C++ ##

  * `MemLink* memlink_create(char *host, int readport, int writeport, int timeout);`<br />
> 创建MemLink数据结构<br />
> host为memlink  server的ip，readport为server端的读端口，writeport为server端的写端口，timeout为网络操作的超时时间。单位为秒。<br />
> 错误返回NULL，否则为成功

  * `void memlink_destroy(MemLink *m);`<br />
> 销毁MemLink数据结构，释放资源

  * `void memlink_close(MemLink *m);`<br />
> 关闭MemLink中的网络连接

  * `int memlink_cmd_dump(MemLink *m);`<br />
> 发送DUMP命令。memlink服务器端执行dump操作。<br />
> 返回MEMLINK\_OK表示执行成功，否则出错

  * `int memlink_cmd_clean(MemLink *m, char *key);`<br />
> 发送CLEAN命令。memlink会对该key下的列表进行重新排列，以去除已真实删除的数据空洞。

  * `int memlink_cmd_stat(MemLink *m, char *key, MemLinkStat *stat);`<br />
> 发送STAT命令。获取对应key的一些统计信息，结果存储到传入的参数stat中。
> MemLinkStat的结构如下：<br />

```
  typedef struct _memlink_stat
  {
      unsigned char   valuesize;
      unsigned char   masksize;
      unsigned int    blocks; // all blocks
      unsigned int    data;   // all alloc data item
      unsigned int    data_used; // all data item used
      unsigned int    mem;       // all alloc mem      
  }MemLinkStat; 
```

> 其中valuesize为value长度的字节数，masksize为mask长度的字节数。blocks为该key下有多少个大的数据块（这里称为block），data为这些所有block中可以容纳的value数，data\_used为实际存储了多少个value值。mem为所有block以及key一共占用的内存字节数。

  * `int memlink_cmd_create(MemLink *m, char *key, int valuelen, char *maskstr, unsigned char listtype, unsigned char valuetype);`<br />
> 发送CREATE命令，创建一个key。key为字符串，长度最长为255字节。参数valuelen表示该key下的value的长度，value在memlink内部处理为2进制格式，并且是定长的，也就是说memlink不关心它是什么类型的，只关心它的长度。value的最大长度为255。maskstr为value的属性格式。该属性格式由问号分隔的数字组成。这些数字表示对应属性项所占用的空间的bit数。比如4:3:1表示有三个属性，这三个属性分别占用4bit，3bit，1bit。属性的值必须为正整数，最大值为有符号4字节整型的最大值。支持的最大属性数量默认为16个。listtype为list类型，当前支持MEMLINK\_LIST, MEMLINK\_QUEUE, MEMLINK\_SORTLIST。分别为普通的列表，队列，按value排序的列表。valuetype仅在list类型为MEMLINK\_SORTLIST有效，表示list中value的类型，可以为MEMLINK\_VALUE\_INT(整型), MEMLINK\_VALUE\_UINT（无符号整型）, MEMLINK\_VALUE\_LONG（64位长整型）, MEMLINK\_VALUE\_ULONG（64位无符号长整型）, MEMLINK\_VALUE\_FLOAT（32位浮点）, MEMLINK\_VALUE\_DOUBLE（64位浮点）, MEMLINK\_VALUE\_STRING（字符串）, MEMLINK\_VALUE\_OBJ（二进制）

  * `int memlink_cmd_create_list(MemLink *m, char *key, int valuelen, char *maskstr);`<br />
> 发送CREATE命令，创建list。实际上是调用的memlink\_cmd\_create

  * `int memlink_cmd_create_queue(MemLink *m, char *key, int valuelen, char *maskstr);`<br />
> 发送CREATE命令，创建queue。实际上是调用的memlink\_cmd\_create

  * `int memlink_cmd_create_sortlist(MemLink *m, char *key, int valuelen, char *maskstr, unsigned char valuetype);`<br />
> 发送CREATE命令，创建排序list。实际上是调用的memlink\_cmd\_create

  * `int memlink_cmd_del(MemLink *m, char *key, char *value, int valuelen);`<br />
> 发送DEL命令。删除一个指定key下的对应value。

  * `int memlink_cmd_insert(MemLink *m, char *key, char *value, int valuelen,char *maskstr, unsigned int pos);`<br />
> 发送insert命令，在指定的key下插入一个value，且此value要插入到一个指定位置。value为定长的二进制格式。maskstr为该value的熟悉信息。格式和创建key时指定的属性格式对应。比如创建时属性格式为4:3:1，那么这里的maskstr也只能设置三项，比如maskstr可以为8:3:1。pos表示value要插入到列表中的哪个位置，0是列表头部，-1是列表尾部。注意，如果pos超出了列表的范围，也会被插入到尾部。

  * `int memlink_cmd_move(MemLink *m, char *key, char *value, int valuelen, unsigned int pos);`<br />
> 发送UPDATE命令，更新一个value的位置。pos为此value的新位置。

  * `int memlink_cmd_mask(MemLink *m, char *key, char *value, int valuelen, char *maskstr);`<br />
> 发送MASK命令，修改一个value的属性值。这里的maskstr的格式和INSERT命令的格式一样。

  * `int memlink_cmd_tag(MemLink *m, char *key, char *value, int valuelen, int tag);`<br />
> 发送TAG命令，对一个value进行标记删除或者恢复。标记删除的value不会被真实删除，只是该数据不再显示。tag值为MEMLINK\_TAG\_DEL表示标记删除，为MEMLINK\_TAG\_RESTORE表示恢复。

  * `int memlink_cmd_range(MemLink *m, char *key, unsigned char kind, char *maskstr,unsigned int frompos, unsigned int len,MemLinkResult *result);`<br />
> 发送RANGE命令，获取指定key下的某个范围的value。kind为查询value的内部属性，MEMLINK\_VALUE\_VISIBLE 表示正常可见数据，MEMLINK\_VALUE\_TAGDEL表示标记删除的数据，MEMLINK\_VALUE\_ALL表示前两者之和。frompos是获取的开始位置，从0开始，len为要获取的长度。结果将写入到result中。
> MemLinkResult的结构定义如下：
```
  typedef struct _memlink_result
  {
      int          count;
      int          valuesize;
      int          masksize;
      MemLinkItem  *root;
  }MemLinkResult;
```
> 其中count为返回的结果的条数，valuesize为value的长度(字节数)，masksize为mask的长度（字节数）。root为返回结果的链表。
> MemLinkItem的结构定义如下：
```
  typedef struct _memlink_item
  {
      struct _memlink_item    *next;
      char                    value[256];
      char                    mask[256];
  }MemLinkItem;
```
> 其中的next是链表的下一个节点指针，value和mask就是对应的结果值。

  * `int memlink_cmd_rmkey(MemLink *m, char *key);`<br />
> 发送RMKEY命令，删除一个key，以及该key下的所有value。

  * `int memlink_cmd_count(MemLink *m, char *key, char *maskstr, MemLinkCount *count);` <br />
> 发送COUNT命令，以maskstr为条件，统计一个key下所有可见的value，以及被标记删除的value的数量。结果写入到count中。MemLinkCount结构定义如下：
```
  typedef struct _memlink_count
  {
     unsigned int    visible_count;
     unsigned int    tagdel_count;
  }MemLinkCount;
```
> visible\_count为可见value的数量。tagdel\_count为标记删除的value的数量。

  * `void memlink_result_free(MemLinkResult *result);`<br />
> 释放MemLinkResult结构中的root项的内存。

## PHP ##

  * class MemLinkClient
> memlink客户端类。
    * construct($host, $readport, $writeport, $timeout)
> > 同c版memlink\_create。
    * close()
> > 同c版memlink\_close
    * destroy()
> > 同c版memlink\_destroy
    * create($key, $valuesize, $maskformat)
> > 同c版memlink\_cmd\_create。返回值为整型，正常返回MEMLNK\_OK。
    * dump()
> > 同c版memlink\_dump。返回值为整型，正常返回MEMLNK\_OK。
    * clean($key)
> > 同c版memlink\_cmd\_clean。返回值为整型，正常返回MEMLNK\_OK。
    * stat($key)
> > 同c版memlink\_cmd\_stat。返回MemLinkStat对象，失败返回null。
    * stat2($key, $mstat)
> > 同c版memlink\_cmd\_stat。和上一个stat方法的区别是，需要传一个MemLinkStat对象作为第二个参数，返回值为整型，正常返回MEMLNK\_OK。
    * delete($key, $value, $valuelen)
> > 同c版memlink\_cmd\_del。返回值为整型，正常返回MEMLNK\_OK。
    * insert($key, $value, $valuelen, $maskstr, $pos)
> > 同c版memlink\_cmd\_insert。返回值为整型，正常返回MEMLNK\_OK。
    * move($key, $value, $valuelen, $pos)
> > 同c版memlink\_cmd\_move。返回值为整型，正常返回MEMLNK\_OK。
    * mask($key, $value, $valulen, $maskstr)
> > 同c版memlink\_cmd\_mask。返回值为整型，正常返回MEMLNK\_OK。
    * tag($key, $value, $valuelen, $tag)
> > 同c版memlink\_cmd\_tag。返回值为整型，正常返回MEMLNK\_OK。
    * range($key, kind, $maskstr, $frompos, $len)
> > 同c版memlink\_cmd\_range。返回MemLinkResult对象，失败返回null。
    * range2($key, kind, $maskstr, $frompos, $len, $result)
> > 同c版memlink\_cmd\_range。和上一个range方法的区别是，需要传一个MemLinkResult对象作为第二个参数，返回值为整型，正常返回MEMLNK\_OK。
    * rmkey($key)
> > 同c版memlink\_cmd\_rmkey。返回值为整型，正常返回MEMLNK\_OK。
    * count($key, $maskstr)
> > 同c版memlink\_cmd\_count。返回MemLinkCount对象，失败返回null。
    * count2($key, $maskstr, $count)
> > 同c版memlink\_cmd\_count2。和上一个count方法的区别是，需要传一个MemLinkCount对象作为第二个参数，返回值为整型，正常返回MEMLNK\_OK。


## Python ##

  * class MemLinkClient

> memlink客户端类
    * init(self, host, readport, writeport, timeout)
> > 同c版memlink\_create。
    * close(self)
> > 同c版memlink\_close
    * destroy(self)
> > 同c版memlink\_destroy
    * dump(self)
> > 同c版memlink\_dump。返回值为整型，正常返回MEMLNK\_OK。
    * clean(self, key)
> > 同c版memlink\_cmd\_clean。返回值为整型，正常返回MEMLNK\_OK。
    * create(self, key, valuesize, maskstr, listtype, valuetype)
> > 同c版memlink\_cmd\_create。返回值为整型，正常返回MEMLNK\_OK。
    * create\_list(self, key, valuesize, maskstr)
> > 同c版memlink\_cmd\_create\_list。返回值为整型，正常返回MEMLNK\_OK。
    * create\_queue(self, key, valuesize, maskstr)
> > 同c版memlink\_cmd\_create\_queue。返回值为整型，正常返回MEMLNK\_OK。
    * create\_sortlist(self, key, valuesize, maskstr, valuetype)
> > 同c版memlink\_cmd\_create\_sortlist。返回值为整型，正常返回MEMLNK\_OK。
    * stat(self, key)
> > 同c版memlink\_cmd\_stat。返回值为tuple,共两个成员。其中第一项为整形返回值，为MEMLINK\_OK表示正常。第二项为MemLinkStat对象。
    * delete(self, key, value)
> > 同c版memlink\_cmd\_delete。返回值为整型，正常返回MEMLNK\_OK。
    * insert(self, key, value, maskstr, pos)
> > 同c版memlink\_cmd\_insert。返回值为整型，正常返回MEMLNK\_OK。
    * move(self, key, value, pos)
> > 同c版memlink\_cmd\_move。返回值为整型，正常返回MEMLNK\_OK。
    * mask(self, key, value, maskstr)
> > 同c版memlink\_cmd\_mask。返回值为整型，正常返回MEMLNK\_OK。
    * tag(self, key, value, tag)
> > 同c版memlink\_cmd\_tag。返回值为整型，正常返回MEMLNK\_OK。
    * range(self, key, kind, maskstr, frompos, len)
> > 同c版memlink\_cmd\_range。返回值为tuple,共两个成员。其中第一项为整形返回值，为MEMLINK\_OK表示正常。第二项为MemLinkResult对象。
    * rmkey(self, key)
> > 同c版memlink\_cmd\_rmkey。返回值为整型，正常返回MEMLNK\_OK。
    * count(self, key, maskstr)
> > 同c版memlink\_cmd\_count。返回值为tuple,共两个成员。其中第一项为整形返回值，为MEMLINK\_OK表示正常。第二项为MemLinkCount对象。

## Java ##

  * public class MemLinkClient

> memlink客户端类。说明和php版本类似。
    * public MemLinkClient (String host, int readport, int writeport, int timeout)
    * public void close()
    * public void destroy()
    * public int create(String key, int valuesize, String maskstr, int listtype, int valuetype)
    * public int create\_list(String key, int valuesize, String maskstr)
    * public int create\_queue(String key, int valuesize, String maskstr)
    * public int create\_sortlist(String key, int valuesize, String maskstr, int valuetype)
    * public int dump()
    * public int clean(String key)
    * public MemLinkStat stat(String key)
    * public int stat2(String key, MemLinkStat stat)
    * public int delete(String key, String value)
    * public int insert(String key, String value, String maskstr, int pos)
    * public int move(String key, String value, int pos)
    * public int mask(String key, String value, String maskstr)
    * public int tag(String key, String value, int tag)
    * public MemLinkResult range(String key, String maskstr, int frompos, int len)
    * public int range2(String key, String maskstr, int frompos, int len, MemLinkResult mresult)
    * public int rmkey(String key)
    * public MemLinkCount count(String key, String maskstr)
    * public int count2(String key, String maskstr, MemLinkCount mcount)



