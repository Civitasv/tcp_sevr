#include "routes.h"
#include "tcp_socket.h"
#include <iostream>

int main() {
  // init server
  TCPSocket server("127.0.0.1", 8080);

  // init routes
  Routes route;
  route.Add("/", "index.html");
  route.Add("/about", "about.html");

  // Print all avaliable routes
  std::cout << route << '\n';

  server.Run(route);
}