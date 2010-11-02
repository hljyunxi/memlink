#!/usr/bin/python
# coding: utf-8
import os, sys
home = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(os.path.join(home, "client/python"))
import time
import subprocess
#import memlinkclient
from memlinkclient import *

def test_result():
    key = 'haha'
    m = MemLinkClient('127.0.0.1', 11001, 11002, 30);
    
    ret, stat = m.stat(key)
    if stat:
        if stat.data_used != 200:
            print 'stat data_used error!', stat
            return -3
    else:
        print 'stat error:', stat, ret
        return -3

    ret, result = m.range(key, "", 0, 1000)
    if not result or result.count == 0:
        print 'range error!', result, ret
        return -4

    #print 'count:', result.count
    item = result.root;

    c = 200
    while item:
        c -= 1
        v = '%06d' % c
        if v != item.value:
            print 'range item error!', item.value, v
            return -5
        item = item.next

    m.destroy()

    return 0



def test():
    home = os.path.dirname(os.getcwd())
    os.chdir(home)

    memlinkfile  = os.path.join(home, 'memlink')
    memlinkstart = memlinkfile + ' test/memlink.conf'

    m = MemLinkClient('127.0.0.1', 11001, 11002, 30);
   
    key = 'haha'
    ret = m.create(key, 6, "4:3:1")
    if ret != MEMLINK_OK:
        print 'create error:', ret
        return -1

    for i in range(0, 200):
        val = '%06d' % i
        maskstr = "8:1:1"
        ret = m.insert(key, val, maskstr, 0)
        if ret != MEMLINK_OK:
            print 'insert error!', key, val, maskstr, ret
            return -2;

    
    m.dump();
    
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
     
    time.sleep(5)
    ret = test_result()

    x.kill()

    return ret

if __name__ == '__main__':
    #sys.exit(test_result())
    sys.exit(test())

