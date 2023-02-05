#include "response.h"
#include <fstream>
#include <ios>
#include <iostream>
#include <sstream>
#include <string>

std::string render_static_file(const std::string &filename) {
  std::ifstream fs;
  std::stringstream ss;

  fs.open(filename);

  if (!fs.is_open()) {
    std::cout << "Cannot find file: " << filename << '\n';
    return "";
  }

  std::string line;
  while (std::getline(fs, line)) {
    ss << line << '\n';
  }
  fs.close();

  return ss.str();
}
