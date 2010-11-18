import os, sys
from memlinkclient import *
#print dir(memlink)

def test():
    m = MemLinkClient('127.0.0.1', 11001, 11002, 10)
    #print dir(m)

    key = 'haha'
    ret, result = m.stat(key)
    print result

    #ret, result = m.range(key, "::", 2, 10) 
    #print ret, result
    #del result

    m.destroy()

def insert():
    m = MemLinkClient('127.0.0.1', 11011, 11012, 10)
   
    ret = m.create('haha', 12, "1")
    if ret != MEMLINK_OK:
        print 'create haha error!', ret
        return

    for i in xrange(0, 100):
        ret = m.insert('haha', '%012d' % i, "1", 0)
        if ret != MEMLINK_OK:
            print 'insert error:', ret, i
            return
    
    m.destroy()

def dump():
    m = MemLinkClient('127.0.0.1', 11011, 11012, 10)
    ret = m.dump()
    if ret != MEMLINK_OK:
        print 'dump error!', ret
    m.destroy()

def dump():
    m = MemLinkClient('127.0.0.1', 11011, 11012, 10)
    ret = m.range('haha', '', 0 ,100)
    if ret != MEMLINK_OK:
        print 'dump error!', ret
    m.destroy()


if __name__ == '__main__':
    action = sys.argv[1]

    func = globals()[action]
    func()
        


