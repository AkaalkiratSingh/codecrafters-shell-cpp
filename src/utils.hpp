#pragma once
#include <string>
#include <sstream>
#include <vector>

std::string trim(std::string &s);
std::vector<std::string> split(std::string &s);
std::pair<std::string,std::string> get_cmd(std::string &s);