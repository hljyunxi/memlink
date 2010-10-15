# coding: utf-8
import os, sys
import glob
import subprocess
import time

def test():
    files = glob.glob("*_test")
    files.sort()
    print files
    home = os.path.dirname(os.getcwd())
    #cmd = 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:' + os.path.join(home, 'client', 'c')
    #print cmd
    #os.system(cmd)

    os.chdir(home)

    memlinkfile = os.path.join(home, 'memlink')
    if not os.path.isfile(memlinkfile):
        print 'not found memlink'
        return

    memlinkstart = memlinkfile + ' test/memlink.conf'

    files.append('dump_test.py')
    result = {}
    for fn in files:
        fpath = os.path.join(home, 'test', fn)
 
        cmd = "killall -9 memlink"
        os.system(cmd)

        logfile = os.path.join(home, 'memlink.log')
        if os.path.isfile(logfile):
            print 'remove log:', logfile
            os.remove(logfile)
        #cmd = "rm -rf data/bin.log*"
        #os.system(cmd)
        binfiles = glob.glob('data/bin.log*')
        for bf in binfiles:
            print 'remove binlog:', bf
            os.remove(bf)

        #cmd = "rm -rf data/dump.dat*"
        #os.system(cmd)
        binfiles = glob.glob('data/dump.dat*')
        for bf in binfiles:
            print 'remove dump:', bf
            os.remove(bf)

        #print 'open memlink:', memlinkstart
        x = subprocess.Popen(memlinkstart, stdout=subprocess.PIPE, stderr=subprocess.PIPE, 
                             shell=True, env=os.environ, universal_newlines=True) 
        time.sleep(1)
       
        if fpath.endswith('.py'):
            fpath = 'python ' + fpath
        print 'run test:', fpath
        ret = os.system(fpath)

        #print 'kill memlink ...'
        x.kill()
        #print 'kill complete!'

        #print x.stderr.read()
        #print x.stdout.read()

        if ret != 0:
            result[fn] = 0
            #print fn, '\t\t\33[31mfailed!\33[0m'
        else:
            result[fn] = 1
            #print fn, '\t\t\33[32msuccess!\33[0m'
            
        #break

    print '=' * 60
    for k,v in result.iteritems():
        if v:
            print k, '\t\t\33[32msuccess!\33[0m' 
        else:
            print k, '\t\t\33[31mfailed!\33[0m' 


    return result

if __name__ == '__main__':
    test()

