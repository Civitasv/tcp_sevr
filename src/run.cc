#include "error.h"
#include "http_server.h"
#include "response.h"
#include "routes.h"
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

namespace {
constexpr int BUFFER_SIZE = 4096;
constexpr int THREAD_NUM = 4;
constexpr int EVENTS_SIZE = 64;

void SetNonBlocking(int fd) {
  int old_opt = fcntl(fd, F_GETFL, 0);
  int new_opt = old_opt | O_NONBLOCK;
  fcntl(fd, F_SETFL, new_opt);
}
} // namespace

int main() {
  // init server
  HttpServer server("127.0.0.1", 8080);
  SetNonBlocking(server.tcp_socket);

  // create epoll
  int epollfd = epoll_create1(0);
  if (epollfd < 0) {
    ExitWithError("Error while creating epoll");
  }

  // let epoll listen
  epoll_event epevent;
  epevent.events = EPOLLIN | EPOLLET;
  epevent.data.fd = server.tcp_socket;
  if (epoll_ctl(epollfd, EPOLL_CTL_ADD, server.tcp_socket, &epevent) < 0) {
    ExitWithError("Add error");
  }

  // epoll 中有响应事件时，通过该数组返回
  epoll_event events[EVENTS_SIZE];

  Routes route;
  route.Add("/", "index.html");
  route.Add("/about", "about.html");

  // Print all avaliable routes
  std::cout << route << '\n';

  char msg[BUFFER_SIZE] = "";

  while (true) {
    int e_num = epoll_wait(epollfd, events, EVENTS_SIZE, -1);
    if (e_num == -1) {
      ExitWithError("Epoll Wait");
    }

    for (int i = 0; i < e_num; i++) {
      if (events[i].data.fd == server.tcp_socket) {
        // 说明是 client 的请求
        if (events[i].events & EPOLLIN) {
          sockaddr_in client_addr;
          socklen_t len = sizeof(client_addr);
          int client =
              accept(server.tcp_socket, (sockaddr *)&client_addr, &len);
          if (client != -1) {
            epevent.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
            epevent.data.fd = client;
            SetNonBlocking(client);
            // 将新的连接添加到 epoll 中
            epoll_ctl(epollfd, EPOLL_CTL_ADD, client, &epevent);
          } else {
            ExitWithError("Accept error");
          }
        }
      } else {
        if (events[i].events & EPOLLIN) {
          // 说明可读了
          int len = read(events[i].data.fd, msg, BUFFER_SIZE);
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
            write(events[i].data.fd, response, sizeof(response));
            epoll_ctl(epollfd, EPOLL_CTL_DEL, events[i].data.fd, nullptr);
            std::cout << "Client out fd: " << events[i].data.fd << '\n';
            close(events[i].data.fd);
          }
        }
      }
    }
  }
  close(epollfd);
}