#pragma once

#include <string>

#include <map>
#include <vector>

#include <optional>
#include <functional>
#include <filesystem>

using str = std::string;


str  trim(const str& s);
str  stringify(const std::vector<str>& tokens, std::size_t from = 0);
std::vector<str> split(const str& s, char delim);


struct Token {
    str  value;
    bool terminated = true;
};
std::vector<Token> tokenize(const str& s);
std::vector<str> token_values(const std::vector<Token>& tokens);

// Container that manages Redirection
// `fd` : File Descriptor -> Stdout / Stderr
struct Redirect {
    enum class Fd { Stdout, Stderr };
    Fd   fd = Fd::Stdout;
    str  target;
    bool append = false;
};

// Container that manages -> command, arguments, redirects
struct Command {
    str                   name;
    std::vector<str>      args;
    std::vector<Redirect> redirects;

    void insert(str tkn);
};

std::optional<std::vector<Command>> parse_line(const str& line);

bool isExecutable(const std::filesystem::path& p);
std::optional<str> find_in_path(const str& cmd);

// Return every executable name that starts with `prefix` (for tab completion).
std::vector<str> completions_for(const str& prefix);

namespace command_runner {
    extern bool isActive;
    extern std::map<str, std::function<void(std::vector<str>&)>> cmd_map;

    bool repl();
    void setup();
}
