#pragma once

#include <filesystem>

#include <functional>

#include <string>
#include <sstream>
#include <iostream>

#include <vector>
#include <map>

std::string trim(const std::string &s);
std::vector<std::string> split(const std::string &s);
std::pair<std::string, std::string> get_cmd(const std::string &s);
bool isExecutable(std::filesystem::path &item);

class command_runner
{

    // Built_In commands
    static void exit(std::string &input);
    static void echo(std::string &input);
    static void type(std::string &input);
    static void pwd(std::string &input);

public:
    static bool isActive;

    static std::map<std::string, std::function<void(std::string &)>> cmd_map;

    static bool repl();
    static void setup();
};