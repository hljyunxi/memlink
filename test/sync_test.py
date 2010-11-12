from socket import *
from struct import *
import time

def cmd(sock, data):
  for param in data:
    sock.send(param)

  header = sock.recv(4)
  length = unpack('I', header)[0]
  data = sock.recv(length)
  retcode = unpack('H', data[:2])[0]
  print 'length: %d, retcode: %d, ' % (length, retcode), repr(header + data)

  if retcode == 0:
      for a in range(2): 
          log_ver = unpack('I', sock.recv(4))[0]
          log_pos = unpack('I', sock.recv(4))[0]
          log_record = sock.recv(2)
          length = unpack('H', log_record)[0]
          log_record += sock.recv(length)
          print 'logver: %d, logpos: %d, length: %d' % \
                        (log_ver, log_pos, length), repr(log_record);

def cmd_sync(sock, log_ver, log_index):
  data = [pack('H', 9),
          pack('B', 100),
          pack('II', log_ver, log_index)]
  cmd(sock, data)

def cmd_sync_dump(sock, dumpver, size):
  data = [pack('H', 13),
          pack('B', 101),
          pack('I', dumpver),
          pack('Q', size)]
  cmd(sock, data)

def test_cmd_sync(sock):
  # invalid index 
  cmd_sync(sock, 1, 4000)
  # invalid log version
  cmd_sync(sock, 2, 4)
  # valid
  cmd_sync(sock, 1, 30)

def test_cmd_sync_dump(sock):
  cmd_sync_dump(sock, 1, 0)

def main():
  sock = socket(AF_INET, SOCK_STREAM);
  sock.connect(('localhost', 11003));

  test_cmd_sync(sock)
  #test_cmd_sync_dump(sock)

  sock.close()

if __name__ == '__main__':
    main()
