#include <iostream>
#include <string>

#include "utils.hpp"

int main()
{
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  // TODO: Uncomment the code below to pass the first stage
  while (true)
  {
    std::cout << "$ ";
    std::string s;
    std::getline(std::cin, s);
    std::string command = split(s)[0];
    std::cerr << command << ": command not found\n";
  }
}
