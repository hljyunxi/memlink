# coding: utf-8
import os, sys
import socket
from errono import EINTR
import struct

CMD_SYNC        100 
CMD_GETDUMP     101

class SyncServer:
    def __init__(self, host, port):
        self.host = host
        self.port = port

        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.bind((host, port))
        self.listen(32)


    def loop(self):
        while True:
            try:
                newsock = self.sock.accept()
            except socket.error, why:
                if why.args[0] == EINTR:
                    continue
                raise
                
            self.run(newsock)
            newsock.close()
    
    def run(self, sock):
        while (True):
            dlen, cmd, param1, param2 = self.recv_cmd(sock)
            print 'recv:', dlen, cmd, param1, param2

            if (cmd == CMD_SYNC){
                self.send_sync_cmd() 
            }else{
                self.send_getdump_cmd()
            }

    def recv_cmd(self, sock):
        hlen = sock.recv(2)
        dlen = struct.unpack('H', hlen)
        print 'head len:', dlen



    def send_sync_cmd(self, sock):
        pass

    def send_getdump_cmd(self, sock):
        pass

    def decode(self, data):
        '''datalen(2B) + cmd(1B) + logver/dumpver(4B) + logline(4B)/size(8B)'''
        dlen, cmd = struct.unpack('HB', data)
        
        if cmd == CMD_SYNC:
            logver, logline = struct.unpack('II', data[3:])
            print 'decode dlen:%d, cmd:%d, logver:%d, logline:%d' % (dlen, cmd, logver, logline)

            return dlen, cmd, logver, logline
        elif cmd == CMD_GETDUMP:
            dumpver, size = struct.unpack('II', data[3:])
            print 'decode dlen:%d, cmd:%d, dumpver:%d, size:%d' % (dlen, cmd, dumpver, size) 
        
            return dlen, cmd, dumpver, size
        else:
            print 'cmd error!', cmd

    def cmd_sync_encode(self, retcode, dumpver):
        s  = struct.pack("hBH", retcode, 0, dumpver)
        ss = struct.pack('H', len(s)) + s
        
        return ss

    def cmd_getdump_encode(self, retcode, dumpver):
        s  = struct.pack("hBH", retcode, 0, dumpver)
        ss = struct.pack('H', len(s)) + s
        
        return ss



def main():
    ss = SyncServer("127.0.0.1", 11005)
    ss.loop()

if __name__ == '__main__':
    main()





