#pragma once
#include "routes.h"
#include <iostream>
#include <sys/epoll.h>
#include <sys/socket.h>

struct TCPSocket {
private:
  constexpr static int BUFFER_SIZE = 4096;
  constexpr static int EVENTS_SIZE = 64;

  int socketfd;
  int port;
  int epollfd;

  epoll_event epevent;
  epoll_event events[EVENTS_SIZE];
  char buf[BUFFER_SIZE];

public:
  TCPSocket(const std::string &ip, int port);

  void Accept();

  std::string Read(int client, char *buf, size_t buffer_size);

  int Write(int client, const char *buf, size_t buffer_size);

  void Run(Routes &route);
  ~TCPSocket();
};