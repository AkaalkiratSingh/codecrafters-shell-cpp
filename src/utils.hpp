#pragma once

#include <filesystem>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

using str = std::string;

// ── String utilities ─────────────────────────────────────────────────────────

str  trim(const str &s);
str  stringify(const std::vector<str> &tokens, std::size_t from = 0);

/// Split `s` on every occurrence of `delim` (no tokenisation logic).
std::vector<str> split(const str &s, char delim);

// ── Shell tokeniser ───────────────────────────────────────────────────────────

/// A single shell word produced by the tokeniser.
struct Token {
    str  value;
    bool terminated = true;   ///< false → merges with the next token (e.g. a'b' → "ab")
};

/// Tokenise a raw shell input string honouring '', "", and \ escapes.
/// Returns an empty vector for empty / whitespace-only input.
std::vector<Token> tokenize(const str &s);

/// Return only the string values from a token list.
std::vector<str> token_values(const std::vector<Token> &tokens);

// ── Redirect descriptor ───────────────────────────────────────────────────────

struct Redirect {
    enum class Fd { Stdout, Stderr };   /// File Descriptor, what is to be redirected
    Fd   fd = Fd::Stdout;               
    str  target;                        ///< destination file path
    bool append = false;
};

// ── Parsed command ────────────────────────────────────────────────────────────

struct Command {
    str                   name;         ///< argv[0]
    std::vector<str>      args;         ///< argv[1..]
    std::vector<Redirect> redirects;

    void insert(str tkn);
};

/// Parse a semicolon-separated pipeline string into individual Commands.
/// Returns nullopt on a syntax error (message already written to stderr).
std::optional<std::vector<Command>> parse_line(const str &line);

// ── PATH helpers ──────────────────────────────────────────────────────────────

bool isExecutable(const std::filesystem::path &p);
std::optional<str> find_in_path(const str &cmd);

/// Return every executable name that starts with `prefix` (for tab completion).
std::vector<str> completions_for(const str &prefix);

// ── Command runner ────────────────────────────────────────────────────────────

class command_runner {
    static void builtin_exit(std::vector<str> &args);
    static void builtin_echo(std::vector<str> &args);
    static void builtin_type(std::vector<str> &args);
    static void builtin_pwd(std::vector<str> &args);
    static void builtin_cd(std::vector<str> &args);

    public:
    static bool isActive;
    static std::map<str, std::function<void(std::vector<str> &)>> cmd_map;

    static bool repl();
    static void setup();
};
