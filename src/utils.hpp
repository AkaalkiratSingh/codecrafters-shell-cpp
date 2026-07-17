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
std::vector<Token> tokenize(const str& s, bool force_terminate_at_end = true, bool alias_complete = false);
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

std::optional<std::vector<std::vector<Command>>> parse_line(const str& line, bool alias_complete = false);

bool isExecutable(const std::filesystem::path& p);
std::optional<str> find_in_path(const str& cmd);

struct CompletionItem {
    str  full;
    str  display;
    bool is_dir;

    bool operator<(const CompletionItem& other) const;
    bool operator==(const CompletionItem& other) const;
};

// Return every executable name that starts with `prefix` (for tab completion).
std::vector<CompletionItem> completions_for(const str& prefix);
std::vector<CompletionItem> file_completions(const str& prefix);
str longest_common_prefix(const std::vector<CompletionItem>& arr);

namespace command_runner {
    extern bool isActive;
    extern std::map<str, std::function<void(std::vector<str>&)>> cmd_map;
    extern std::map<str, std::vector<str>> alias_map;

    bool repl();
    void setup();
}
