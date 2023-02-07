#pragma once
#include <iostream>

struct HttpServer {
  int tcp_socket;
  int port;

  HttpServer(const std::string& ip, int port);

  ~HttpServer();
};