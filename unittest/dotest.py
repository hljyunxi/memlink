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

    #os.chdir(home)

    memlinkfile = os.path.join(home, 'memlink')
    if not os.path.isfile(memlinkfile):
        print 'not found memlink'
        return

    memlinkstart = memlinkfile + ' test/memlink.conf'

    #files.append('dump_test.py')
    result = {}
    for fn in files:
        fpath = os.path.join(home, 'unittest', fn)
 
        cmd = "killall -9 memlink"
        #os.system(cmd)
       
        print 'run test:', fpath
        ret = os.system(fpath)

        #print 'kill memlink ...'
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

