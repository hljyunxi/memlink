# coding: utf-8
import os, sys
import struct

def main():
    f = open("bin.log", 'rb')

    s = f.read(10)
    v = []
    v.extend(struct.unpack('h', s[:2]))
    v.extend(struct.unpack('ll', s[2:]))

    print 'head:', repr(s), v
   
    dlen = 0
    indexes = []
    while True:
        ns = f.read(4)
        v = struct.unpack('I', ns)

        if v[0] == 0:
            break
        else:
            dlen += 1
        
        indexes.append(v[0])
    print 'index:', len(indexes), indexes

    f.seek(4000010)
    for i in range(0, dlen):
        s1 = f.read(2)
        slen = struct.unpack('H', s1)[0]
        s2 = f.read(slen)
        s = s1 + s2
        print 'data %02d %d:' % (i+1, len(s)), repr(s), f.tell()

    f.close()

if __name__ == '__main__':
    main()

