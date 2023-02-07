#pragma once
#include <iostream>

inline void ExitWithError(const std::string &error_msg) {
  std::cerr << "ERROR: " << error_msg << '\n';
  exit(1);
}