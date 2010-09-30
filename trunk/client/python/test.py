import os, sys
import memlink

#print dir(memlink)

def test():
    conn = memlink.memlink_create('127.0.0.1', 11001, 11002, 10)
    print dir(conn)

    memlink.memlink_destroy(conn)

test()

