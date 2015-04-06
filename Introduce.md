#Memlink介绍
# 为什么会有Memlink? #
对于大型论坛服务，比如百度贴吧、天涯论坛，日均发帖量过百万或千万，日均PV过亿，日积月累下来的帖子数量可能几十亿到上百亿。这种超级论坛，其海量存储、海量访问都是一个非常有挑战性的技术难题。

中小规模的论坛（Phpwind/discuz）通常使用mysql/sql server作为后端存储，当数据量膨胀时，比如一个版面有百万、千万级别主贴，一个主贴下有数百万回复，此时使用SQL语句select … order by … limit … 进行数据查询和展现，性能可想而知。

大型论坛中的数据可以抽象为如下三类数据模型：
  1. Key=>Value结构数据。比如：版面信息、主帖信息、回帖信息等。主贴id对应主贴的信息（标题、发帖时间、作者等等），回帖id对应回贴的信息（回复内容、回复时间、回复者等等）。
  1. Key=>List结构数据。比如：主贴列表，主贴列表按发表时间或者最后回复时间进行排序；回复列表等等。<br />Key=>List通常有如下特点：
    1. 可排序
    1. 可动态调整顺序
  1. 其他周边数据（SQL系统/检索系统）。比如：斑竹、会员、友情版面/链接等等，由于数据量较小、逻辑关系较复杂，可以使用sql系统存储；比如帖子检索可以使用检索系统。

对于Key=>Value系统，市面上有较多选择，它们的数据容量大小从数百万到上百亿不等，性能、功能也各有差异，由于讨论KV系统的文章很多，再次不赘述。

对于Key=>List系统，市面上可选择的余地非常小，加之线上工业级别的一些要求，经对比了一些Key=>List系统（Redis），最终选择开发memlink系统，同时开源出来，也希望为业界同行提供多一种选择，繁荣开源社区。


# Memlink简介 #

Memlink是一个高性能、持久化、分布式的Key=>List/Queue数据引擎。正如名称中的Memlink所示，所有数据都建构在内存中，保证了系统的高性能(读性能大约是Redis几倍到十倍)，精简内存（内存消耗大约是Redis的1/4），使用了redo-log技术保证数据的持久化。此外，Memlink还支持主从复制、读写分离、数据项过滤操作等功能。

特点：
  * 内存数据引擎，性能极为高效
  * List中的Node采用块链组织，精简内存，优化查找效率
  * Node数据项可定义Mask表，支持多种过滤操作
  * 支持redo-log，数据持久化，非Cache模式
  * 分布式，主从同步
  * 读写分离，写优先处理。

# 与Redis区别 #
Redis同样也提供key=>list 存储功能，Memlink与Redis区别有：
  * Redis比较消耗内存。每个存储节点，在不支持vm的情况下要额外消耗12字节内存，在支持vm的情况下，每个节点额外消耗24字节内存。对于存储上亿条数据来说，额外消耗的内存太大。
  * Redis redo-log不够完善。redis提供了两种redo-log机制，机制一：每隔一段时间同步磁盘（此期间重启会丢失部分数据）；机制二：追加log方式，会使log文件越来越膨胀，造成性能不优化（需采用额外命令减小log）。
  * 主从同步不完善。如果slaver因为某原因丢失了部分同步数据，需要重新完全获取一份主节点的所有数据。在大数据量的情况下，不太合适线上生产的需求。
  * 网络处理主事件循环只有一个线程，不能很好的利用多核；同时读写没有分离，没有进行写优先处理。
  * List中的Node没有mask表，不能进行一些属性过滤。

Memlink主要对上述特点进行了改进。

# 性能测试 #
硬件
  * CentOS release 4.6 (Final)
  * Kernel 2.6.9-67.0.22.ELsmp 32位
  * Memory 4G
  * CPU Intel(R) Xeon(R) CPU E5405  @ 2.00GHz （四核）
  * Disk 250G SATA <br />

客户端
  * 10个客户端，并发短连接
  * Memlink内部开启4个处理线程。
  * Redis只支持单线程模型。结果表格中的hiredis为使用hiredis客户端测试的结果。redis为官方的redis-benchmark测试结果。

