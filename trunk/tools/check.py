# coding: utf-8
import os, sys
import struct
import getopt

onlyheader = False
restore = False

def binlog(filename='bin.log'):
    global onlyheader
    f = open(filename, 'rb')

    s = f.read(2 + 4 + 4)
    v = []
    v.extend(struct.unpack('h', s[:2]))
    v.extend(struct.unpack('I', s[2:6]))
    v.extend(struct.unpack('I', s[6:]))
    maxdata = v[-1]
    d = f.tell() + 4 * v[-1]
    v.append(d)

    print '====================== bin log   ========================='
    #print 'head:', repr(s)
    print 'format:%d, logver:%d, index:%d, data:%d' % tuple(v)
    
    indexes = []
    filelen = 0
    rdc = 0
    while rdc < maxdata * 4:
        ns = f.read(4)
        v = struct.unpack('I', ns)
        #if v[0] == 0:
        #    break
        #else:
        if v[0] > 0:
            indexes.append(v[0])
        rdc += 4
        
    if onlyheader:
        print 'index:', len(indexes)
        return

    if indexes:
        dlen = len(indexes)
        print 'index:', dlen, indexes
        f.seek(d)
        #for i in range(0, dlen):
        while True:
            s1 = f.read(8)
            if not s1 or len(s1) != 8:
                break
            log_ver = struct.unpack('I', s1[0:4])
            log_pos = struct.unpack('I', s1[4:])
            s1 = f.read(4)
            slen = struct.unpack('I', s1)[0]
            s2 = f.read(slen)
            s = s1 + s2 
            print 'ver:%d, ln:%d, %d:' % \
                (log_ver[0], log_pos[0], len(s)), repr(s), f.tell()
    f.close()


def dumpfile(filename):
    global onlyheader
    if not os.path.isfile(filename):
        print 'not found:', filename
        return
    f = open(filename, "rb") 
    headstr = f.read(2 + 4 + 4 + 4 + 8)
    dformat = struct.unpack('H', headstr[:2])[0]
    dfver, dlogver, dlogpos = struct.unpack('III', headstr[2:14]) 
    size = struct.unpack('Q', headstr[14:])[0]
    print '====================== dump file ========================='
    print 'format:%d, dumpver:%d, logver:%d, logpos:%d, size:%d' % (dformat, dfver, dlogver, dlogpos, size)

    if onlyheader:
        return

    while True:
        ltypestr = f.read(1)
        if not ltypestr:
            break
        # list type
        listtype = struct.unpack('B', ltypestr)[0]
        # value sort field
        sortfield = struct.unpack('B', f.read(1))[0]
        # value type 
        valuetype = struct.unpack('B', f.read(1))[0]
        # key len (1B)
        klen = struct.unpack('B', f.read(1))[0]
        # key string (keylen B)
        key  = struct.unpack(str(klen) + 's', f.read(klen))[0]
        # valuesize (1B)
        valuelen = struct.unpack('B', f.read(1))[0]
        # masksize (1B)
        masklen  = struct.unpack('B', f.read(1))[0]
        # masknum (1B)
        masknum  = struct.unpack('B', f.read(1))[0]
        # maskformat (masknum B)
        s = f.read(masknum)
        maskformat = []
        for i in range(0, masknum):
            maskformat.append(struct.unpack('B', s[i])[0]) 

        itemnum  = struct.unpack('I', f.read(4))[0] 
        
        print 'listtype:%d, valuetype:%d, sortfield:%d, key:%s, valuelen:%d, masklen:%d, masknum:%d, maskformat:%s, itemnum:%d' % \
                (listtype, valuetype, sortfield, key, valuelen, masklen, masknum, maskformat, itemnum)

        datalen  = valuelen + masklen

        if itemnum > 0:
            for i in xrange(0, itemnum):
                s = f.read(datalen)
                print '\t' + repr(s)
        
    f.close()

def commitfile(filename):
    print '====================== commit log   ========================='
    f = open(filename, 'rb')
    # lastver, lastline, lastret, logver, logline
    headstr = f.read(4 + 4 + 4 + 4 + 4) 
    lastver, lastline, lastret, logver, logline = struct.unpack('IIIII', headstr)
    print 'lastver:%d, lastline:%d, lastret:%d, logver:%d, logline:%d' % \
                (lastver, lastline, lastret, logver, logline)
    
    cmdlen = struct.unpack('I', f.read(4))[0]
    print 'cmd len:%d, cmd:%d' % (cmdlen, struct.unpack('b', f.read(1))[0])

    f.close()

