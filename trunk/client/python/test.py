import os, sys
import time
from memlinkclient import *
#print dir(memlink)

READ_PORT  = 11001
WRITE_PORT = 11002

key = 'haha'

def insert(*args):
    try:
        start = int(args[0])
        num   = int(args[1])
    except:
        start = 0
        num   = 1000

    try:
        val = '%012d' % int(args[2])
    except:
        val = None

    print 'insert:', start, num, val

    m = MemLinkClient('127.0.0.1', READ_PORT, WRITE_PORT, 10)
   
    ret = m.create(key, 12, "1")
    if ret != MEMLINK_OK:
        print 'create haha error!', ret
        #return

    for i in xrange(start, start + num):
        if not val:
            val2 = '%012d' % i
        else:
            val2 = val
        print 'insert:', val2
        ret = m.insert(key, val2, "1", i)
        if ret != MEMLINK_OK:
            print 'insert error:', ret, i
            return

    m.destroy()

def delete(*args):
    m = MemLinkClient('127.0.0.1', READ_PORT, WRITE_PORT, 10)
    val = '%012d' % int(args[0]) 
    print 'delete:', val
    ret = m.delete(key, val)
    if ret != MEMLINK_OK:
        print 'delete error:', ret, val
        return

    m.destroy()
    
    return

    for i in xrange(1, 300):
        print 'del %012d' % (i*2)
        ret = m.delete(key, '%012d' % (i*2))
        if ret != MEMLINK_OK:
            print 'delete error:', ret, i*2
            return
    
    m.destroy()


def range(*args):
    try:
        frompos = int(args[0])
        slen    = int(args[1])
        mask    = args[2]
    except:
        frompos = 0
        slen    = 1000
        mask    = '1'

    print 'range from:%d, len:%d, mask:%s' % (frompos, slen, mask)

    m = MemLinkClient('127.0.0.1', READ_PORT, WRITE_PORT, 10)
    
    ret, recs = m.range(key, mask, frompos, slen)
    if ret != MEMLINK_OK:
        print 'range error:', ret
        return

    print recs.count
    items = recs.root
    while items:
        print items.value
        items = items.next

    m.destroy()

def dump(*args):
    m = MemLinkClient('127.0.0.1', READ_PORT, WRITE_PORT, 10)
    ret = m.dump()
    if ret != MEMLINK_OK:
        print 'dump error!', ret
    m.destroy()


def clean(*args):
    m = MemLinkClient('127.0.0.1', READ_PORT, WRITE_PORT, 10)
    ret = m.clean(key)
    if ret != MEMLINK_OK:
        print 'clean error!', ret
    m.destroy()

def stat(*args):
    m = MemLinkClient('127.0.0.1', READ_PORT, WRITE_PORT, 10)
    ret, stat = m.stat(key)
    if ret != MEMLINK_OK:
        print 'clean error!', ret
    print stat
    m.destroy()

def ping(*args):
    try:
        count = int(args[1])
    except:
        count = 1000
    print 'ping count:', count
    start = time.time()
    m = MemLinkClient('127.0.0.1', READ_PORT, WRITE_PORT, 10)
    for i in xrange(0, count):
        ret = m.ping()
        if ret != MEMLINK_OK:
            print 'ping error:', ret
            return 
    m.destroy()
    end = time.time()

    print 'use time:', end - start, 'speed:', count / (end-start)

if __name__ == '__main__':
    action = sys.argv[1]
    args = []
    if len(sys.argv) > 2:
        args.extend(sys.argv[2:])

    func = globals()[action]
    func(*args)
            


