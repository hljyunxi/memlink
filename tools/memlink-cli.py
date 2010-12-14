#coding:utf-8
import os, sys
import time
from memlinkclient import *
import string
#print dir(memlink)

READ_PORT  = 11011
WRITE_PORT = 11012

class ShortInputException (Exception):
    def __init__(self, length, atleast):
        Exception.__init__(self)
        self.length = length
        self.atleast = atleast

class MemLinkTestException (Exception):
    pass

class MemLinkTest:
    def __init__(self, args):
        ip = '127.0.0.1'
        r_port = READ_PORT
        w_port = WRITE_PORT
        timeout = 30
        for item in args:
            if item.startswith('-'):
                if item[:2] == '-h':
                    ip = item[2:]
                elif item[:2] == '-r':
                    r_port = int(item[2:])
                elif item[:2] == '-w':
                    w_port = int(item[2:])
                elif item[:2] == '-t':
                    timeout = int(item[2:])

        print ip, r_port, w_port, timeout
        self.m = MemLinkClient(ip, r_port, w_port, timeout)
        self.allkey = {}
        self.maskformat = '4:3:1'
        self.valuesize = 12
        if not self.m:
            raise MemLinkTestException

    def create(self, args):
        try:
            key = args[0]
            self.maskformat = args[1]
            self.valuesize = int(args[2])
        except:
            key = 'haha'
            self.maskformat = '4:3:1'
            self.valuesize = 12            
        ret = self.m.create(key, self.valuesize, self.maskformat)
        if ret != MEMLINK_OK:
            print 'create error:', ret, key
            return ret
                
    def insert(self, args):
        ''' insert key value 
            insert key value mask
            insert key value mask 0
            insert key value mask 0 -n100
            '''
        try:
            argv = len(args)
            if argv < 2:
                raise ShortInputException(argv, 2)

            num = 1
            i = 0
            while i < len(args):
                if args[i].startswith('-n'):
                    #print args[i]
                    argv -= 1
                    numstr = args[i][2:]
                    if numstr: num = int(numstr)
                    del args[i]
                i += 1
            print num
            if argv == 2:
                key = args[0]
                value = args[1]
                maskstr = '8:2:1'
                pos = 0
            elif argv == 3:
                key = args[0]
                value = args[1]
                maskstr = args[2]
                pos = 0
            else:
                key = args[0]
                value = args[1]
                maskstr = args[2]
                pos = args[3]
        except ShortInputException, x:
            print 'bad input! expected at least %d' % x.atleast
            return -1
            
        if key not in self.allkey:
            ret = self.m.create(key, 12, '4:3:1')
            if ret == MEMLINK_OK or ret == MEMLINK_ERR_EKEY:
                keyinfo = (12, '4:3:1')
                self.allkey[key] = keyinfo
            else:
                print 'create:%s error:%d!'% (key, ret)
                return ret

        for i in xrange(0, num):
            myvalue = value
            if pos == -1:
                pos = i
            valuesize = self.allkey[key][0]
            llen = len(value)
            sstr = '%d' % i
            n = 0
            n = valuesize - llen - len(sstr)
            if n < 0:
                n = 0
            myvalue += '0'*n + sstr
            ret = self.m.insert(key, myvalue, maskstr, pos)
            if ret != MEMLINK_OK:
                print 'insert error:', myvalue, ret
                return ret
            else:
                print 'insert:', (key, myvalue, maskstr, pos, i), 'ok!'
       
        return 0

    def range(self, args):
        ''' range haha 10 100 8:3:1 '''
        try:
            argv = len(args)
            if argv < 1:
                raise ShortInputException(argv, 1)
            key = args[0]
            frompos = int(args[1])
            llen = int(args[2])
            maskstr = args[3]
        except ShortInputException, x:
            print 'bad input! expected at least %d' % x.atleast
            return -1
        except:
            maskstr = ''
            frompos = 0
            llen = 1000

        print 'range %s, %d, %d, "%s"' % (key, frompos, llen, maskstr)

        ret, recs = self.m.range(key, maskstr, frompos, llen)
        if ret != MEMLINK_OK:
            print 'range error:', ret
            return ret
            
        print 'range count:', recs.count
        items = recs.root
        while items:
            print items.value, items.mask
            items = items.next
        return 0

    def update(self, args):
        try:
            argc = len(args)
            if argc < 3:
                raise ShortInputException(argv, 3)
            key = args[0]
            value = args[1]
            pos = int(args[2])
        except ShortInputException, x:
            print 'bad input! expected at least %d' % x.atleast
            return -1
            
        ret = self.m.update(key, value, pos)
        if ret != MEMLINK_OK:
            print 'update (%s, %s, %d) err:%d' % (key, value, pos, ret)
            return -1
        else:
            print 'update (%s, %s, %d) ok' % (key, value, pos)
            
        return 0

    def stat(self, args):
        try:
            key = args[0]
        except:
            print 'bad input! stat must have a key!'
            return -1
        print 'stat %s...' % key
        ret, stat = self.m.stat(key)
        if ret != MEMLINK_OK:
            print 'stat err:', ret
            return -1
        print stat
        return 0
        
    def dump(self, args):
        ret = self.m.dump()
        if ret != MEMLINK_OK:
            print 'dump err:', ret
            return -1
        return 0

    def clean(self, args):
        try:
            argc = len(args)
            if argc < 1:
                raise ShortInputException(argc, 1)
            key = args[0]
        except ShortInputException, x:
            print 'bad input! expected at least %d' % x.atleast
            return -1
                
        ret = self.m.clean(key)
        if ret != MEMLINK_OK:
            print 'clean err:', ret
            return -1
        else:
            print 'clean ok!'
        return 0

    def delete(self, args):
        try:
            argc = len(args)
            if argc < 2:
                raise ShortInputException(argc, 2)

            multidel = 0
            if argc == 2:
                key = args[0]
                value = args[1]
            elif argc == 3:
                key = args[0]
                frompos = int(args[1])
                llen = int(args[2])
                multidel = 1
        except ShortInputException, x:
            print 'bad input! expected at least %d' % x.atleast
            return -1
            
        if not multidel:
            ret = self.m.delete(key, value)
            if ret != MEMLINK_OK:
                print 'del (%s, %s) err:%d!'% (key, value, ret)
                return -1
            else:
                print 'del (%s, %s) OK!'% (key, value)
        else:
            ret, result = self.m.range(key, "", frompos, llen)
            if ret != MEMLINK_OK:
                print 'multidel err:%d' % ret, result
                return -1
            else:
                num = 0
                item = result.root
                while item:
                    ret = self.m.delete(key, item.value)
                    if ret != MEMLINK_OK:
                        print 'del (%s, %s) err:%d!'% (key, item.value, ret)
                        return -1
                    else:
                        num += 1
                        print 'del (%s, %s) ok!'% (key, item.value)
                    item = item.next
                print 'multidel %d' % num
                
        return 0
         
    def tag(self, args):
        try:
            argv = len(args)
            if argv < 3:
                raise ShortInputException(argv, 3)
            key = args[0]
            value = args[1]
            flag = int(args[2])
        except ShortInputException, x:
            print 'bad input! expected at least %d' % x.atleast
            return -1
        ret = self.m.tag(key, value, flag)
        if ret != MEMLINK_OK:
            print 'tag err:', ret
            return -1
        else:
            print 'tag (%s, %s, %d) ok!' % (key, value, flag)
        return 0

    def mask(self, args):
        try:
            argv = len(args)
            if argv < 3:
                raise ShortInputException(argv, 3)
            key = args[0]
            value = args[1]
            maskstr = args[2]
        except ShortInputException, x:
            print 'bad input! expected at least %d' % x.atleast
            return -1
        print 'mask (%s, %s, "%s")' % (key, value, maskstr) 
        ret = self.m.mask(key, value, maskstr)
        if ret != MEMLINK_OK:
            print 'mask err:', ret
        else:
            print 'mask (%s, %s, "%s") ok' % (key, value, maskstr) 
            return -1
        return 0

    def count(self, args):
        try:
            argv = len(args)
            if argv < 1:
                raise ShortInputException(argv, 1)
            elif argv == 1:
                key = args[0]
                maskstr = ''
            else:
                key = args[0]
                maskstr = args[1]
        except ShortInputException, x:
            print 'bad input! expected at least %d' % x.atleast
            return -1
        print 'count (%s, "%s")' % (key, maskstr) 
        ret, count = self.m.count(key, maskstr)
        if ret != MEMLINK_OK:
            print 'count err:', ret
            return -1
        print 'visible_count:%d, tagdel_count:%d' % (count.visible_count, count.tagdel_count)
        return 0

def test_main(args):
    all_the_cmd = ('create', 'rmkey', 'insert', 'del', 'range', 'update', 'tag', 'mask')
    mtest = MemLinkTest(args)
    while 1:
        try:
            sstr = raw_input('memlink> ')
        except EOFError:
            return;
        if not sstr:
            continue
        if sstr in ('q', 'quit'):
            return
        cmd_str = string.split(sstr)
        cmd = cmd_str[0]
        if not hasattr(mtest, cmd):
            print 'bad input, input -help for some help.'
            continue
        args = cmd_str[1:]
        ret = getattr(mtest, cmd)(args)
        #if ret != MEMLINK_OK:
            #print cmd, 'error:', ret

if __name__ == '__main__':
    args = []
    if len(sys.argv) > 1:
        args.extend(sys.argv[1:])
    #print args
    test_main(args)

