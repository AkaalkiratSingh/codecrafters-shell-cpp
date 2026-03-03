#include <iostream>
#include <string>

#include "utils.hpp"

bool repl()
{
  std::cout << "$ ";

  std::string s;
  std::getline(std::cin, s);

  auto [cmd, rest] = get_cmd(s);

  if (cmd == "exit")
    return false;

  if (cmd == "echo")
    std::cout << rest << '\n';

  std::cerr << cmd << ": command not found\n";

  return true;
}

int main()
{
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  // TODO: Uncomment the code below to pass the first stage
  while (repl)
    ;
}
