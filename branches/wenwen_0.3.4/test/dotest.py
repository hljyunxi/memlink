# coding: utf-8
import os, sys
import glob
import subprocess
import time

def testone(fn, wait=2):
    home = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    cwd  = os.getcwd()

    print 'home:', home, 'cwd:', cwd
    fpath = os.path.join(home, 'test', fn)

    memlinkfile = os.path.join(home, 'memlink')
    if not os.path.isfile(memlinkfile):
        print 'not found memlink:', memlinkfile
        return
    memlinkstart = '%s %s/test/memlink.conf' % (memlinkfile, home)

    cmd = "killall -9 memlink"
    os.system(cmd)

    logfile = os.path.join('memlink.log')
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
    if os.path.isfile('dotest.log'):
        os.remove('dotest.log')

    files = glob.glob("*_test")
    files.sort()
    print files
    #pyfiles = glob.glob("*_test.py")
    pyfiles = ['dump_test.py', 'push_pop_test.py', 'sortlist_test.py']
    files += pyfiles

    result = {}
    print 'do all test ...'
    
    f = open('dotest.log', 'w')
    for fn in files:
        ret = testone(fn)
        if ret != 0: # failed
            result[fn] = -1
            f.write('%s\t0\n' % fn)
        else: # success
            result[fn] = 0
            f.write('%s\t1\n' % fn)
    f.close()

    return result

if __name__ == '__main__':
    if len(sys.argv) > 1:
        sys.exit(testone(sys.argv[1]))
    else:
        ret = test()
        for row in ret:
            if row < 0:
                sys.exit(row)

