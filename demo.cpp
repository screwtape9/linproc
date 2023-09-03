#include <iostream>
#include <vector>
#include <string>
#include "Process.h"

int main(__attribute__((unused)) int argc,
         __attribute__((unused)) char *argv[])
{
  std::string cmd;
  for (int i = 1; i < argc; ++i) {
    if (i > 1)
      cmd += ' ';
    cmd += argv[i];
  }

  std::vector<std::string> lines, errlines;
  Process p;
  if (!p.Run(cmd, lines, errlines)) {
    for (auto str : lines)
      std::cout << str << std::endl;
  }
  else
    std::cerr << p.GetLastErr() << std::endl;
  return EXIT_SUCCESS;
}
