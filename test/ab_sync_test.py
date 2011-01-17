#!/usr/bin/python
# coding: utf-8
import os, sys
home = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(os.path.join(home, "client/python"))
import time
import subprocess
#import memlinkclient
from memlinkclient import *

MASTER_READ_PORT  = 11011
MASTER_WRITE_PORT = 11012

SLAVE_READ_PORT  = 11021
SLAVE_WRITE_PORT = 11022

client2master = MemLinkClient('127.0.0.1', MASTER_READ_PORT, MASTER_WRITE_PORT, 30);
client2slave  = MemLinkClient('127.0.0.1', SLAVE_READ_PORT, SLAVE_WRITE_PORT, 30);

def test_result():
    key = 'haha000'
    m = MemLinkClient('127.0.0.1', READ_PORT, WRITE_PORT, 30);
    
    ret, stat = m.stat(key)
    if stat:
        if stat.data_used != 500:
            print 'stat data_used error!', stat
            return -3
        """if stat.data != 500:
            print 'stat data error!', stat
            return -3"""
    else:
        print 'stat error:', stat, ret
        return -3

    ret, result = m.range(key, MEMLINK_VALUE_VISIBLE, "", 0, 1000)
    if not result or result.count != 250:
        print 'range error!', result, ret
        return -4

    #print 'count:', result.count
    item = result.root;

    i = 250
    while i > 0:
        i -= 1
        v = '%010d' % (i*2 + 1)
        if v != item.value:
            print 'range item error!', item.value, v
            return -5
        item = item.next

    print '---------', i

    if i != 0:
        print 'ranged items are not right!'

    m.destroy()

    return 0

def result_check():
    ret1, stat1 = client2master.stat_sys()
    ret2, stat2 = client2slave.stat_sys()
    if stat1 and stat2:
        if stat1.keys != stat2.keys or stat1.values != stat2.values or stat1.data_used != stat2.data_used:
            print 'stat error!'
            print 'ret1: %d, ret2: %d' % (ret1, ret2)
            print 'master stat', stat1
            print 'slave stat', stat2
            return -1
    else:
        print 'stat error!'
        print 'ret1: %d, ret2: %d' % (ret1, ret2)
        return -1

    return 0

