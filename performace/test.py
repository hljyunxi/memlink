# coding: utf-8
import os, sys
sys.path.append('../client/python')
import time
from memlinkclient import *

key = 'testmyhaha'

def test_insert(count):
    print '====== test_insert ======'
    global key
    vals = []
    for i in xrange(0, count):
        val = '%012d' % i
        vals.append(val)

    m = MemLinkClient('127.0.0.1', 11001, 11002, 30)
 
    ret = m.create(key, 12, "4:3:1")
    if ret != MEMLINK_OK:
        print 'create error!', ret
        return

    maskstr = "8:3:1"
    starttm = time.time()
    for val in vals:
        #print 'insert:', val
        ret = m.insert(key, val, maskstr, 0)
        if ret != MEMLINK_OK:
            print 'insert error!', val, ret
            return

    endtm = time.time()
    print 'use time:', endtm - starttm, 'speed:', count / (endtm - starttm)

    m.destroy()


def test_insert_short(count):
    print '====== test_insert ======'
    global key
    vals = []
    for i in xrange(0, count):
        val = '%012d' % i
        vals.append(val)

    maskstr = "8:3:1"
    iscreate = 0

    starttm = time.time()
    for val in vals:
        m = MemLinkClient('127.0.0.1', 11001, 11002, 30)

        if iscreate == 0:
            ret = m.create(key, 12, "4:3:1")
            if ret != MEMLINK_OK:
                print 'create error!', ret
                return
            iscreate = 1

        ret = m.insert(key, val, maskstr, 0)
        if ret != MEMLINK_OK:
            print 'insert error!', val, ret
            return
        m.destroy()

    endtm = time.time()
    print 'use time:', endtm - starttm, 'speed:', count / (endtm - starttm)





def test_range(frompos, dlen, testcount):
    print '====== test_range ======'
    global key

    ss = range(0, testcount)
    m = MemLinkClient('127.0.0.1', 11001, 11002, 30)

    starttm = time.time()
    for i in ss:
        ret, result = m.range(key, "", frompos, dlen)
        if not result or result.count != dlen:
            print 'result error!', ret
            return
         
    endtm = time.time()
    print 'use time:', endtm - starttm
    print 'speed:', testcount / (endtm - starttm)

    m.destroy()

def dotest():
    if len(sys.argv) != 4:
        print 'usage: test.py count range_start range_len'
        sys.exit()
    count = int(sys.argv[1])
    range_start = int(sys.argv[2])
    range_len = int(sys.argv[3])

    test_insert(count)
    test_range(range_start, range_len, 1000)


if __name__ == '__main__':
    dotest()


