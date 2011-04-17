# coding: utf-8
import os, sys
import glob
import subprocess
import time

def testone(fn, wait=2):
    home   = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    print 'home:', home
    fpath = os.path.join(home, 'test', fn)

    memlinkfile = os.path.join(home, 'memlink')
    if not os.path.isfile(memlinkfile):
        print 'not found memlink:', memlinkfile
        return
    memlinkstart = '%s %s/test/memlink.conf' % (memlinkfile, home)

    cmd = "killall -9 memlink"
    os.system(cmd)

    logfile = os.path.join(home, 'memlink.log')
    if os.path.isfile(logfile):
        print 'remove log:', logfile
        os.remove(logfile)

    binfiles = glob.glob('data/bin.log*')
    for bf in binfiles:
        print 'remove binlog:', bf
        os.remove(bf)

    binfiles = glob.glob('data/dump.dat*')
    for bf in binfiles:
        print 'remove dump:', bf
        os.remove(bf)

    print 'open memlink:', memlinkstart
    x = subprocess.Popen(memlinkstart, stdout=subprocess.PIPE, stderr=subprocess.PIPE, 
                         shell=True, env=os.environ, universal_newlines=True) 
    time.sleep(wait)
   
    if fpath.endswith('.py'):
        fpath = 'python ' + fpath
    print 'run test:', fpath
    ret = os.system(fpath)
    x.kill()
    
    if ret != 0:
        print fn, '\t\t\33[31mfailed!\33[0m'
    else:
        print fn, '\t\t\33[32msuccess!\33[0m'

    return ret

def test():
    files = glob.glob("*_test")
    files.sort()
    print files

    files.append('dump_test.py')
    result = {}
    print 'do all test ...'

    for fn in files:
        ret = testone(fn)
        if ret != 0:
            result[fn] = 0
        else:
            result[fn] = 1
    return result

if __name__ == '__main__':
    if len(sys.argv) > 1:
        testone(sys.argv[1])
    else:
        test()

