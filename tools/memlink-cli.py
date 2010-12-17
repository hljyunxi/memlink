#coding:utf-8
import os, sys
clientpath = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'client/python')
sys.path.append(clientpath)
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
        '''Command -- create.

        create key valuesize maskformat -- for instance : create haha 12 4:3:1
        '''
        try:
            argc = len(args)
            if argc == 1:
                key = args[0]
                valuesize = 12
                maskformat = '4:3:1'
            elif argc >= 3:
                key = args[0]
                valuesize = int(args[1])
                maskformat = args[2]
            else:
                raise ShortInputException(argc, 3)
        except ShortInputException, x:
            print 'bad input! expected at least %d' % x.atleast
            return -1
        
        opnum = 0
        while 1:
            opnum += 1
            ret = self.m.create(key, valuesize, maskformat)
            if ret == MEMLINK_OK:
                print 'create %s ok!'% key
                return 0
            elif ret == MEMLINK_ERR_RECV:
                if opnum < 2:
                    continue
                else:
                    print 'create %s err:%d'% (key, ret)
                    return -1
            elif ret == MEMLINK_ERR_EKEY:
                print '%s existed' % key
                return -1
            else:
                print 'create %s err:%d'% (key, ret)
                return ret

    def rmkey(self, args):
        '''Command -- rmkey.

        rmkey key
        '''
        try:
            argc = len(args)
            if argc < 1:
                raise ShortInputException(argc, 1)
            else:
                key = args[0]
        except ShortInputException, x:
            print 'bad input! expected at least %d' % x.atleast
            return -1

        opnum = 0
        while 1:
            opnum += 1
            ret = self.m.rmkey(key)
            if ret == MEMLINK_OK:
                print 'rmkey %s ok!'% key
                return 0
            elif ret == MEMLINK_ERR_RECV:
                if opnum < 2:
                    continue
                else:
                    print 'rmkey %s err:%d'% (key, ret)
                    return -1
            else:
                print 'rmkey %s err:%d'% (key, ret)
                return ret
                
    def insert(self, args):
        '''Command -- create.
    
        insert key value 
        insert key value mask
        insert key value mask pos
        insert key value mask pos -nNUM
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
            #print num
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
                pos = int(args[3])
        except ShortInputException, x:
            print 'bad input! expected at least %d' % x.atleast
            return -1
            
        '''if key not in self.allkey:
            #print 'key does not exist!'
            print 'key does not exist! do you want to create it as default'
            ret = self.m.create(key, 12, '4:3:1')
            keyinfo = (12, '4:3:1')
            if ret == MEMLINK_OK:
                self.allkey[key] = keyinfo
            elif ret == MEMLINK_ERR_EKEY:
                ret, stat = self.m.stat(key)
                if ret == MEMLINK_OK:
                    keyinfo[0] = stat.valuesize
                self.allkey[key] = keyinfo
            else:
                print 'create:%s error:%d!'% (key, ret)
                return ret'''
                
        if key not in self.allkey:
            opnum = 0
            while 1:
                opnum += 1
                ret, stat = self.m.stat(key)
                if ret == MEMLINK_ERR_RECV:
                    if opnum < 2:
                        continue
                    else:
                        print 'err %d', ret
                        return -1
                elif ret == MEMLINK_OK:
                    keyinfo = [12, '4:3:1']
                    keyinfo[0] = stat.valuesize
                    self.allkey[key] = keyinfo
                elif ret == MEMLINK_ERR_NOKEY:
                    print 'key does not exist!'
                    return -1
                break;
        
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
                print 'insert:', (key, myvalue, maskstr, pos, i), 'err:%d!'%ret
                return ret
            else:
                print 'insert:', (key, myvalue, maskstr, pos, i), 'ok!'
       
        return 0

    def range(self, args):
        '''Command range.
    
        range key frompos llen maskstr -- for instance : 
        range haha
        range haha 10 1000
        range haha 10 1000 8::1
        '''
        try:
            argv = len(args)
            if argv < 1:
                raise ShortInputException(argv, 1)
            if argv == 1:
                key = args[0]
                maskstr = ''
                frompos = 0
                llen = 1000
            elif argv == 3:
                key = args[0]
                frompos = int(args[1])
                llen = int(args[2])
                maskstr = ''
            elif argv == 4:
                key = args[0]
                frompos = int(args[1])
                llen = int(args[2])
                maskstr = args[3]
            else:
                print 'bad input!'
                return -1
        except ShortInputException, x:
            print 'bad input! expected at least %d' % x.atleast
            return -1

        opnum = 0
        while 1:
            opnum += 1
            ret, recs = self.m.range(key, maskstr, frompos, llen)
            if ret == MEMLINK_OK:
                print 'range (%s, %d, %d, "%s") ok"' % (key, frompos, llen, maskstr)
                break
            elif ret == MEMLINK_ERR_RECV:
                if opnum < 2:
                    continue
                else:
                    print 'range %s err:%d'% (key, ret)
                    return ret
            else:
                print 'range %s err:%d'% (key, ret)
                return ret
            
        print 'range count:', recs.count
        items = recs.root
        while items:
            print items.value, items.mask
            items = items.next
            
        return 0

    def update(self, args):
        '''Command update.

        update key value pos -- for instance : update haha 123 0
        '''
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
        '''Command stat.

        stat key
        '''
        try:
            key = args[0]
        except:
            print 'bad input! stat must have one param!'
            return -1
        
        opnum = 0
        while 1:
            opnum += 1
            ret, stat = self.m.stat(key)
            if ret == MEMLINK_OK:
                print 'stat %s ok' % key
                print stat
                return 0
            elif ret == MEMLINK_ERR_RECV:
                if opnum < 2:
                    continue
                else:
                    print 'stat err:', ret
                    return -1
            else:
                print 'stat err:', ret
                return -1
     
    def dump(self, args):
        '''Command dump.

        dump
        '''
        ret = self.m.dump()
        if ret != MEMLINK_OK:
            print 'dump err:', ret
            return -1
        return 0

    def clean(self, args):
        '''Command clean.

        clean key
        '''
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
        '''Command delete.
    
        delete key value: delete haha 123
        delete key frompos len : delete haha 10 50
        '''
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
        '''Command tag.
    
        tag key value flag : flag -- 1 for tag del; 0 for restore.
        '''
    
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
        '''Command mask.
    
        mask key value maskstr -- for instance: mask haha 123 4::1
        '''
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
        '''Command count.
    
        count key
        count key maskstr
        '''
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
            
        opnum = 0
        while 1:
            opnum += 1
            ret, count = self.m.count(key, maskstr)
            if ret == MEMLINK_OK:
                print 'count (%s, "%s") ok.' % (key, maskstr) 
                print 'visible_count:%d, tagdel_count:%d' % (count.visible_count, count.tagdel_count)
                return 0
            elif ret == MEMLINK_ERR_RECV:
                if opnum < 2:
                    continue
                else:
                    print 'count err:', ret
                    return -1
            else:
                print 'count err:', ret
                return -1

def test_main(args):
    all_the_cmd = ('create', 'rmkey', 'insert', 'delete', 'range', 'update', 'tag', 'mask', 'count', 'stat', 'dump', 'clean')
    mtest = MemLinkTest(args)
    print ''
    print 'help -- type "help command" for some help. for instance: help insert.'
    print 'list -- type "list" to list all the commands that are supported.'
    print '   q -- quit.'
    print ''
    
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
        if not hasattr(mtest, cmd) and cmd != 'help'and cmd != 'list':            
            print 'Bad input. DO NOT have this command.'
            continue

        if cmd == 'list':
            print all_the_cmd
        elif cmd == 'help':
            if len(cmd_str) < 2:
                print 'type "help command" for some help. for instance: help insert'
            elif hasattr(mtest, cmd_str[1]):
                print getattr(mtest, cmd_str[1]).__doc__
            else:
                print 'command "%s" don\'t exist! All are as follows:', cmd_str[1], all_the_cmd
        else:
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


