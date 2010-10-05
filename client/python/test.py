import os, sys
import memlink
import memlinkclient
#print dir(memlink)

def test1():
    conn = memlink.memlink_create('127.0.0.1', 11001, 11002, 10)
    print dir(conn)
    
    stat = memlink.MemLinkStat()

    key = "haha1"
    memlink.memlink_cmd_stat(conn, key, stat)

    print stat.valuesize, stat.masksize, stat.blocks, stat.data, stat.data_used, stat.mem, stat.mem_used

    result = memlink.MemLinkResult()

    memlink.memlink_cmd_range(conn, key, "::", 2, 10, result)

    item = result.root
    while item:
        print item.value, repr(item.mask)
        item = item.next

    memlink.memlink_result_free(result)

    memlink.memlink_destroy(conn)


def test2():
    m = memlinkclient.MemLinkClient('127.0.0.1', 11001, 11002, 10)
    print dir(m)

    key = 'haha1'

    ret = m.stat(key)
    print ret

    ret = m.range(key, "::", 2, 10) 
    print ret
  
    del ret

    m.destroy()

test2()