def test():
    cmd = "cp ../memlink ../memlink_master -rf"
    print cmd
    os.system(cmd)
    
    cmd = "cp ../memlink ../memlink_slave -rf"
    print cmd
    os.system(cmd)
    
    home = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    os.chdir(home)

    memlink_master_file  = os.path.join(home, 'memlink_master')
    memlink_master_start = memlink_master_file + ' etc/memlink.conf'

    memlink_slave_file  = os.path.join(home, 'memlink_slave')
    memlink_slave_start = memlink_slave_file + ' etc_slave/memlink.conf'

    print 
    print '=============================== test a ============================'
    #start a new master
    cmd = 'bash clean.sh'
    print cmd
    os.system(cmd)
    cmd = "killall memlink_master"
    print cmd
    os.system(cmd)
    time.sleep(1)
    print memlink_master_start
    x1 = subprocess.Popen(memlink_master_start, stdout=subprocess.PIPE, stderr=subprocess.PIPE, 
                             shell=True, env=os.environ, universal_newlines=True) 
    time.sleep(1)
   
    key = 'haha'
    maskstr = "8:1:1"
    ret = client2master.create(key , 12, "4:3:1")
    if ret != MEMLINK_OK:
        print 'create error:', ret, key
        return -1
    print 'create 1 key'

    #insert 3000
    num = 3000
    for i in xrange(0, num):
        val = '%012d' % i
        ret = client2master.insert(key, val, maskstr, i)
        if ret != MEMLINK_OK:
            print 'insert error!', key, val, maskstr, ret
            return -2;
    print 'insert %d val' % num

    #start a new slave
    cmd = 'bash clean_slave.sh'
    print cmd
    os.system(cmd)
    cmd = "killall memlink_slave"
    print cmd
    os.system(cmd)
    time.sleep(1)
    print memlink_slave_start
    x2 = subprocess.Popen(memlink_slave_start, stdout=subprocess.PIPE, stderr=subprocess.PIPE, 
                             shell=True, env=os.environ, universal_newlines=True) 
    #os.system(memlink_slave_start)
    
    time.sleep(1)
    
    print 'dump.....'
    client2master.dump();
    
    #delete val 1000
    num2 = 1000
    for i in range(0, num2):
        val = '%012d' % i
        ret = client2master.delete(key, val)
        if ret != MEMLINK_OK:
        	print 'delete val error:', ret, val
        	return -1
    print 'delete val %d' % num2

    time.sleep(1)

    if 0 != result_check():
        print 'testa error!'
        return -1

    x1.kill()
    x2.kill()
    client2master.close()
    client2slave.close()
    print 'test a ok'
    
    print 
    print '============================= test b =============================='
    #start a new master
    cmd = 'bash clean.sh'
    print cmd
    os.system(cmd)
    cmd = "killall memlink_master"
    print cmd
    os.system(cmd)
    time.sleep(1)
    print memlink_master_start
    x1 = subprocess.Popen(memlink_master_start, stdout=subprocess.PIPE, stderr=subprocess.PIPE, 
                             shell=True, env=os.environ, universal_newlines=True) 
    time.sleep(1)
   
    ret = client2master.create(key , 12, "4:3:1")
    if ret != MEMLINK_OK:
        print 'create error:', ret, key
        return -1
    print 'create 1 key'

    #insert 1000
    num = 1000
    for i in xrange(0, num):
        val = '%012d' % i
        ret = client2master.insert(key, val, maskstr, i)
        if ret != MEMLINK_OK:
            print 'insert error!', key, val, maskstr, ret
            return -2;
    print 'insert %d val' % num

    print 'dump.....'
    client2master.dump();
    
    #insert 2000
    num2 = 3000
    for i in xrange(num, num2):
        val = '%012d' % i
        ret = client2master.insert(key, val, maskstr, i)
        if ret != MEMLINK_OK:
            print 'insert error!', key, val, maskstr, ret
            return -2;
    print 'insert %d val' % (num2 - num)
    
    #start a new slave
    cmd = 'bash clean_slave.sh'
    print cmd
    os.system(cmd)
    cmd = "killall memlink_slave"
    print cmd
    os.system(cmd)
    time.sleep(1)
    print memlink_slave_start
    x2 = subprocess.Popen(memlink_slave_start, stdout=subprocess.PIPE, stderr=subprocess.PIPE, 
                             shell=True, env=os.environ, universal_newlines=True) 
    time.sleep(1)
    
    #delete val 1000
    num2 = 1000
    for i in range(0, num2):
        val = '%012d' % i
        ret = client2master.delete(key, val)
        if ret != MEMLINK_OK:
        	print 'delete val error:', ret, val
        	return -1
    print 'delete val %d' % num2

    time.sleep(1)
    
    if 0 != result_check():
        print 'testb error!'
        return -1

    print 'test b ok'
        
    x1.kill()
    x2.kill()
    
    client2master.destroy()
    client2slave.destroy()

    return 0
    '''
    #move reverse the list
    for i in range(0, num2):
        val = '%010d' % (i * 2 + 1)
        ret = m.move(key, val, 0)
        if ret != MEMLINK_OK:
            print 'move val error:', ret, val
            return -1
    print 'move reverse the list'

    # set all the values' mask = 4:4:1
    for i in range(0, 500):
        val = '%010d' % (i*2 + 1)
        ret = m.mask(key, val, "4:4:1")
        if ret != MEMLINK_OK:
            print 'mask val error:', ret, val
            return -1
    print 'set all the values mask = 4:4:1'

    #tag del 250 - 499
    for i in range(250, 500):
        val = '%010d' % (i*2 + 1)
        ret = m.tag(key, val, 1)
        if ret != MEMLINK_OK:
            print 'tag val error:', ret, val
            return -1
    print 'tag del 250 - 499'

    """ret = m.clean(key)
    if ret != MEMLINK_OK:
        print 'clean error:', ret"""
        
    m.destroy()

    cmd = "killall memlink"
    print cmd
    os.system(cmd)
   
    time.sleep(1)

    print memlinkstart
    #x = subprocess.Popen(memlinkstart, stdout=subprocess.PIPE, stderr=subprocess.PIPE, 
    #                         shell=True, env=os.environ, universal_newlines=True) 
    x = subprocess.Popen(memlinkstart, stdout=subprocess.PIPE, stderr=subprocess.PIPE, 
                             shell=True, env=os.environ, universal_newlines=True) 
    #x = subprocess.Popen(memlinkstart, shell=True, env=os.environ, universal_newlines=True) 
     
    time.sleep(1)
    ret = test_result()

    x.kill()

    return ret
    '''

if __name__ == '__main__':
    sys.exit(test())

