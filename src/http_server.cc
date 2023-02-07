#include "http_server.h"
#include "error.h"
#include <arpa/inet.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

HttpServer::HttpServer(const std::string &ip, int port) : port(port) {
  // https://man7.org/linux/man-pages/man2/bind.2.html
  sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = inet_addr(ip.c_str());

  // AF_INET is internet protocol v4 addresses
  // https://man7.org/linux/man-pages/man2/socket.2.html
  tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (tcp_socket == -1) {
    ExitWithError("Socket Initialization FAILED");
    return;
  }

  int val = bind(tcp_socket, (sockaddr *)&addr, sizeof(addr));
  if (val == -1) {
    ExitWithError("BIND FAILED");
    return;
  }

  int fd = listen(tcp_socket, 5);
  if (fd == -1) {
    ExitWithError("Socket Listening FAILED");
  }

  std::cout << "HTTP Server Initialized" << '\n'
            << "Ip: " << ip << '\n'
            << "Port: " << port << '\n';
}

HttpServer::~HttpServer() {
  if (tcp_socket != -1) {
    close(tcp_socket);
  }
}