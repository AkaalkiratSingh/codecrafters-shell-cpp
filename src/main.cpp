#include "utils.hpp"

#include <iostream>

int main()
{
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  command_runner::setup();
  while (command_runner::repl())
    ;
}