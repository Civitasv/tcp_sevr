#include "http_server.h"
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>

HttpServer::HttpServer(int port) : port(port), success(true) {
  // AF_INET is internet protocol v4 addresses
  // https://man7.org/linux/man-pages/man2/socket.2.html
  tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (tcp_socket == -1) {
    std::cout << "Socket Initialization FAILED: " << tcp_socket << '\n';
    success = false;
    return;
  }

  // https://man7.org/linux/man-pages/man2/bind.2.html
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = INADDR_ANY;

  int val = bind(tcp_socket, (struct sockaddr *)&addr, sizeof(addr));

  if (val == -1) {
    std::cout << "BIND FAILED: " << val << '\n';
    success = false;
    return;
  }

  listen(tcp_socket, 5);

  std::cout << "HTTP Server Initialized" << '\n' << "Port: " << port << '\n';
  success = true;
}