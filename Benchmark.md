# Introduction #

Memlink用于存储Key=>List数据，下面对比了不同数据存储引擎存储存储Key=>List数据的性能和内存开销。压力测试分为：客户端有长连接和短链接、N个并发数执行M个查询、在不同数据规模下等多种组合测试。


# Details #
硬件

OS：CentOS release 4.6 (Final)
内核：2.6.9-67.0.22.ELsmp 32位<br />
内存：4G <br />
CPU：Intel(R) Xeon(R) CPU E5405  @ 2.00GHz （四核）<br />
硬盘：250G SATA <br />


数据模型
```
CREATE TABLE `ThreadList` (
  `forumid` int(11) NOT NULL,
  `threadid` char(12) NOT NULL,
  `status` bit(1) DEFAULT 0,
  `reply_time` datetime NOT NULL,
  KEY `threadlist` (`forumid`,`reply_time`,`status`)
) ENGINE=Innodb DEFAULT CHARSET=utf8;
```

<font color='red'><b>memlink</b></font> <br />
c:表示c客户端每秒操作成功条数，py:表示python客户端每秒操作成功条数，php:为php客户端每秒操作成功条数。mem:表示memlink server消耗内存。
insert为插入操作，range为获取列表某个范围的操作。插入的列表中的数据每条为12字节。

<font color='green'><b>redis</b></font><br />
redis测试一样每条数据是12字节。redis只测试c客户端。这里有两组测试，一组使用hiredis客户端，另一组使用redis自带的redis-benchmark。用LRANGE命令获取列表，用LPUSH向队列插入数据。redis是默认配置。

<font color='blue'><b>mysql</b></font><br />
mysql使用上面的数据库表结构。插入语句为：insert into ThreadList values (1, 'xxxx', 0, now()) 查询列表的语句为：select threadid from  ThreadList where forumid=1 order by reply\_time limit frompos,len

下面的测试结果中，<font color='red'>红色为memlink</font>，<font color='green'>绿色为redis</font>，<font color='blue'>蓝色为mysql</font>。

## 内存消耗测试 ##
| |**初始内存** | **1w** | **10w** | **100w** | **1000w** |
|:|:----------------|:-------|:--------|:---------|:----------|
| memlink | 4764K | 5032K | 6628K | 18M | 139M |
| redis | 1060K | 1920K | 9044K | 78M | 771M |

