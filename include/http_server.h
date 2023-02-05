#pragma once

struct HttpServer {
  int tcp_socket;
  int port;
  bool success;

  HttpServer(int port);
};