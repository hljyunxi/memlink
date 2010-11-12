# coding: utf-8
import os, sys
import struct

def binlog(filename='bin.log'):
    f = open(filename, 'rb')

    s = f.read(2 + 4 + 1 + 4)
    v = []
    v.extend(struct.unpack('h', s[:2]))
    v.extend(struct.unpack('I', s[2:6]))
    v.extend(struct.unpack('B', s[6]))
    v.extend(struct.unpack('I', s[7:]))

    print '====================== bin log   ========================='
    #print 'head:', repr(s)
    print 'format:%d, logver:%d, role:%d, index:%d' % tuple(v)

    indexes = []
    while True:
        ns = f.read(4)
        v = struct.unpack('I', ns)
        if v[0] == 0:
            break
        else:
            indexes.append(v[0])

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
            print 'logver: %d, logpos: %d, data %02d %d:' % \
                (log_ver[0], log_pos[0], i+1, len(s)), repr(s), f.tell()
    f.close()


def dumpfile():
    if not os.path.isfile('dump.dat'):
        return
    f = open("dump.dat", "rb") 
    headstr = f.read(2 + 4 + 4 + 1 + 8)
    dformat = struct.unpack('H', headstr[:2])[0]
    dfver, dlogver = struct.unpack('II', headstr[2:10]) 
    role = struct.unpack('B', headstr[10])[0]
    size = struct.unpack('Q', headstr[11:])[0]
    print '====================== dump file ========================='
    print 'format:%d, dumpver:%d, logver:%d, role:%d, size:%d' % (dformat, dfver, dlogver, role, size)
    f.close()

def main():
    if len(sys.argv) > 1:
        binlog('bin.log.' + sys.argv[1])
    else: 
        binlog()
    dumpfile()


if __name__ == '__main__':
    main()