## 访问性能测试 ##
**1. 一个客户端，长连接**
<table border='1'><tr><td><img src='http://memlink.googlecode.com/svn/trunk/doc/figures/1.png' /></td><td></td><td><b>1w</b></td><td><b>10w</b></td><td><b>100w</b></td><td><b>1000w</b></td></tr>
<tr><td>insert</td><td><font color='red'>memlink</font></td><td><font color='red'>c:16296<br /> py:12558<br /> php:12153<br /></font></td><td><font color='red'>c:14125<br /> py:12565<br /> php:12144<br /></font></td><td><font color='red'>c:13868<br /> py: 13096<br /> php: 12521<br /></font></td><td><font color='red'>c:13187<br /> py:12611<br /> php:12124<br /></font></td></tr>
<tr><td><font color='green'>hiredis</font></td><td><font color='green'>15183<br /></font></td><td><font color='green'>14997<br /></font></td><td><font color='green'>14828<br /></font></td><td><font color='green'>14788<br /></font></td></tr>
<tr><td><font color='green'>redis</font></td><td><font color='green'>15673</font></td><td><font color='green'>15743</font></td><td><font color='green'>15594</font></td><td><font color='green'>15304</font></td></tr>
<tr><td><font color='blue'>mysql</font></td><td><font color='blue'>10891</font></td><td><font color='blue'>10297</font></td><td><font color='blue'>10022</font></td><td><font color='blue'>9718</font></td></tr>
<tr><td>range first100</td><td><font color='red'>memlink</font></td><td><font color='red'>c:10874<br /> py:8275<br /> php:9185</font></td><td><font color='red'>c:11429<br /> py:8273<br /> php:9171</font></td><td><font color='red'>c:10994<br /> py:8068<br /> php:9136</font></td><td><font color='red'>c:11663<br /> py:8256<br /> php:10071</font></td></tr>
<tr><td><font color='green'>hiredis</font></td><td><font color='green'>1328</font></td><td><font color='green'>1329</font></td><td><font color='green'>1324</font></td><td><font color='green'>1332</font></td></tr>
<tr><td><font color='green'>redis</font></td><td><font color='green'>4237</font></td><td><font color='green'>4587</font></td><td><font color='green'>4310</font></td><td><font color='green'>4347</font></td></tr>
<tr><td><font color='blue'>mysql</font></td><td><font color='blue'>1550</font></td><td><font color='blue'>1563</font></td><td><font color='blue'>1559</font></td><td><font color='blue'>1307</font></td></tr>
<tr><td>range first200</td><td><font color='red'>memlink</font></td><td><font color='red'>c:8045<br /> py:6015<br /> php:6544</font></td><td><font color='red'>c:8041<br /> py:6017<br /> php:6545</font></td><td><font color='red'>c:7863<br /> py:5941<br /> php:6542</font></td><td><font color='red'>c:7970<br /> py:6024<br /> php:7010</font></td></tr>
<tr><td><font color='green'>hiredis</font></td><td><font color='green'>708</font></td><td><font color='green'>709</font></td><td><font color='green'>705</font></td><td><font color='green'>709</font></td></tr>
<tr><td><font color='green'>redis</font></td><td><font color='green'>2631</font></td><td><font color='green'>2777</font></td><td><font color='green'>2666</font></td><td><font color='green'>2631</font></td></tr>
<tr><td><font color='blue'>mysql</font></td><td><font color='blue'>956</font></td><td><font color='blue'>954</font></td><td><font color='blue'>956</font></td><td><font color='blue'>941</font></td></tr>
<tr><td>range first1000</td><td><font color='red'>memlink</font></td><td><font color='red'>c:1362<br /> py:1446<br /> php:1466</font></td><td><font color='red'>c:1362<br /> py:1448<br /> php:1468</font></td><td><font color='red'>c:1354<br /> py:1440<br /> php:1462</font></td><td><font color='red'>c:1362<br /> py:1444<br /> php:1492</font></td></tr>
<tr><td><font color='green'>hiredis</font></td><td><font color='green'>147</font></td><td><font color='green'>148</font></td><td><font color='green'>148</font></td><td><font color='green'>147</font></td></tr>
<tr><td><font color='green'>redis</font></td><td><font color='green'>646</font></td><td><font color='green'>666</font></td><td><font color='green'>652</font></td><td><font color='green'>636</font></td></tr>
<tr><td><font color='blue'>mysql</font></td><td><font color='blue'>230</font></td><td><font color='blue'>230</font></td><td><font color='blue'>230</font></td><td><font color='blue'>235</font></td></tr>
<tr><td>range last100</td><td><font color='red'>memlink</font></td><td><font color='red'>c:11153<br /> py:8137<br /> php:8996</font></td><td><font color='red'>c:10318<br /> py:7526<br /> php:8263</font></td><td><font color='red'>c:5519<br /> py:4410<br /> php:4691</font></td><td><font color='red'>c:112<br /> py:97<br /> php:97</font></td></tr>
<tr><td><font color='green'>hiredis</font></td><td><font color='green'>1221</font></td><td><font color='green'>212</font></td><td><font color='green'>19</font></td><td><font color='green'>2</font></td></tr>
<tr><td><font color='green'>redis</font></td><td><font color='green'>3289</font></td><td><font color='green'>197</font></td><td><font color='green'>19</font></td><td><font color='green'>1.97</font></td></tr>
<tr><td><font color='blue'>mysql</font></td><td><font color='blue'>31</font></td><td><font color='blue'>3</font></td><td><font color='blue'>0.3</font></td><td><font color='blue'>0.04</font></td></tr>
<tr><td>range last200</td><td><font color='red'>memlink</font></td><td><font color='red'>c:7938<br /> py:5956<br /> php:6461</font></td><td><font color='red'>c:7515<br /> py:5629<br /> php:6078</font></td><td><font color='red'>c:4615<br /> py:3688<br /> php:3899</font></td><td><font color='red'>c:111<br /> py:96<br /> php:97</font></td></tr>
<tr><td><font color='green'>hiredis</font></td><td><font color='green'>675</font></td><td><font color='green'>184</font></td><td><font color='green'>19</font></td><td><font color='green'>2</font></td></tr>
<tr><td><font color='green'>redis</font></td><td><font color='green'>2237</font></td><td><font color='green'>188</font></td><td><font color='green'>19</font></td><td><font color='green'>1.97</font></td></tr>
<tr><td><font color='blue'>mysql</font></td><td><font color='blue'>31</font></td><td><font color='blue'>3</font></td><td><font color='blue'>0.3</font></td><td><font color='blue'>0.04</font></td></tr>
<tr><td>range last1000</td><td><font color='red'>memlink</font></td><td><font color='red'>c:1358<br /> py:1442<br /> php:1458</font></td><td><font color='red'>c:1345<br /> py:1421<br /> php:1440</font></td><td><font color='red'>c:1209<br /> py:1251<br /> php:1272</font></td><td><font color='red'>c:103<br /> py:91<br /> php:91</font></td></tr>
<tr><td><font color='green'>hiredis</font></td><td><font color='green'>147</font></td><td><font color='green'>92</font></td><td><font color='green'>17</font></td><td><font color='green'>2</font></td></tr>
<tr><td><font color='green'>redis</font></td><td><font color='green'>615</font></td><td><font color='green'>143</font></td><td><font color='green'>19</font></td><td><font color='green'>1.97</font></td></tr>
<tr><td><font color='blue'>mysql</font></td><td><font color='blue'>30</font></td><td><font color='blue'>3</font></td><td><font color='blue'>0.3</font></td><td><font color='blue'>0.03</font></td></tr>
</table>


