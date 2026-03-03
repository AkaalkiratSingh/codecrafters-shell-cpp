#pragma once
#include <string>
#include <sstream>
#include <vector>

std::string trim(const std::string &s);
std::vector<std::string> split(const std::string &s);
std::pair<std::string,std::string> get_cmd(const std::string &s);