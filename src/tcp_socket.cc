#include "tcp_socket.h"
#include "error.h"
#include "response.h"
#include "routes.h"
#include <arpa/inet.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <fcntl.h>

namespace {
void SetNonBlocking(int fd) {
  int old_opt = fcntl(fd, F_GETFL, 0);
  int new_opt = old_opt | O_NONBLOCK;
  fcntl(fd, F_SETFL, new_opt);
}
} // namespace

TCPSocket::TCPSocket(const std::string &ip, int port) : port(port) {
  // https://man7.org/linux/man-pages/man2/bind.2.html
  sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = inet_addr(ip.c_str());

  // AF_INET is internet protocol v4 addresses
  // https://man7.org/linux/man-pages/man2/socket.2.html
  socketfd = socket(AF_INET, SOCK_STREAM, 0);
  if (socketfd == -1) {
    ExitWithError("Socket Initialization FAILED");
    return;
  }

  int val = bind(socketfd, (sockaddr *)&addr, sizeof(addr));
  if (val == -1) {
    ExitWithError("BIND FAILED");
    return;
  }

  int status = listen(socketfd, 5);
  if (status == -1) {
    ExitWithError("Socket Listening FAILED");
  }

  std::cout << "HTTP Server Initialized" << '\n'
            << "Ip: " << ip << '\n'
            << "Port: " << port << '\n';
  SetNonBlocking(socketfd);

  // create epoll
  epollfd = epoll_create1(0);
  if (epollfd < 0) {
    ExitWithError("Error while creating epoll");
  }

  // let epoll listen
  epevent.events = EPOLLIN | EPOLLET;
  epevent.data.fd = socketfd;
  if (epoll_ctl(epollfd, EPOLL_CTL_ADD, socketfd, &epevent) < 0) {
    ExitWithError("Add error");
  }
}

int TCPSocket::Accept(sockaddr *addr, socklen_t *len) {
  int client = accept(socketfd, addr, len);
  if (client == -1) {
    ExitWithError("Accept error");
  }
  return client;
}

int TCPSocket::Read(int client, void *buf, size_t buffer_size) {
  int len = read(client, buf, buffer_size);
  return len;
}

int TCPSocket::Write(int client, const void *buf, size_t buffer_size) {
  return write(client, buf, buffer_size);
}

void TCPSocket::Run(Routes &route) {
  while (true) {
    int e_num = epoll_wait(epollfd, events, EVENTS_SIZE, -1);
    if (e_num == -1) {
      ExitWithError("Epoll Wait");
    }

    for (int i = 0; i < e_num; i++) {
      if (events[i].data.fd == socketfd) {
        // 说明是 client 的请求
        if (events[i].events & EPOLLIN) {
          sockaddr_in client_addr;
          socklen_t len = sizeof(client_addr);
          int client = Accept((sockaddr *)&client_addr, &len);

          epevent.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
          epevent.data.fd = client;
          SetNonBlocking(client);
          // 将新的连接添加到 epoll 中
          epoll_ctl(epollfd, EPOLL_CTL_ADD, client, &epevent);
        }
      } else {
        if (events[i].events & EPOLLIN) {
          // 说明可读了
          int len = Read(events[i].data.fd, msg, BUFFER_SIZE);
          if (len <= 0) {
            epoll_ctl(epollfd, EPOLL_CTL_DEL, events[i].data.fd, nullptr);
            close(events[i].data.fd);
          } else {
            std::cout << msg << '\n';

            // method and route
            std::string ms(msg);
            std::string method, url_route;
            int n = ms.find('\n');
            if (n != std::string::npos) {
              int pos = 0;
              std::string client_http_header = ms.substr(0, n);
              if ((n = client_http_header.find(' ', pos)) !=
                  std::string::npos) {
                method = ms.substr(pos, n - pos);
                pos = n + 1;
              }

              if ((n = client_http_header.find(' ', pos)) !=
                  std::string::npos) {
                url_route = ms.substr(pos, n - pos);
                pos = n + 1;
              }
            }

            std::cout << "The method is: '" << method << "'" << '\n';
            std::cout << "The route is: '" << url_route << "'" << '\n';

            std::string templates = "";

            if (url_route.find("/static/") != std::string::npos) {
              // strcat(template, urlRoute+1);
              templates += "static/index.css";
            } else {
              templates += "templates/";

              if (!route.Has(url_route)) {
                templates += "404.html";
              } else {
                templates += route.Get(url_route);
              }
            }

            char response[4096] = "HTTP/1.1 200 OK\r\n\r\n";
            std::string page = render_static_file(templates);

            strcat(response, page.data());
            strcat(response, "\r\n\r\n");
            Write(events[i].data.fd, response, sizeof(response));
            epoll_ctl(epollfd, EPOLL_CTL_DEL, events[i].data.fd, nullptr);
            std::cout << "Client out fd: " << events[i].data.fd << '\n';
            close(events[i].data.fd);
          }
        }
      }
    }
  }
}
TCPSocket::~TCPSocket() {
  close(socketfd);
  close(epollfd);
}