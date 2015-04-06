# 主从复制功能设计 #
## 主从复制功能 ##
  * 一主多从 √
  * 从又可为其他节点的主 √
  * 主从切换 √
  * 互为主从（单实例支持多表情况下，互为主从，memlink为单实例单表，没有这种需求）×

## 主从复制原理 ##
参照Mysql Replication原理：http://forge.mysql.com/wiki/MySQL_Internals_Replication
具体如下：
  * Slave上IO线程连接上Master，并请求从指定日志文件的指定位置（或者从最开始的日志）之后的日志内容，请求包含2个参数bin-log-name和pos;
  * Master接收到来自Slave的IO进程的请求后，通过负责复制的IO线程根据请求信息读取制定日志指定位置之后的日志信息，返回给Slave 的IO进程。返回信息中除了日志所包含的信息之外，还包括本次返回的信息已经到Master端的bin-log文件的名称以及bin-log的位置；
  * Slave的IO进程接收到命令后，运行该命令，并将该命令记录追加在Slave端的bin-log文件的最末端，然后将Master端的 bin-log的文件名和位置记录到master-info文件中，以便在下一次读取的时候能够清楚的高速Master“我需要从某个bin-log的哪个位置开始往后的日志内容，请发给我”；

先记录bin-log，再记录master-info，并非是一个原子操作，最坏情况是重复执行了一条命令。见Mysql rpl\_mi.cc<br />
We flushed the relay log BEFORE the master.info file, because if we crash now, we will get a duplicate event in the relay log at restart. If we flushed in the other order, we would get a hole in the relay log. And duplicate is better than hole (with a duplicate, in later versions we can add detection and scrap one event; with a hole there's nothing we can do).

## Master-info中信息 ##
<pre>
Master-bin-log-name<br>
Bin-log-pos<br>
Master ip<br>
Master user<br>
Master password<br>
</pre>