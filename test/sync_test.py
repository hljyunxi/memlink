from socket import *
from struct import *

def test(sock, log_ver, log_index):
  sock.send(pack('H', 9))
  sock.send(pack('B', 100))
  sock.send(pack('II', log_ver, log_index))
  response = sock.recv(1024)
  print(repr(response))

def main():
  sock = socket(AF_INET, SOCK_STREAM);
  sock.connect(('localhost', 11003));

  # valid
  test(sock, 1, 4)

  # invalid index 
  test(sock, 1, 4000)

  # invalid log version
  test(sock, 2, 4)

  sock.close()

if __name__ == '__main__':
    main()
