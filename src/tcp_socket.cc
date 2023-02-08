#include "tcp_socket.h"
#include "error.h"
#include "response.h"
#include "routes.h"
#include <arpa/inet.h>
#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

namespace {
void SetNonBlocking(int fd) {
  int old_opt = fcntl(fd, F_GETFL, 0);
  int new_opt = old_opt | O_NONBLOCK;
  fcntl(fd, F_SETFL, new_opt);
}
} // namespace

TCPSocket::TCPSocket(const std::string &ip, int port)
    : port(port), socketfd(-1), epollfd(-1), epevent({}), events(), buf("") {
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

void TCPSocket::Accept() {
  while (true) {
    sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);
    int client = accept(socketfd, (sockaddr *)&client_addr, &len);
    if (client < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        break;
      } else {
        ExitWithError("Accept error");
      }
    }

    epevent.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    epevent.data.fd = client;
    SetNonBlocking(client);
    // 将新的连接添加到 epoll 中
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, client, &epevent) < 0) {
      ExitWithError("Add error");
    }
  }
}

std::string TCPSocket::Read(int client, char *buf, size_t buffer_size) {
  std::stringstream ss;
  while (true) {
    int len = read(client, buf, BUFFER_SIZE);
    if (len < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        break;
      } else {
        ExitWithError("Error While Reading");
      }
    } else if (len == 0) {
      // 表示读取完毕
      break;
    }
    ss << buf;
    if (len < BUFFER_SIZE) {
      break;
    }
  }
  return ss.str();
}

int TCPSocket::Write(int client, const char *buf, size_t buffer_size) {
  size_t left = buffer_size;
  ssize_t written;
  const char *bufp = (const char *)buf;

  while (left > 0) {
    if ((written = write(client, bufp, left)) <= 0) {
      return 0;
    }
    left -= written;
    bufp += written;
  }

  return buffer_size;
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
          std::cout << "============ CONNECT ===============" << '\n';
          Accept();
          std::cout << "============ CONNECT FINISHED ===============" << '\n';
        }
      } else {
        if (events[i].events & EPOLLIN) {
          int client = events[i].data.fd;
          std::string msg = Read(client, buf, BUFFER_SIZE);
          std::cout << "============= REQUEST INFO ============== " << '\n';
          std::cout << msg << '\n';
          std::cout << "============= REQUEST INFO ============== " << '\n';

          std::cout << "============= REQUEST START ============== " << '\n';
          // method and route
          std::string method, url_route;
          int n = msg.find('\n');
          if (n != std::string::npos) {
            int pos = 0;
            std::string client_http_header = msg.substr(0, n);
            if ((n = client_http_header.find(' ', pos)) != std::string::npos) {
              method = msg.substr(pos, n - pos);
              pos = n + 1;
            }

            if ((n = client_http_header.find(' ', pos)) != std::string::npos) {
              url_route = msg.substr(pos, n - pos);
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

          std::string response = "HTTP/1.1 200 OK\r\n\r\n";
          std::string page = render_static_file(templates);

          response += page.data();
          response += "\r\n\r\n";

          int writelen = Write(events[i].data.fd, response.c_str(),
                               response.size() * sizeof(char));
          if (writelen < response.size()) {
            ExitWithError("Error while sending response");
          }
          epoll_ctl(epollfd, EPOLL_CTL_DEL, events[i].data.fd, nullptr);
          close(events[i].data.fd);

          std::cout << "============= REQUEST END ============== " << '\n';
        }
      }
    }
  }
}

TCPSocket::~TCPSocket() {
  close(socketfd);
  close(epollfd);
}