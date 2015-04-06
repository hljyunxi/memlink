# 编译和安装介绍 #
> 本文档着重介绍memlink的安装和编译，描述memlink编译过程中依赖的一些库，编译客户端需要的一些模块。方便用户能够方便快捷的使用memlink。

## 操作系统平台 ##
> 目前支持的操作系统是linux， 内核版本需要2.6以上，64bit， 32bit的系统都可以。

## 编译环境 ##
  * 依赖python http://www.python.org/ 版本2.6
  * 依赖scons http://www.scons.org/ 版本为2.0.1

> scons是个python模块。安装方法为，解压缩后，进入该目录，运行：
```
 python setup.py install
```
> 注意python的版本。

> scons相当于make。它依赖于python。它基本命令和make对比：<br />
> scons         => make<br />
> scons -c      => make clean<br />
> scons  安装路径 => make install<br />

## 编译memlink ##
  * 库依赖libevent http://monkey.org/~provos/libevent/ 版本1.4.14b

> 编译需要先安装libevent。

> 去google code上面check 一份代码， 进入memlink目录下， 直接运行scons，正常情况，在该目录下面会出现可执行文件memlink。

## 编译客户端模块 ##
> memlink的客户端能支持不同的语言， 包括C， PHP, Python, Java等，

> ## 编译C客户端 ##
> > 要编译C的客户端， 先进入client/c目录，直接运行scons，会生成libmemlink.so和libmemlink.a，分别为动态库和静态库。


> ## 编译安装python客户端模块 ##
    * 依赖python 2.6.x 开发库
> > > 如果是自己编译python，在configure的时候让它把库生成出来：
```
   ./configure --enable-shared
```
> > > 如果是用rpm包或者deb包安装的，需要安装python2.6-dev类似名字的开发包。
    * 依赖swig http://sourceforge.net/projects/swig/files/ 版本 swig-1.3.40，注意不是 swigwin-1.3.40。
> > > 有了这两个模块之后， 进入memlink/client/python目录， 直接运行scons,注意在该目录下面有一个SConstruct的文件，里面有个配置选项python\_config, 可能需要根据情况修改，这个是python2.6-config文件的路径.编译后生成memlink.so（前面有一个下划线）和memlink.py。这里需要把memlink.so, memlink.py, memlinkclient.py复制到python扩展模块路径中，或者是在当前路径下使用。


> ## 编译安装PHP客户端模块 ##
    * 依赖PHP 5.x 开发库
> > > 编译php只需要默认的configure, make, make install 就可以编译出能使用memlink的版本。
    * 依赖swig http://sourceforge.net/projects/swig/files/ 版本 swig-1.3.40，注意不是 swigwin-1.3.40。
> > > 进入memlink/client/php目录， 直接运行scons.该目录下面有一个叫SConstruct的文件， 里面有个配置选项叫php\_config的，这个是php-config文件的路径。编译生成的memlink.so拷贝到php扩展模块目录中。当前路径下需要memlink.php和memlinkclient.php。


> ## 编译安装Java客户端模块 ##
    * 需要jdk版本1.5以上
    * 依赖swig http://sourceforge.net/projects/swig/files/ 版本 swig-1.3.40，注意不是 swigwin-1.3.40。
> > 首先要设置java sdk路径。打开 client/java/SConstruct，修改里面的java\_home为jdk路径（和JAVA\_HOME一样的）。另外scons还要求jdk的javac、jar命令在/usr/bin下。如果不在可以做一个符号链接。
> > 编译直接在client/java目录下执行scons即可。会生成memlink.jar以及libmemlink.so
> > > 特别注意： 在使用java客户端时，任何memlink的对象（除MemLinkClient外，它是destroy()），必须调用delete()方法及时释放内存。

## memlink安装 ##

> memlink安装需要执行 "scons 安装路径" 这个命令。其中的安装路径在SConstruct文件中有配置，必须和里面的install\_dir一致。这里默认是/opt/memlink，可以根据情况修改。

> memlink安装的目录结构是这个样子的：<br />

> /memlink
> > /bin
> > > memlink

> > /etc
> > > memlink.conf

> > /data
> > > (bin.log, dump.dat这两种数据文件，运行时生成)

> > /log
> > > (memlink运行过程中产生的日志，运行时生成)

## memlink运行 ##

> memlink需要带一个参数，就是配置文件的路径。比如：
```
 memlink ../etc/memlink.conf
```
> 表示memlink的配置文件在 ../etc/memlink.conf

> 一些配置信息，这里有描述：<a href='http://code.google.com/p/memlink/wiki/DesignDocument#8_配置信息'>点击这里</a>