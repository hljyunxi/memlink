# coding: utf-8
import os, sys
import struct
import getopt

onlyheader = False

def binlog(filename='bin.log'):
    global onlyheader
    f = open(filename, 'rb')

    s = f.read(2 + 4 + 1 + 4)
    v = []
    v.extend(struct.unpack('h', s[:2]))
    v.extend(struct.unpack('I', s[2:6]))
    v.extend(struct.unpack('B', s[6]))
    v.extend(struct.unpack('I', s[7:]))
    d = f.tell() + 4 * v[-1]
    v.append(d)

    print '====================== bin log   ========================='
    #print 'head:', repr(s)
    print 'format:%d, logver:%d, role:%d, index:%d, data:%d' % tuple(v)
    
    indexes = []
    while True:
        ns = f.read(4)
        v = struct.unpack('I', ns)
        if v[0] == 0:
            break
        else:
            indexes.append(v[0])

    if onlyheader:
        print 'index:', len(indexes)
        return

    if indexes:
        dlen = len(indexes)
        print 'index:', dlen, indexes
        f.seek(4000011)
        for i in range(0, dlen):
            log_ver = struct.unpack('I', f.read(4))
            log_pos = struct.unpack('I', f.read(4))
            s1 = f.read(2)
            slen = struct.unpack('H', s1)[0]
            s2 = f.read(slen)
            s = s1 + s2 
            print 'ver:%d, i:%d, %d:' % \
                (log_ver[0], log_pos[0], len(s)), repr(s), f.tell()
    f.close()


def dumpfile(filename):
    global onlyheader
    if not os.path.isfile(filename):
        print 'not found:', filename
        return
    f = open(filename, "rb") 
    headstr = f.read(2 + 4 + 4 + 1 + 8)
    dformat = struct.unpack('H', headstr[:2])[0]
    dfver, dlogver = struct.unpack('II', headstr[2:10]) 
    role = struct.unpack('B', headstr[10])[0]
    size = struct.unpack('Q', headstr[11:])[0]
    print '====================== dump file ========================='
    print 'format:%d, dumpver:%d, logver:%d, role:%d, size:%d' % (dformat, dfver, dlogver, role, size)

    if onlyheader:
        return

    while True:
        klenstr = f.read(1)
        if not klenstr:
            break
        # key len (1B)
        klen = struct.unpack('B', klenstr)[0]
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
        
        print 'key:%s, valuelen:%d, masklen:%d, masknum:%d, maskformat:%s, itemnum:%d' % (key, valuelen, masklen, masknum, maskformat, itemnum)

        datalen  = valuelen + masklen

        if itemnum > 0:
            for i in xrange(0, itemnum):
                s = f.read(datalen)
                print '\t' + repr(s)
        
    f.close()

def show_help():
    print 'usage:'
    print '\tpython check.py [options]'
    print 'options:'
    print '\t-b binlog file name'
    print '\t-d dump file name'
    print '\t-h only print header'

def main():
    global onlyheader
    binlog_filename = ''
    dump_filename   = ''

    try:
        opts, args = getopt.getopt(sys.argv[1:], 'b:d:h')
    except:
        show_help()
        return

    for opt,arg in opts:
        if opt == '-b':
            binlog_filename = arg
        elif opt == '-d':
            dump_filename = arg
        elif opt == '-h':
            onlyheader = True
           
    if not binlog_filename and not dump_filename:
        show_help()
        return
        
    if binlog_filename:
        binlog(binlog_filename) 
    
    if dump_filename:
        dumpfile(dump_filename)

if __name__ == '__main__':
    main()

