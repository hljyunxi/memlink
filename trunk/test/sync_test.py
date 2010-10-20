from socket import *
from struct import *

def cmd(sock, data):
  for param in data:
    sock.send(param)
  response = sock.recv(1024)
  print(repr(response))


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
  # valid
  cmd_sync(sock, 1, 4)
  # invalid index 
  cmd_sync(sock, 1, 4000)
  # invalid log version
  cmd_sync(sock, 2, 4)

def test_cmd_sync_dump(sock):
  cmd_sync_dump(sock, 1, 0)

def main():
  sock = socket(AF_INET, SOCK_STREAM);
  sock.connect(('localhost', 11003));

  test_cmd_sync(sock)
  test_cmd_sync_dump(sock)

  sock.close()

if __name__ == '__main__':
    main()