<table border='1'><tr><td><img src='http://memlink.googlecode.com/svn/trunk/doc/figures/1.png' /> </td><td></td><td><b>1w</b></td><td><b>10w</b></td><td><b>100w</b></td><td><b>1000w</b></td></tr>
<tr><td>insert</td><td><font color='red'>memlink</font></td><td><font color='red'>9665</font></td><td><font color='red'>9650</font></td><td><font color='red'>10078</font></td><td><font color='red'>10183</font></td></tr>
<tr><td><font color='green'>hiredis</font></td><td><font color='green'>9381</font></td><td><font color='green'>9489</font></td><td><font color='green'>8993</font></td><td><font color='green'>8976</font></td></tr>
<tr><td><font color='green'>redis</font></td><td><font color='green'>9285</font></td><td><font color='green'>9290</font></td><td><font color='green'>9287</font></td><td><font color='green'>8835</font></td></tr>
<tr><td><font color='blue'>mysql</font></td><td><font color='blue'>5623</font></td><td><font color='blue'>5621</font></td><td><font color='blue'>5468</font></td><td><font color='blue'>5306</font></td></tr>
<tr><td>range first100</td><td><font color='red'>memlink</font></td><td><font color='red'>17400</font></td><td><font color='red'>17504</font></td><td><font color='red'>16614</font></td><td><font color='red'>17292</font></td></tr>
<tr><td><font color='green'>hiredis</font></td><td><font color='green'>1695</font></td><td><font color='green'>1637</font></td><td><font color='green'>1696</font></td><td><font color='green'>1586</font></td></tr>
<tr><td><font color='green'>redis</font></td><td><font color='green'>4629</font></td><td><font color='green'>4587</font></td><td><font color='green'>4504</font></td><td><font color='green'>4545</font></td></tr>
<tr><td><font color='blue'>mysql</font></td><td><font color='blue'>2210</font></td><td><font color='blue'>2286</font></td><td><font color='blue'>1955</font></td><td><font color='blue'>1611</font></td></tr>
<tr><td>range first200</td><td><font color='red'>memlink</font></td><td><font color='red'>15786</font></td><td><font color='red'>15772</font></td><td><font color='red'>15964</font></td><td><font color='red'>16180</font></td></tr>
<tr><td><font color='green'>hiredis</font></td><td><font color='green'>711</font></td><td><font color='green'>711</font></td><td><font color='green'>719</font></td><td><font color='green'>692</font></td></tr>
<tr><td><font color='green'>redis</font></td><td><font color='green'>2941</font></td><td><font color='green'>2949</font></td><td><font color='green'>2941</font></td><td><font color='green'>2857</font></td></tr>
<tr><td><font color='blue'>mysql</font></td><td><font color='blue'>1444</font></td><td><font color='blue'>1791</font></td><td><font color='blue'>1870</font></td><td><font color='blue'>1402</font></td></tr>
<tr><td>range first1000</td><td><font color='red'>memlink</font></td><td><font color='red'>3795</font></td><td><font color='red'>3918</font></td><td><font color='red'>3703</font></td><td><font color='red'>3250</font></td></tr>
<tr><td><font color='green'>hiredis</font></td><td><font color='green'>118</font></td><td><font color='green'>115</font></td><td><font color='green'>116</font></td><td><font color='green'>114</font></td></tr>
<tr><td><font color='green'>redis</font></td><td><font color='green'>761</font></td><td><font color='green'>739</font></td><td><font color='green'>761</font></td><td><font color='green'>735</font></td></tr>
<tr><td><font color='blue'>mysql</font></td><td><font color='blue'>550</font></td><td><font color='blue'>692</font></td><td><font color='blue'>620</font></td><td><font color='blue'>686</font></td></tr>
<tr><td>range last100</td><td><font color='red'>memlink</font></td><td><font color='red'>16989</font></td><td><font color='red'>16502</font></td><td><font color='red'>13118</font></td><td><font color='red'>319</font></td></tr>
<tr><td><font color='green'>hiredis</font></td><td><font color='green'>2132</font></td><td><font color='green'>240</font></td><td><font color='green'>20</font></td><td><font color='green'>2</font></td></tr>
<tr><td><font color='green'>redis</font></td><td><font color='green'>4385</font></td><td><font color='green'>191</font></td><td><font color='green'>19</font></td><td><font color='green'>2</font></td></tr>
<tr><td><font color='blue'>mysql</font></td><td><font color='blue'>80</font></td><td><font color='blue'>8</font></td><td><font color='blue'>1</font></td><td><font color='blue'>-</font></td></tr>
<tr><td>range last200</td><td><font color='red'>memlink</font></td><td><font color='red'>15915</font></td><td><font color='red'>15596</font></td><td><font color='red'>12203</font></td><td><font color='red'>316</font></td></tr>
<tr><td><font color='green'>hiredis</font></td><td><font color='green'>743</font></td><td><font color='green'>229</font></td><td><font color='green'>20</font></td><td><font color='green'>2</font></td></tr>
<tr><td><font color='green'>redis</font></td><td><font color='green'>2941</font></td><td><font color='green'>182</font></td><td><font color='green'>19</font></td><td><font color='green'>2</font></td></tr>
<tr><td><font color='blue'>mysql</font></td><td><font color='blue'>94</font></td><td><font color='blue'>9</font></td><td><font color='blue'>1</font></td><td><font color='blue'>-</font></td></tr>
<tr><td>range last1000</td><td><font color='red'>memlink</font></td><td><font color='red'>3893</font></td><td><font color='red'>3641</font></td><td><font color='red'>3332</font></td><td><font color='red'>299</font></td></tr>
<tr><td><font color='green'>hiredis</font></td><td><font color='green'>120</font></td><td><font color='green'>174</font></td><td><font color='green'>19</font></td><td><font color='green'>2</font></td></tr>
<tr><td><font color='green'>redis</font></td><td><font color='green'>756</font></td><td><font color='green'>149</font></td><td><font color='green'>18</font></td><td><font color='green'>2</font></td></tr>
<tr><td><font color='blue'>mysql</font></td><td><font color='blue'>94</font></td><td><font color='blue'>9</font></td><td><font color='blue'>1</font></td><td><font color='blue'>-</font></td></tr>
</table>

