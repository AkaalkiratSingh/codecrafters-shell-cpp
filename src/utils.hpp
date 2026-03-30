#pragma once

#include <filesystem>

#include <functional>

#include <string>
#include <map>

using str = std::string;

str trim(const str &s);
std::vector<str> split(const str &s);
str echofi(const str &s);
std::pair<str, str> get_cmd(const str &s);

bool isExecutable(std::filesystem::path &item);
str get_executable_path(const str &cmd);

class command_runner
{
    // Built_In commands
    static void exit(str &input);
    static void echo(str &input);
    static void type(str &input);
    static void pwd(str &input);
    static void cd(str &input);

public:
    static bool isActive;

    static std::map<str, std::function<void(str &)>> cmd_map;

    static bool repl();
    static void setup();
};