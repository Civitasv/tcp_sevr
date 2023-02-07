#include "error.h"
#include "http_server.h"
#include "response.h"
#include "routes.h"
#include <cstdio>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>

namespace {
constexpr int BUFFER_SIZE = 4096;
}

int main() {
  // init server
  HttpServer server("127.0.0.1", 8088);

  Routes route;
  route.Add("/", "index.html");
  route.Add("/about", "about.html");

  // Print all avaliable routes
  std::cout << route << '\n';

  while (true) {
    char msg[BUFFER_SIZE] = "";
    int client = accept(server.tcp_socket, NULL, NULL);
    if (client == -1) {
      ExitWithError("Server failed to accept incoming connection");
    }

    if (read(client, msg, BUFFER_SIZE) == -1) {
      ExitWithError("Failed to read bytes from client");
    }

    std::cout << msg << '\n';

    // method and route
    std::string ms(msg);
    std::string method, url_route;
    int n = ms.find('\n');
    if (n != std::string::npos) {
      int pos = 0;
      std::string client_http_header = ms.substr(0, n);
      if ((n = client_http_header.find(' ', pos)) != std::string::npos) {
        method = ms.substr(pos, n - pos);
        pos = n + 1;
      }

      if ((n = client_http_header.find(' ', pos)) != std::string::npos) {
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

    send(client, response, sizeof(response), 0);
    close(client);
  }
}