详细性能测试请见[Benchmark](Benchmark.md)

# Client API #
客户端命令描述：

| 命令名 | 类型 | 描述 |
|:----------|:-------|:-------|
| dump | 管理 | 立即复制一份内存数据到磁盘 |
| clean | 管理 | 重排某个key下的列表 |
| stat | 管理 | 统计信息 |
| create | 写 | 创建key |
| del | 写 | 删除key下的某个value |
| insert | 写 | 在key下的列表中插入一条 |
| update | 写 | 更新key下列表中某value在列表中的位置 |
| mask | 写 | 修改某个value的mask信息 |
| tag | 写 | 标记删除列表中的某个value或者恢复某个value |
| rmkey | 写 | 删除一个key，包括它的列表 |
| range | 读 | 获取指定key下的列表中的某个范围的value |
| count | 读 | 获取指定key下列表的条数 |

详细客户端API请见[ClientAPI](ClientAPI.md)

# 谁在使用? #
目前Memlink应用于天涯来吧、天涯论坛系统。
未来Memlink的Key=>Queue会应用在天涯微博系统上。

# 未来 #
Memlink是专注于Key => List/Queue对象的存储系统，它内存使用更精简、性能更高效。
Key => List/Queue系统作为Key => Value另一种形式补充，为高性能、海量数据的Web应用提供了新的数据存储模型选择。

# FAQ #
  1. **为什么Memlink的读速度会比Redis快？**
> > 这是内部存储数据结构决定的。Redis内部是一个链表。Memlink内部同样是链表，不同的是memlink把N个节点组织到了一个链表数据块内，这样循环查找位置的时候，Memlink理论上最好情况，循环次数比Redis少N倍(N可配置，默认为100)。
  1. **为什么在多线程并发的情况下Memlink比Redis读速度快很多？**
> > 因为Memlink读数据是完全多线程的，并且没有同步锁，可以利用到多核。而Redis在读取数据上是单核的。