#include "http_server.h"
#include "response.h"
#include "routes.h"
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>

int main() {
  // init server
  HttpServer server(6968);
  if (!server.success) {
    std::cout << "Oops...SERVER INIT FAILED" << '\n';
    return 0;
  }

  Routes route;
  route.Add("/", "index.html");
  route.Add("/about", "about.html");

  // Print all avaliable routes
  std::cout << route << '\n';

  while (true) {
    char msg[4096] = "";
    int client = accept(server.tcp_socket, NULL, NULL);
    read(client, msg, 4095);
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