**2. 一个客户端， 全部是短连接。一次请求一个连接。**

<table border='1'><tr><td><img src='http://memlink.googlecode.com/svn/trunk/doc/figures/1.png' /></td><td></td><td><b>1w</b></td><td><b>10w</b></td><td><b>100w</b></td><td><b>1000w</b></td></tr>
<tr><td>insert</td><td><font color='red'>memlink</font></td><td><font color='red'>c:6560<br /> py:6138<br /> php:6064<br /></font></td><td><font color='red'>c:6466<br /> py:5990<br /> php:5986<br /></font></td><td><font color='red'>c:6675<br /> py:6092<br /> php:6010<br /></font></td><td><font color='red'>c:6719<br /> py:6012<br /> php:6003<br /></font></td></tr>
<tr><td><font color='green'>hiredis</font></td><td><font color='green'>10891</font></td><td><font color='green'>10297</font></td><td><font color='green'>10233</font></td><td><font color='green'>10135</font></td></tr>
<tr><td><font color='green'>redis</font></td><td><font color='green'>6134</font></td><td><font color='green'>5855</font></td><td><font color='green'>6072</font></td><td><font color='green'>5995</font></td></tr>
<tr><td><font color='blue'>mysql</font></td><td><font color='blue'>3313</font></td><td><font color='blue'>3251</font></td><td><font color='blue'>3108</font></td><td><font color='blue'>3001</font></td></tr>
<tr><td>range first100</td><td><font color='red'>memlink</font></td><td><font color='red'>c:5467<br /> py:4535<br /> php:5064</font></td><td><font color='red'>c:5472<br /> py:4450<br /> php:4944</font></td><td><font color='red'>c:5662<br /> py:4495<br /> php:4712</font></td><td><font color='red'>c:5484<br /> py:4475<br /> php:4779</font></td></tr>
<tr><td><font color='green'>hiredis</font></td><td><font color='green'>1058</font></td><td><font color='green'>1172</font></td><td><font color='green'>1166</font></td><td><font color='green'>1163</font></td></tr>
<tr><td><font color='green'>redis</font></td><td><font color='green'>3058</font></td><td><font color='green'>3058</font></td><td><font color='green'>3125</font></td><td><font color='green'>3333</font></td></tr>
<tr><td><font color='blue'>mysql</font></td><td><font color='blue'>923</font></td><td><font color='blue'>1158</font></td><td><font color='blue'>1031</font></td><td><font color='blue'>885</font></td></tr>
<tr><td>range first200</td><td><font color='red'>memlink</font></td><td><font color='red'>c:4533<br /> py:3746<br /> php:4124</font></td><td><font color='red'>c:4563<br /> py:3693<br /> php:4049</font></td><td><font color='red'>c:4709<br /> py:3724<br /> php:3893</font></td><td><font color='red'>c:4567<br /> py:3708<br /> php:3931</font></td></tr>
<tr><td><font color='green'>hiredis</font></td><td><font color='green'>654</font></td><td><font color='green'>651</font></td><td><font color='green'>669</font></td><td><font color='green'>667</font></td></tr>
<tr><td><font color='green'>redis</font></td><td><font color='green'>2150</font></td><td><font color='green'>2159</font></td><td><font color='green'>2222</font></td><td><font color='green'>2272</font></td></tr>
<tr><td><font color='blue'>mysql</font></td><td><font color='blue'>815</font></td><td><font color='blue'>823</font></td><td><font color='blue'>783</font></td><td><font color='blue'>743</font></td></tr>
<tr><td>range first1000</td><td><font color='red'>memlink</font></td><td><font color='red'>c:1206<br /> py:1254<br /> php:1291</font></td><td><font color='red'>c:1200<br /> py:1249<br /> php:1285</font></td><td><font color='red'>c:1215<br /> py:1253<br /> php:1263</font></td><td><font color='red'>c:1202<br /> py:1250<br /> php:1266</font></td></tr>
<tr><td><font color='green'>hiredis</font></td><td><font color='green'>146</font></td><td><font color='green'>145</font></td><td><font color='green'>145</font></td><td><font color='green'>144</font></td></tr>
<tr><td><font color='green'>redis</font></td><td><font color='green'>626</font></td><td><font color='green'>627</font></td><td><font color='green'>636</font></td><td><font color='green'>641</font></td></tr>
<tr><td><font color='blue'>mysql</font></td><td><font color='blue'>227</font></td><td><font color='blue'>230</font></td><td><font color='blue'>226</font></td><td><font color='blue'>212</font></td></tr>
<tr><td>range last100</td><td><font color='red'>memlink</font></td><td><font color='red'>c:5468<br /> py:4451<br /> php:4970</font></td><td><font color='red'>c:5229<br /> py:4233<br /> php:4692</font></td><td><font color='red'>c:3763<br /> py:3055<br /> php:3155</font></td><td><font color='red'>c:110<br /> py:96<br /> php:96</font></td></tr>
<tr><td><font color='green'>hiredis</font></td><td><font color='green'>994</font></td><td><font color='green'>271</font></td><td><font color='green'>32</font></td><td><font color='green'>3</font></td></tr>
<tr><td><font color='green'>redis</font></td><td><font color='green'>2531</font></td><td><font color='green'>189</font></td><td><font color='green'>19</font></td><td><font color='green'>1.97</font></td></tr>
<tr><td><font color='blue'>mysql</font></td><td><font color='blue'>20</font></td><td><font color='blue'>3</font></td><td><font color='blue'>0.21</font></td><td><font color='blue'>0.02</font></td></tr>
<tr><td>range last200</td><td><font color='red'>memlink</font></td><td><font color='red'>c:4558<br /> py:3692<br /> php:4053</font></td><td><font color='red'>c:4355<br /> py:3545<br /> php:3886</font></td><td><font color='red'>c:3323<br /> py:2680<br /> php:2790</font></td><td><font color='red'>c:110<br /> py:95<br /> php:96</font></td></tr>
<tr><td><font color='green'>hiredis</font></td><td><font color='green'>630</font></td><td><font color='green'>233</font></td><td><font color='green'>32</font></td><td><font color='green'>3</font></td></tr>
<tr><td><font color='green'>redis</font></td><td><font color='green'>1879</font></td><td><font color='green'>182</font></td><td><font color='green'>19</font></td><td><font color='green'>1.97</font></td></tr>
<tr><td><font color='blue'>mysql</font></td><td><font color='blue'>30</font></td><td><font color='blue'>3</font></td><td><font color='blue'>0.3</font></td><td><font color='blue'>0.04</font></td></tr>
<tr><td>range last1000</td><td><font color='red'>memlink</font></td><td><font color='red'>c:1198<br /> py:1247<br /> php:1284</font></td><td><font color='red'>c:1191<br /> py:1226<br /> php:1267</font></td><td><font color='red'>c:1096<br /> py:1093<br /> php:1125</font></td><td><font color='red'>c:102<br /> py:91<br /> php:91</font></td></tr>
<tr><td><font color='green'>hiredis</font></td><td><font color='green'>144</font></td><td><font color='green'>104</font></td><td><font color='green'>27</font></td><td><font color='green'>3</font></td></tr>
<tr><td><font color='green'>redis</font></td><td><font color='green'>602</font></td><td><font color='green'>141</font></td><td><font color='green'>19</font></td><td><font color='green'>1.96</font></td></tr>
<tr><td><font color='blue'>mysql</font></td><td><font color='blue'>30</font></td><td><font color='blue'>3</font></td><td><font color='blue'>0.3</font></td><td><font color='blue'>0.03</font></td></tr>
</table>



