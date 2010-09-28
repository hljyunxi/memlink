# coding: utf-8
import os, sys
import struct

def main():
    f = open("bin.log", 'rb')

    s = f.read(10)
    print 'head:', repr(s), struct.unpack("h", s[:2]), struct.unpack("ll", s[2:])
   
    dlen = 0
    print 'index:', 
    while True:
        ns = f.read(4)
        v = struct.unpack('I', ns)

        if v[0] == 0:
            break
        else:
            dlen += 1
        
        print v,
    print

    f.seek(4000010)
    for i in range(0, dlen):
        s = f.read(23)
        print 'data %02d %d:' % (i+1, len(s)), repr(s), f.tell()

    f.close()

if __name__ == '__main__':
    main()

