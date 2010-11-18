# coding: utf-8
import os, sys
import socket
from errno import EINTR
import struct
import time
import traceback

CMD_SYNC    = 100 
CMD_GETDUMP = 101
CMD_DEL     = 5

class SyncServer:
    def __init__(self, host, port):
        self.host = host
        self.port = port

        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.sock.bind((host, port))
        self.sock.listen(32)

        self.dump_filename = "mydump.dat"

    def loop(self):
        while True:
            try:
                newsock, newaddr = self.sock.accept()
            except socket.error, why:
                if why.args[0] == EINTR:
                    continue
                raise
            print 'new conn:', newsock, newaddr
            try:
                self.run(newsock)
            except:
                traceback.print_exc()
            finally:
                newsock.close()
    
    def run(self, sock):
        logver  = 1
        logline = 10

        f = open(self.dump_filename, 'rb')
        dumpdata = f.read()
        f.close()

        dumpformat = struct.unpack("H", dumpdata[:2])[0]
        dumpver    = struct.unpack('I', dumpdata[2:6])[0]
        print 'dump format:%d ver:%d' % (dumpformat, dumpver)

        while (True):
            dlen, cmd, param1, param2 = self.recv_cmd(sock)
            print 'recv len:%d, cmd:%d, %d %d' % (dlen, cmd, param1, param2)

            if cmd == CMD_SYNC:
                if logver == param1 and logline == param2:
                    self.send_sync_cmd(sock, 0) 
                    break
                else:
                    self.send_sync_cmd(sock, 1) 
            else:
                self.send_getdump_cmd(sock, 1, dumpver, len(dumpdata))
                print 'send dump:', sock.send(dumpdata)
                print 'send dump ok!'
                break

        while True:
            print 'send data ...', logver, logline
            self.send_data(sock, logver, logline)
            time.sleep(1)
            logline += 1

    def recv_cmd(self, sock):
        hlen = sock.recv(2)
        dlen = struct.unpack('H', hlen)[0]
        print 'head len:', dlen
        s = sock.recv(dlen)
        ss = hlen + s
        print 'recv:', repr(ss)
        return self.decode(ss)

    def send_sync_cmd(self, sock, ret):
        print 'send sync:', ret
        s  = struct.pack('H', ret)
        ss = struct.pack('I', len(s)) + s
        print 'reply sync:', repr(ss)
        sock.send(ss)

    def send_getdump_cmd(self, sock, ret, dumpver, size):
        print 'send getdump:', ret,  dumpver, size
        s  = struct.pack('H', ret) + struct.pack('I', dumpver) + struct.pack('Q', size)
        ss = struct.pack('I', len(s)) + s 
        print 'reply getdump:', repr(ss)
        sock.send(ss)

    def send_data(self, sock, logver, logline):
        '''logver(4B) + logline(4B) + len(2B) + cmd(1B) + data''' 
        print 'send data:', logver, logline
        s  = struct.pack('BB4sB12s', CMD_DEL, 4, 'haha', 12, 'abcdefghijkl')
        ss = struct.pack('IIH', logver, logline, len(s)) + s
        print 'push:', repr(ss)
        sock.send(ss)

    def decode(self, data):
        '''datalen(2B) + cmd(1B) + logver/dumpver(4B) + logline(4B)/size(8B)'''
        dlen, cmd = struct.unpack('HB', data[:3])
        
        if cmd == CMD_SYNC:
            logver, logline = struct.unpack('II', data[3:])
            print 'decode dlen:%d, cmd:%d, logver:%d, logline:%d' % (dlen, cmd, logver, logline)
            return dlen, cmd, logver, logline
        elif cmd == CMD_GETDUMP:
            dumpver, size = struct.unpack('IQ', data[3:])
            print 'decode dlen:%d, cmd:%d, dumpver:%d, size:%d' % (dlen, cmd, dumpver, size) 
            return dlen, cmd, dumpver, size
        else:
            raise ValueError, 'cmd error:' + cmd

    
def main():
    ss = SyncServer("127.0.0.1", 11005)
    ss.loop()

if __name__ == '__main__':
    main()