**3. 10个客户端，并发长连接。**

memlink内部开启4个处理线程。仅c客户端测试。
<table border='1'><tr><td><img src='http://memlink.googlecode.com/svn/trunk/doc/figures/1.png' /></td><td></td><td><b>1w</b></td><td><b>10w</b></td><td><b>100w</b></td><td><b>1000w</b></td></tr>
<tr><td>insert</td><td><font color='red'>memlink</font></td><td><font color='red'>23021</font></td><td><font color='red'>22210</font></td><td><font color='red'>22200</font></td><td><font color='red'>22242</font></td></tr>
<tr><td><font color='green'>hiredis</font></td><td><font color='green'>29401</font></td><td><font color='green'>30929</font></td><td><font color='green'>29109</font></td><td><font color='green'>30278</font></td></tr>
<tr><td><font color='green'>redis</font></td><td><font color='green'>32571</font></td><td><font color='green'>33692</font></td><td><font color='green'>33665</font></td><td><font color='green'>32945</font></td></tr>
<tr><td><font color='blue'>mysql</font></td><td><font color='blue'>13704</font></td><td><font color='blue'>12786</font></td><td><font color='blue'>12087</font></td><td><font color='blue'>11023</font></td></tr>
<tr><td>range first100</td><td><font color='red'>memlink</font></td><td><font color='red'>45451</font></td><td><font color='red'>53463</font></td><td><font color='red'>55645</font></td><td><font color='red'>48283</font></td></tr>
<tr><td><font color='green'>hiredis</font></td><td><font color='green'>1443</font></td><td><font color='green'>1384</font></td><td><font color='green'>1396</font></td><td><font color='green'>1339</font></td></tr>
<tr><td><font color='green'>redis</font></td><td><font color='green'>7042</font></td><td><font color='green'>7142</font></td><td><font color='green'>7092</font></td><td><font color='green'>6666</font></td></tr>
<tr><td><font color='blue'>mysql</font></td><td><font color='blue'>2011</font></td><td><font color='blue'>2059</font></td><td><font color='blue'>2015</font></td><td><font color='blue'>2387</font></td></tr>
<tr><td>range first200</td><td><font color='red'>memlink</font></td><td><font color='red'>23768</font></td><td><font color='red'>33966</font></td><td><font color='red'>35045</font></td><td><font color='red'>35417</font></td></tr>
<tr><td><font color='green'>hiredis</font></td><td><font color='green'>667</font></td><td><font color='green'>641</font></td><td><font color='green'>636</font></td><td><font color='green'>655</font></td></tr>
<tr><td><font color='green'>redis</font></td><td><font color='green'>3816</font></td><td><font color='green'>3816</font></td><td><font color='green'>3846</font></td><td><font color='green'>3571</font></td></tr>
<tr><td><font color='blue'>mysql</font></td><td><font color='blue'>1637</font></td><td><font color='blue'>1768</font></td><td><font color='blue'>1854</font></td><td><font color='blue'>1614</font></td></tr>
<tr><td>range first1000</td><td><font color='red'>memlink</font></td><td><font color='red'>4269</font></td><td><font color='red'>4305</font></td><td><font color='red'>4281</font></td><td><font color='red'>4271</font></td></tr>
<tr><td><font color='green'>hiredis</font></td><td><font color='green'>114</font></td><td><font color='green'>115</font></td><td><font color='green'>114</font></td><td><font color='green'>115</font></td></tr>
<tr><td><font color='green'>redis</font></td><td><font color='green'>811</font></td><td><font color='green'>812</font></td><td><font color='green'>815</font></td><td><font color='green'>781</font></td></tr>
<tr><td><font color='blue'>mysql</font></td><td><font color='blue'>654</font></td><td><font color='blue'>641</font></td><td><font color='blue'>648</font></td><td><font color='blue'>631</font></td></tr>
<tr><td>range last100</td><td><font color='red'>memlink</font></td><td><font color='red'>53719</font></td><td><font color='red'>48948</font></td><td><font color='red'>24086</font></td><td><font color='red'>295</font></td></tr>
<tr><td><font color='green'>hiredis</font></td><td><font color='green'>1584</font></td><td><font color='green'>229</font></td><td><font color='green'>19</font></td><td><font color='green'>2</font></td></tr>
<tr><td><font color='green'>redis</font></td><td><font color='green'>4694</font></td><td><font color='green'>197</font></td><td><font color='green'>19</font></td><td><font color='green'>1.82</font></td></tr>
<tr><td><font color='blue'>mysql</font></td><td><font color='blue'>91</font></td><td><font color='blue'>8</font></td><td><font color='blue'>1</font></td><td><font color='blue'>-</font></td></tr>
<tr><td>range last200</td><td><font color='red'>memlink</font></td><td><font color='red'>35593</font></td><td><font color='red'>34264</font></td><td><font color='red'>21830</font></td><td><font color='red'>292</font></td></tr>
<tr><td><font color='green'>hiredis</font></td><td><font color='green'>696</font></td><td><font color='green'>220</font></td><td><font color='green'>19</font></td><td><font color='green'>2</font></td></tr>
<tr><td><font color='green'>redis</font></td><td><font color='green'>3039</font></td><td><font color='green'>188</font></td><td><font color='green'>19</font></td><td><font color='green'>1.75</font></td></tr>
<tr><td><font color='blue'>mysql</font></td><td><font color='blue'>96</font></td><td><font color='blue'>9</font></td><td><font color='blue'>1</font></td><td><font color='blue'>-</font></td></tr>
<tr><td>range last1000</td><td><font color='red'>memlink</font></td><td><font color='red'>4285</font></td><td><font color='red'>4270</font></td><td><font color='red'>4336</font></td><td><font color='red'>277</font></td></tr>
<tr><td><font color='green'>hiredis</font></td><td><font color='green'>114</font></td><td><font color='green'>142</font></td><td><font color='green'>19</font></td><td><font color='green'>2</font></td></tr>
<tr><td><font color='green'>redis</font></td><td><font color='green'>772</font></td><td><font color='green'>149</font></td><td><font color='green'>18</font></td><td><font color='green'>1.75</font></td></tr>
<tr><td><font color='blue'>mysql</font></td><td><font color='blue'>91</font></td><td><font color='blue'>8</font></td><td><font color='blue'>1</font></td><td><font color='blue'>-</font></td></tr>
</table>




上面的 - 表示时间太长，数百秒也没有结果。

**4. 10个客户端，并发短连接**

Memlink内部开启4个处理线程。仅c客户端测试。
<table border='1'><tr><td><b>操作</b></td><td></td><td><b>1w</b></td><td><b>10w</b></td><td><b>100w</b></td><td><b>1000w</b></td></tr>
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



# FAQ #

  1. **为什么memlink的读速度会比redis快？**
> > 这个是内部存储数据结构决定的。redis内部是一个链表。memlink内部同样是链表，不同的是memlink把100个value组织到了一个链表数据块内，这样循环查找位置的时候，memlink理论上最好情况，循环次数比redis少100倍。
  1. **为什么在多线程并发的情况下memlink比redis读速度快很多？**
> > 因为memlink读数据是完全多线程的，并且没有同步锁，可以利用到多核。而redis在读取数据上是单线程的。