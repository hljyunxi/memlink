import os, sys
from memlinkclient import *
#print dir(memlink)

READ_PORT  = 11001
WRITE_PORT = 11002

def test():
    m = MemLinkClient('127.0.0.1', READ_PORT, WRITE_PORT, 10)
    #print dir(m)

    key = 'haha'
    ret, result = m.stat(key)
    print result

    #ret, result = m.range(key, "::", 2, 10) 
    #print ret, result
    #del result

    m.destroy()

def insert():
    m = MemLinkClient('127.0.0.1', READ_PORT, WRITE_PORT, 10)
   
    ret = m.create('haha', 12, "1")
    if ret != MEMLINK_OK:
        print 'create haha error!', ret
        return

    for i in xrange(0, 1000):
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


def range():
    m = MemLinkClient('127.0.0.1', READ_PORT, WRITE_PORT, 10)
    
    ret, recs = m.range('haha', '1', 0, 1000)
    if ret != MEMLINK_OK:
        print 'range error:', ret
    print recs.count
    items = recs.root
    while items:
        print items.value
        items = items.next

    m.destroy()

def dump():
    m = MemLinkClient('127.0.0.1', READ_PORT, WRITE_PORT, 10)
    ret = m.dump()
    if ret != MEMLINK_OK:
        print 'dump error!', ret
    m.destroy()


def clean():
    m = MemLinkClient('127.0.0.1', READ_PORT, WRITE_PORT, 10)
    ret = m.clean('haha')
    if ret != MEMLINK_OK:
        print 'clean error!', ret
    m.destroy()

def stat():
    m = MemLinkClient('127.0.0.1', READ_PORT, WRITE_PORT, 10)
    ret, stat = m.stat('haha')
    if ret != MEMLINK_OK:
        print 'clean error!', ret
    print stat
    m.destroy()


def insert_haha():
    m = MemLinkClient('127.0.0.1', READ_PORT, WRITE_PORT, 10)
 
    ret = m.create('haha1', 12, "1")
    if ret != MEMLINK_OK:
        print 'create haha error!', ret
  
    for i in xrange(0, 1):
        print 'insert:', '%012d' % i
        ret = m.insert('haha1', '%012d' % i, "1", 0)
        if ret != MEMLINK_OK:
            print 'insert error:', ret, i
            return
    
    m.destroy()


if __name__ == '__main__':
    action = sys.argv[1]

    func = globals()[action]
    func()
        


