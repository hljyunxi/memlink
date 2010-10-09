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

    for fn in files:
        fpath = os.path.join(home, 'test', fn)
 
        cmd = "killall -9 memlink"
        os.system(cmd)
       
        cmd = "rm -rf data/bin.log*"
        os.system(cmd)

        cmd = "rm -rf data/dump.dat*"
        os.system(cmd)

        print 'open memlink:', memlinkfile
        x = subprocess.Popen(memlinkfile, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True, env=os.environ, universal_newlines=True) 
        print 'go ..' 
        
        time.sleep(2)

        print 'run test:', fpath
        ret = os.system(fpath)

        print 'kill memlink ...'
        x.kill()
        print 'kill complete!'

        #print x.stderr.read()
        #print x.stdout.read()

        if ret != 0:
            print fn, '\t\t\33[31mfailed!\33[0m'
        else:
            print fn, '\t\t\33[32failed!\33[0m'
            
        break


if __name__ == '__main__':
    test()