def checklog(filename):
    f = open(filename, 'rb+')

    f.seek(0, 2)
    filesize = f.tell()
    f.seek(0)
    s = f.read(2 + 4 + 4)
    v = []
    v.extend(struct.unpack('h', s[:2]))
    v.extend(struct.unpack('I', s[2:6]))
    v.extend(struct.unpack('I', s[6:]))
    maxdata = v[-1]
    d = f.tell() + 4 * v[-1]
    v.append(d)

    print '====================== check binlog   ========================='
    #print 'head:', repr(s)
    #print 'format:%d, logver:%d, index:%d, data:%d' % tuple(v)
    print 'filesize: ', filesize
    minfilesize = 2 + 4 + 4 + maxdata * 4
    if filesize == minfilesize:
        print 'log size ok!'
        return 0
    elif filesize < minfilesize:
        print 'log size too small'
        return -1
        
    last_data_pos = 0
    last_index_offset = 0
    rdc = 0
    indexnum = 0
    while rdc < maxdata * 4:
        ns = f.read(4)
        v = struct.unpack('I', ns)
        if v[0] == 0: #the index after the last index
            f.seek(-8, 1) #move to the last index
            last_index_offset = f.tell() 
            ns = f.read(4)
            llen = struct.unpack('I', ns)
            last_data_pos = llen[0] #pos of the last cmd
            break
        indexnum += 1
        rdc += 4
        
    if rdc == maxdata * 4: # if the index zone is full
        f.seek(-4, 1) #move to the last index
        last_index_offset = f.tell() 
        ns = f.read(4)
        llen = struct.unpack('I', ns)
        last_data_pos = llen[0] #pos of the last cmd
        
    print 'last_index_pos: %d' % (indexnum,)
    flag = 0
    lllen = 0
    print 'last_data_pos: ', last_data_pos
    while 1:
        f.seek(last_data_pos, 0)
        s1 = f.read(8)
        if not s1 or len(s1) != 8:
            flag = 1
            break;
        s2 = f.read(4)
        if not s2 or len(s2) != 4:
            flag = 1
            break
        slen = struct.unpack('I', s2)[0]
        lllen = last_data_pos + 8 + 4 + slen
        print 'last_data_pos + last_cmd_len: ', lllen
        if filesize == lllen:
            flag = 0
            break
        if filesize > lllen:
            if indexnum >= maxdata:
                print 'log index if full, but still have data, just truncate the tail.'
                restore = True
                flag = 2
                break
            else:
                print 'revise log index: %d' % (indexnum,)
                indexnum += 1
                f.seek(last_index_offset+4, 0)
                data = struct.pack("I", lllen)
                f.write(data)
                last_index_offset += 4
                last_data_pos = lllen
        else:
            flag = 1
            break;
            
    if 0 == flag:
        print 'log size ok!'
    elif 1 == flag:
        print 'log size is smaller than expected!'
    elif 2 == flag:
        print 'log size is larger than expected!'
        if restore == True:
            print 'restore the file!'
            f.truncate(lllen)
        
    f.close()
    
def show_help():
    print 'usage:'
    print '\tpython check.py [options]'
    print 'options:'
    print '\t-b bin.log file path'
    print '\t-d dump.dat file path'
    print '\t-m commit.log file path'
    print '\t-h only print header'
    print '\t-c check bin.log data'
    print '\t-r repair bin.log'

def main():
    global onlyheader
    global restore

    binlog_filename = ''
    dump_filename   = ''
    commit_filename = ''

    try:
        opts, args = getopt.getopt(sys.argv[1:], 'b:d:m:c:hr')
    except:
        show_help()
        return

    flag = False
    for opt,arg in opts:
        if opt == '-b':
            binlog_filename = arg
        elif opt == '-d':
            dump_filename = arg
        elif opt == '-m':
            commit_filename = arg
        elif opt == '-h':
            onlyheader = True
        elif opt == '-c':
            flag = True
            binlog_filename = arg
        elif opt == '-r':
            restore = True

    if not binlog_filename and not dump_filename and not commit_filename:
        show_help()
        return

    if binlog_filename and flag:
        checklog(binlog_filename)
        return
        
    if binlog_filename:
        binlog(binlog_filename) 
    
    if dump_filename:
        dumpfile(dump_filename)

    if commit_filename:
        commitfile(commit_filename)

if __name__ == '__main__':
    main()

