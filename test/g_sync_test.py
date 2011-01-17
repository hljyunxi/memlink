#!/usr/bin/python
# coding: utf-8
import os, sys
home = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(os.path.join(home, "client/python"))
import time
import subprocess
from memlinkclient import *
from synctest import *

def test():
    client2master = MemLinkClient('127.0.0.1', MASTER_READ_PORT, MASTER_WRITE_PORT, 30);
    client2slave  = MemLinkClient('127.0.0.1', SLAVE_READ_PORT, SLAVE_WRITE_PORT, 30);
    
    test_init()

    print 
    print '============================= test g  =============================='
    cmd = 'rm -rf data'
    print cmd
    os.system(cmd)
    cmd = 'cp -rf data_bak data'
    print cmd
    os.system(cmd)
    
    x1 = restart_master()
    time.sleep(3)
    
    cmd = 'rm data/dump.dat'
    print cmd
    os.system(cmd)
    cmd = 'mv data/dump.dat_bak data/dump.dat'
    os.system(cmd)

    x2 = start_a_new_slave()

    #2 kill slave
    time.sleep(10)
    print 'kill slave'
    x2.kill()

    x2 = restart_slave()
    time.sleep(180)
    
    if 0 != stat_check(client2master, client2slave):
        print 'test g error!'
        return -1
    print 'stat ok'
    
    print 'test g ok'

    x1.kill()
    x2.kill()
    
    client2master.destroy()
    client2slave.destroy()

    return 0

if __name__ == '__main__':
    sys.exit(test())

