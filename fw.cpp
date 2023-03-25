#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <cstdio>
#include <cstdlib>
#include "Process.h"

static bool GetFWVersion(uint16_t vid, uint16_t pid, std::string& fw)
{
  char cmd[512] = { '\0' };
  snprintf(cmd, sizeof(cmd), "/usr/bin/lsusb -vd %04x:%04x", vid, pid);

  std::vector<std::string> lines;
  Process p;
  if (!p.Run(cmd, lines)) {
    for (auto const& line : lines) {
      if (line.find("bcdDevice") != std::string::npos) {
        std::istringstream ss(line);
        std::string token;
        if ((ss >> token) && (ss >> token)) {
          fw = token;
          return true;
        }
      }
    }
  }
  return false;
}

int main(__attribute__((unused)) int argc,
         __attribute__((unused)) char *argv[])
{
  std::string fw;
  char *ptr = nullptr;
  uint16_t vid = strtoul(argv[1], &ptr, 16);
  uint16_t pid = strtoul(argv[2], &ptr, 16);
  if (GetFWVersion(vid, pid, fw))
    std::cout << "fw: " << fw << std::endl;
  else
    std::cerr << "Could not find firmware version for "
              << argv[1] << ':' << argv[2] << std::endl;
  return EXIT_SUCCESS;
}
