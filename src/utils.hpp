#pragma once

#include <filesystem>

#include <functional>

#include <string>
#include <map>

typedef std::string str;

std::vector<str> tokenize(const str &s);

str stringify(const std::vector<str> &tkns, int x);
inline str stringify(const std::vector<str> &tkns) { return stringify(tkns, 0); }

str trim(const str &s);
str echofi(const str &s);
std::pair<str, std::vector<str>> get_cmd(const str &s);

bool isExecutable(std::filesystem::path &item);
str get_executable_path(const str &cmd);

class command_runner
{
    // Built_In commands
    static void exit(std::vector<str> &args);
    static void echo(std::vector<str> &args);
    static void type(std::vector<str> &args);
    static void pwd(std::vector<str> &args);
    static void cd(std::vector<str> &args);

public:
    static bool isActive;

    static std::map<str, std::function<void(std::vector<str> &)>> cmd_map;

    static bool repl();
    static void setup();
};