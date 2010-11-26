import os, sys
import time
from memlinkclient import *
#print dir(memlink)

READ_PORT  = 11001
WRITE_PORT = 11002

def insert(*args):
    try:
        num = int(args[0])
    except:
        num = 1000
    print 'insert:', num

    m = MemLinkClient('127.0.0.1', READ_PORT, WRITE_PORT, 10)
   
    ret = m.create('haha', 12, "1")
    if ret != MEMLINK_OK:
        print 'create haha error!', ret
        return

    for i in xrange(0, num):
        ret = m.insert('haha', '%012d' % i, "1", 0)
        if ret != MEMLINK_OK:
            print 'insert error:', ret, i
            return

    m.destroy()

def delete():
    m = MemLinkClient('127.0.0.1', READ_PORT, WRITE_PORT, 10)

    for i in xrange(1, 300):
        print 'del %012d' % (i*2)
        ret = m.delete('haha', '%012d' % (i*2))
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
    
    ret, recs = m.range('haha', mask, frompos, slen)
    if ret != MEMLINK_OK:
        print 'range error:', ret
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
    ret = m.clean('haha')
    if ret != MEMLINK_OK:
        print 'clean error!', ret
    m.destroy()

def stat(*args):
    m = MemLinkClient('127.0.0.1', READ_PORT, WRITE_PORT, 10)
    ret, stat = m.stat('haha')
    if ret != MEMLINK_OK:
        print 'clean error!', ret
    print stat
    m.destroy()


if __name__ == '__main__':
    action = sys.argv[1]
    args = []
    if len(sys.argv) > 2:
        args.extend(sys.argv[2:])

    func = globals()[action]
    func(*args)
        


