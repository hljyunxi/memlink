#!/usr/bin/python
# coding: utf-8
import os, sys
home = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(os.path.join(home, "client/python"))
import time
import subprocess
#import memlinkclient
from memlinkclient import *

READ_PORT  = 11011
WRITE_PORT = 11012

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


def test():
    home = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    os.chdir(home)

    memlinkfile  = os.path.join(home, 'memlink')
    memlinkstart = memlinkfile + ' test/memlink.conf'

    m = MemLinkClient('127.0.0.1', READ_PORT, WRITE_PORT, 30);
   
    key = 'haha000'
    ret = m.create(key, 10, "4:3:1")
    if ret != MEMLINK_OK:
        print 'create error:', ret, key
        return -1
    print 'create 1 key'

    num = 200
    for i in xrange(0, num):
        val = '%010d' % i
        maskstr = "8:1:1"
        ret = m.insert(key, val, maskstr, 0)
        if ret != MEMLINK_OK:
            print 'insert error!', key, val, maskstr, ret
            return -2;
    print 'insert %d val' % num
    print 'dump.....'
    m.dump();
    print 'dump over.....'

    #insert 800 val
    num = 1000
    for i in range(200, num):
        val = '%010d' % i
        maskstr = "8:1:1"
        ret = m.insert(key, val, maskstr, i)
        if ret != MEMLINK_OK:
            print 'insert error!', key, val, maskstr, ret
            return -2;
    print 'insert 800 val'

    #create 199 key
    for i in range(1, 200):
        key = 'haha%03d' % i
        ret = m.create(key, 6, "4:3:1")
        if ret != MEMLINK_OK:
            print 'create error:', ret, key
            return -1
    print 'create 199 key'

    #rmkey key 100-199
    for i in range(100, 200):
        key = 'haha%03d' % i
        ret = m.rmkey(key)
    	if ret != MEMLINK_OK:
        	print 'rmkey error:', ret, key
        	return -1
    print 'rmkey key 100-199'
    
    num2 = num/2
    
    #delete val oushu
    key = 'haha000'
    for i in range(0, num2):
        val = '%010d' % (i * 2)
        ret = m.delete(key, val)
        #print 'ding!', i
        if ret != MEMLINK_OK:
        	print 'delete val error:', ret, val
        	return -1
    print 'delete val oushu'

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

if __name__ == '__main__':
    #sys.exit(test_result())
    sys.exit(test())

