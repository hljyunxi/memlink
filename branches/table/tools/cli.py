# coding:utf-8
import os, sys
home = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
clientpath = os.path.join(home, 'client/python')
sys.path.append(clientpath)
import traceback
import time
import getopt
from memlinkclient import *


class MemLinkErrors:
    def __init__(self):
        self.errors = {}
         
        self.load() 

    def load(self):
        global home

        fpath = os.path.join(home, 'common.h')
        f = open(fpath, 'r')
        lines = f.readlines()
        f.close()
        
        for i in range(0, len(lines)):
            line = lines[i]
            if line.find('MEMLINK_ERR_') > 0:
                parts = line.split() 
                s = lines[i-1].strip()[2:].strip()
                self.errors[int(parts[-1])] = s

    def __str__(self):
        s = ''
        for k,v  in self.errors.iteritems():
            s += '%d %s\n' % (k, v)
        return s

    def get(self, n):
        return self.errors[n]

errors = MemLinkErrors()

class MemLinkCli:
    def __init__(self, host='127.0.0.1', r=11001, w=11002, timeout=10):
        self.host = host
        self.r = r
        self.w = w
        self.timeout = timeout

        self.m = MemLinkClient(self.host, self.r, self.w, self.timeout)

        self.cmds = []

        self.table_type_map = {'list': MEMLINK_LIST, 
                               'l': MEMLINK_LIST,
                               'queue': MEMLINK_QUEUE,
                               'q': MEMLINK_QUEUE,
                               'sortlist': MEMLINK_SORTLIST,
                               's': MEMLINK_SORTLIST,
                              }
        self.table_value_map = {'string': MEMLINK_VALUE_STRING,
                                's': MEMLINK_VALUE_STRING,
                                'binary': MEMLINK_VALUE_OBJ,
                                'b': MEMLINK_VALUE_OBJ,
                                'integer': MEMLINK_VALUE_INT,
                                'i': MEMLINK_VALUE_INT,
                                'float': MEMLINK_VALUE_FLOAT,
                                'f': MEMLINK_VALUE_FLOAT
                                }

        self.value_flag_map = {'all': MEMLINK_VALUE_ALL,
                               'a': MEMLINK_VALUE_ALL,
                               'tagdel': MEMLINK_VALUE_TAGDEL,
                               'd': MEMLINK_VALUE_TAGDEL,
                               'visible': MEMLINK_VALUE_VISIBLE,
                               'v': MEMLINK_VALUE_VISIBLE,
                              }
        self.value_tag_map = {'tagdel': MEMLINK_TAG_DEL,
                              'd': MEMLINK_TAG_DEL,
                              'restore': MEMLINK_TAG_RESTORE,
                              'r': MEMLINK_TAG_RESTORE,
                            } 
    
    def run(self):
        print '\33[31mmemlink at %s:%d,%d %d\33[0m' % (self.host, self.r, self.w, self.timeout)
        while True:
            try:
                line = raw_input('memlink>')
            except:
                print
                print 'bye'
                return
            line = line.strip()
            if line in ['q', 'quit', 'exit']:
                print 'bye'
                return
            parts = line.split() 
            try:
                func = getattr(self, 'cmd_' + parts[0])
            except:
                print 'unkown command.'
            else:
                try:
                    func(parts[1:])
                except:
                    traceback.print_exc() 
        
    
    def cmd_help(self, args=None):
        if not args: 
            cmds = []
            for k in dir(self):
                if k.startswith('cmd_'):
                    cmds.append(k[4:])
            print 'commands:\n\t' + ', '.join(cmds) + '\n\nuse "help command" for detail'
            #for c in cmds:
            #    docstr = getattr(self, 'cmd_' + c).__doc__
            #    if docstr:
            #        print docstr + '\n'
            return
        else:
            cmd = args[0]
            print getattr(self, 'cmd_' + cmd).__doc__ 

    def cmd_table(self, args):
        '''
\33[32mtable\tname,valuesize,listtype,valuetype,[attr]\33[0m
description:
    create table
arguments:
    name: string, table name
    valuesize: integer, value size in list, must <=255
    listtype: list(l)/queue(q)/sortlist(s) 
    valuetype: string(s)/binary(b)/integer(i)/float(f)
    attr: xx:xx:xx, attribute define. split by colon. not more than number of 15 items.
eg:
    table test 4 q s 4:3:1
    table test2 4 q i
    table test3 4 q i 2:1
        '''
        try:
            name = args[0]
            valuesize = int(args[1])
            listtype = self.table_type_map[args[2]]
            valuetype = self.table_value_map[args[3]]
            try:
                attrformat = args[4]
            except:
                attrformat = ''
        except Exception, e:
            print 'table argument error!', e
            return

        ret = self.m.create_table(name, valuesize, listtype, valuetype, attrformat)
        if ret != MEMLINK_OK:
            print 'error:', ret, errors.get(ret)
        else:
            print 'ok', ret

    def cmd_list(self, args):
        '''
\33[32mlist\tname,valuesize,[attr]\33[0m
description:
    create table, with type list
arguments:
    name: string, table name
    valuesize: integer, value size in list 
    attr: xx:xx:xx, attribute fomat, split by colon.
eg:
    list test 4 4:3:1
    list test2 4
        '''
        try:
            name = args[0]
            valuesize = int(args[1])
            if len(args) == 3:
                attr = args[2]
            else:
                attr = ''
        except Exception, e:
            print 'argument error!', e
            return

        ret = self.m.create_table_list(name, valuesize, attr)
        if ret != MEMLINK_OK:
            print 'error:', ret, errors.get(ret)
        else:
            print 'ok', ret

    def cmd_queue(self, args):
        '''
\33[32mqueue\tname,valuesize,[attr]\33[0m
description:
    create table, with type queue
arguments:
    name: string, table name
    valuesize: integer, value size in list 
    attr: xx:xx:xx, attribute fomat, split by colon.
eg:
    queue test 4 4:3:1
    queue test2 4
        '''

        # args: name,valuesize,attrformat
        try:
            name = args[0]
            valuesize = int(args[1])
            attr = ''
            if len(args) == 3:
                attr = args[2]
        except:
            print 'argument error! args: name, valuesize, [attrformat]'
            return

        ret = self.m.create_table_list(name, valuesize, attr)
        if ret != MEMLINK_OK:
            print 'error:', ret, errors.get(ret)
        else:
            print 'ok', ret

       

    def cmd_sortlist(self, args):
        '''
\33[32msortlist\tname,valuesize,valuetype,[attr]\33[0m
description:
    create table, with type queue
arguments:
    name: string, table name
    valuesize: integer, value size in list 
    valuetype: string(s)/binary(b)/integer(i)/float(f)
    attr: xx:xx:xx, attribute fomat, split by colon.
eg:
    sortlist test 4 4:3:1
    sortlist test2 4
        '''
        # args: name,valuesize,valuetype,attrformat
        try:
            name = args[0]
            valuesize = int(args[1])
            valuetype = self.table_value_map[args[2]]
            attr = ''
            if len(args) == 4:
                attr = args[3]
        except:
            print 'argument error! args: name, valuesize, [attrformat]'
            return

        ret = self.m.create_table_sortlist(name, valuesize, valuetype, attr)
        if ret != MEMLINK_OK:
            print 'error:', ret, errors.get(ret)
        else:
            print 'ok', ret

    def cmd_ping(self, args):
        '''
\33[32mping\t\33[0m
description:
    ping, send a message to memlink read port
arguments:
    no arguments
        '''

        ret = self.m.ping()
        if ret != MEMLINK_OK:
            print 'error:', ret, errors.get(ret)
        else:
            print 'ok', ret

    def cmd_tables(self, args):
        ret, tbs = self.m.tables()
        if ret != MEMLINK_OK:
            print 'error:', ret, errors.get(ret)
            return

        print 'ok', ret
        for x in ret:
            print x

    def cmd_range(self, args):
        '''
\33[32mrange\t\33[0m
description:
    query part of list data
arguments:
    key: key of value, fomat: table.list
    valueflag: all(a)/tagdel(d)/visible(v)
    from: start position
    len: query length
    attr: xx:xx:xx, condition for value filter. split by colon
eg:
    range table.abc a 0 100
    range table.ccc v 0 100 4:2:
        '''

        # args: key, valueflag, from, len, attr
        try:
            key = args[0]
            valflag = self.value_flag_map[args[1]]
            start  = int(args[2])
            length = int(args[3])
            attr = ''
            if len(args) == 5:
                attr = args[4]
        except:
            print 'argument error! args: key,valueflag,from,len,[attrformat]'
            return

        ret, obj = self.m.range(key, valflag, start, length, attr)
        if ret != MEMLINK_OK:
            print 'error:', ret, errors.get(ret)
            return

        print 'ok', ret
        print 'count:', obj.count
        for x in obj.list():
            if x[1]:
                print 'value:', x[0], 'value:', repr(x[0]), 'attr:', x[1]
            else:
                print 'value:', x[0], 'value(binary):', repr(x[0]).strip("'")

    def cmd_node(self, args):
        '''
\33[32mnode\tkey\33[0m
description:
    create list node with null
arguments:
    key: key of value, fomat: table.list
eg:
    node table.abc
        '''
        try:
            key = args[0]
        except:
            print 'argument error! args: key'
            return
        # key
        ret = self.m.create_node(key)
        if ret != MEMLINK_OK:
            print 'error:', ret, errors.get(ret)
            return
        print 'ok', ret

    def cmd_rmtable(self, args):
        '''
\33[32mrmtable\tkey\33[0m
description:
    remove table
arguments:
    key: key of value, fomat: table.list
eg:
    rmtable tablename
        '''
        try:
            key = args[0]
        except:
            print 'argument error! args: key'
            return
        # key
        ret = self.m.rmtable(key)
        if ret != MEMLINK_OK:
            print 'error:', ret, errors.get(ret)
            return
        print 'ok', ret

    def cmd_rmkey(self, args):
        '''
\33[32mrmkey\tkey\33[0m
description:
    remove list in table
arguments:
    key: key of value, fomat: table.list
eg:
    rmkey tablename
        '''
        try:
            key = args[0]
        except:
            print 'argument error! args: key'
            return
        # key
        ret = self.m.rmkey(key)
        if ret != MEMLINK_OK:
            print 'error:', ret, errors.get(ret)
            return
        print 'ok', ret



    def cmd_insert(self, args):
        '''
\33[32minsert\tkey value position [attribute]\33[0m
description:
    query part of list data
arguments:
    key: key of value, fomat: table.list
    value: value in list
    position: where to storing. 0 means first, -1 means last
    attribute: xx:xx:xx, split by colon
eg:
    insert table.abc 1111 0
    insert table.ccc 1111 0 4:2:1
        '''

        # key, value, pos, attr
        try:
            key = args[0]
            value  = args[1]
            pos  = int(args[2])
            attr = ''
            if len(args) == 4:
                attr = args[3]
        except:
            print 'argument error! args: key,valueflag,from,len,[attrformat]'
            return

        ret = self.m.insert(key, value, pos, attr)
        if ret != MEMLINK_OK:
            print 'error:', ret, errors.get(ret)
            return
        print 'ok', ret

    def cmd_del(self, args):
        '''
\33[32mdel\tkey value\33[0m
description:
    remove value in list
arguments:
    key: key of value, fomat: table.list
    value: value for delete
eg:
    del table.aaa 11111
        '''
        try:
            key = args[0]
            value = args[1]
        except:
            print 'argument error! args: key'
            return
        # key
        ret = self.m.delete(key, value)
        if ret != MEMLINK_OK:
            print 'error:', ret, errors.get(ret)
            return
        print 'ok', ret


    def cmd_move(self, args):
        '''
\33[32mmove\tkey value position\33[0m
description:
    move value to another position
arguments:
    key: key of value, fomat: table.list
    value: value for delete
    position: new position
eg:
    move table.xxx 11111 3
        '''
        try:
            key = args[0]
            value = args[1]
            pos = int(args[2])
        except:
            print 'argument error! args: key'
            return
        # key
        ret = self.m.move(key, value, pos)
        if ret != MEMLINK_OK:
            print 'error:', ret, errors.get(ret)
            return
        print 'ok', ret

    def cmd_tag(self, args):
        '''
\33[32mtag\tkey value flag\33[0m
description:
    move value to another position
arguments:
    key: key of value, fomat: table.list
    value: value for delete
    flag: tagdel(d)/restore(r)
eg:
    tag table.xxx 1111 d
    tag table.xxx 2222 r
        '''
        try:
            key = args[0]
            value = args[1]
            flag = self.value_tag_map[args[2]]
        except:
            print 'argument error! args: key'
            return
        # key
        ret = self.m.tag(key, value, flag)
        if ret != MEMLINK_OK:
            print 'error:', ret, errors.get(ret)
            return
        print 'ok', ret

    def cmd_attr(self, args):
        '''
\33[32mattr\tkey value attr\33[0m
description:
    change value's attribute
arguments:
    key: key of value, fomat: table.list
    value: value for delete
    attr: new attribute
eg:
    attr table.xxx 2222 3:2:1
        '''
        try:
            key = args[0]
            value = args[1]
            attr = args[2]
        except:
            print 'argument error! args: key'
            return
        # key
        ret = self.m.attr(key, value, attr)
        if ret != MEMLINK_OK:
            print 'error:', ret, errors.get(ret)
            return
        print 'ok', ret

    def cmd_count(self, args):
        '''
\33[32mcount\tkey [attribute]\33[0m
description:
    value in key information
arguments:
    key: key of value, fomat: table.list
    attribute: attribute filter for value
eg:
    count table.abc
        '''
        try:
            key = args[0]
            attr = ''
            if len(args) == 2:
                attr = args[1]
        except:
            print 'argument error! args: key'
            return
        # key
        ret, obj = self.m.count(key, attr)
        if ret != MEMLINK_OK:
            print 'error:', ret, errors.get(ret)
            return
        print 'ok', ret
        print obj



    def cmd_stat(self, args):
        '''
\33[32mstat\tkey\33[0m
description:
    list stat
arguments:
    key: key of value, fomat: table.list
eg:
    stat table.abc
        '''
        try:
            key = args[0]
        except:
            print 'argument error! args: key'
            return
        # key
        ret, obj = self.m.stat(key)
        if ret != MEMLINK_OK:
            print 'error:', ret, errors.get(ret)
            return
        print 'ok', ret
        print obj

    def cmd_statsys(self, args):
        '''
\33[32mstat\t\33[0m
description:
    stat all
arguments:
eg:
    statsys
        '''
        ret, obj = self.m.stat_sys()
        if ret != MEMLINK_OK:
            print 'error:', ret, errors.get(ret)
            return
        print 'ok', ret
        print obj

    def cmd_dump(self, args):
        '''
\33[32mdump\t\33[0m
description:
    dump data to disk
arguments:
eg:
    dump
        '''
        ret = self.m.dump()
        if ret != MEMLINK_OK:
            print 'error:', ret, errors.get(ret)
            return
        print 'ok', ret

    def cmd_clean(self, args):
        '''
\33[32mclean\tkey\33[0m
description:
    rebuild list for removing null value
arguments:
    key: key of value, fomat: table.list
eg:
    clean table.abc
        '''
        try:
            key = args[0]
        except:
            print 'argument error! args: key'
            return
        # key
        ret = self.m.clean(key)
        if ret != MEMLINK_OK:
            print 'error:', ret, errors.get(ret)
            return
        print 'ok', ret

    def cmd_cleanall(self, args):
        '''
\33[32mcleanall\t\33[0m
description:
    clean all tables
arguments:
eg:
    cleanall
        '''
        ret = self.m.cleanall()
        if ret != MEMLINK_OK:
            print 'error:', ret, errors.get(ret)
            return
        print 'ok', ret



def main():
    cli = MemLinkCli()
    cli.run()

if __name__ == '__main__':
    main()

