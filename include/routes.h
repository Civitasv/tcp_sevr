#pragma once
#include <ostream>
#include <string>
#include <unordered_map>

struct Routes {
  std::unordered_map<std::string, std::string> routes;

  void Add(const std::string &k, const std::string &v) { routes[k] = v; }

  bool Has(const std::string &k) { return routes.find(k) != routes.end(); }

  std::string Get(const std::string &k) { return routes[k]; }

  friend std::ostream &operator<<(std::ostream &os, Routes &route) {
    for (auto &[k, v] : route.routes) {
      os << k << " : " << v << '\n';
    }
    return os;
  }
};