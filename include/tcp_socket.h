#pragma once
#include "routes.h"
#include <iostream>
#include <sys/epoll.h>
#include <sys/socket.h>

struct TCPSocket {
  constexpr static int BUFFER_SIZE = 4096;
  constexpr static int THREAD_NUM = 4;
  constexpr static int EVENTS_SIZE = 64;

  int socketfd;
  int port;
  int epollfd;

  epoll_event epevent;
  epoll_event events[EVENTS_SIZE];
  char msg[BUFFER_SIZE];

  TCPSocket(const std::string &ip, int port);

  int Accept(sockaddr *addr, socklen_t *len);

  int Read(int client, void *buf, size_t buffer_size);

  int Write(int client, const void *buf, size_t buffer_size);

  void Run(Routes &route);
  ~TCPSocket();
};