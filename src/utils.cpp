#include "utils.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <sstream>

static constexpr char PATH_DELIM = ':';

void Command::insert(str tkn) {
    if (name.empty()) name = tkn;
    else args.push_back(tkn);
}

str trim(const str& s) {
    /*
    int i = 0, j = s.size() - 1;
    while (std::isspace(s[i])) i++;
    while (std::isspace(s[j])) j--;
    return s.substr(i, j - i + 1);
    */

    // crazy logic that trims s
    auto front = std::find_if_not(s.begin(), s.end(), ::isspace);
    auto back = std::find_if_not(s.rbegin(), s.rend(), ::isspace).base();
    return (front < back) ? str(front, back) : "";
}

std::vector<str> split(const str& s, char delim) {
    std::vector<str> result;
    str current;
    for (char c : s) {
        if (c == delim) {
            result.push_back(current);
            current.clear();
        }
        else {
            current.push_back(c);
        }
    }
    result.push_back(current);
    return result;
}

str stringify(const std::vector<str>& tokens, std::size_t from) {
    if (from >= tokens.size()) return {};
    str result;
    for (auto i = from; i < tokens.size(); ++i) {
        if (i != from) result += ' ';
        result += tokens[i];
    }
    return result;
}

std::vector<Token> tokenize(const str& input) {
    enum class State {
        Whitespace,
        Default,
        SingleQ,
        DoubleQ
    };

    std::vector<Token> result;
    str   cur;
    State state = State::Whitespace;

    auto flush = [&](bool term) {
        if (!result.empty() && !result.back().terminated) {
            result.back().value += cur;
            result.back().terminated = term;
        }
        else if (!cur.empty() || state != State::Whitespace) {
            result.push_back({ cur, term });
        }

        cur.clear();
        };

    std::size_t i = 0;
    while (i < input.size()) {
        char c = input[i];

        switch (state) {
        case State::Whitespace:
            if (c == '\'') {
                state = State::SingleQ;
            }
            else if (c == '"') {
                state = State::DoubleQ;
            }
            else if (c == '\\') {
                state = State::Default;
                cur.push_back(input[++i]);
            }
            else if (!std::isspace(static_cast<unsigned char>(c))) {
                cur.push_back(c);
                state = State::Default;
            }
            else {
                if (!result.empty())
                    result.back().terminated = true;
            }
            break;

        case State::Default:
            if (std::isspace(static_cast<unsigned char>(c))) {
                flush(true);
                state = State::Whitespace;
            }
            else if (c == '\'') {
                flush(false);
                state = State::SingleQ;
            }
            else if (c == '"') {
                flush(false);
                state = State::DoubleQ;
            }
            else if (c == '\\') {
                cur.push_back(input[++i]);
            }
            else {
                cur.push_back(c);
            }
            break;

        case State::SingleQ:
            if (c == '\'') {
                flush(false);
                state = State::Whitespace;
            }
            else {
                cur.push_back(c);
            }
            break;

        case State::DoubleQ:
            if (c == '"') {
                flush(false);
                state = State::Whitespace;
            }
            else if (c == '\\' && i + 1 < input.size() && (input[i + 1] == '\\' || input[i + 1] == '"')) {
                cur.push_back(input[++i]);
            }
            else {
                cur.push_back(c);
            }
            break;
        }
        ++i;
    }

    // Flush any remaining content (handles unclosed quotes gracefully)
    if (!cur.empty() || state == State::SingleQ || state == State::DoubleQ)
        flush(true);

    return result;
}

std::vector<str> token_values(const std::vector<Token>& tokens) {
    std::vector<str> v;
    v.reserve(tokens.size());
    for (const auto& t : tokens) v.push_back(t.value);
    return v;
}

struct RawPiece {
    str  text;
    bool is_redirect_op = false;
    bool append = false;
    Redirect::Fd fd = Redirect::Fd::Stdout;
};
/// Break a sentence into operators and text-segments
/// {a 1>> b 2> c} converts to {[a], ["",redirect,append,Stdout], [b], ["",redirect,Stderr], [c]}
std::vector<RawPiece> split_redirects(const str& seg) {
    std::vector<RawPiece> pieces;
    str buf;

    auto flush_buf = [&]() {
        if (buf.empty()) return;

        pieces.push_back({ buf });
        buf.clear();

        };

    std::size_t i = 0;
    while (i < seg.size()) {

        // 2>
        if (seg[i] == '>' && !buf.empty() && buf.back() == '2') {
            buf.pop_back(); // remove 2

            flush_buf();
            bool append = (i + 1 < seg.size() && seg[i + 1] == '>'); // 2> is 2>>
            if (append) i++;
            pieces.push_back({ "", true, append, Redirect::Fd::Stderr });
        }

        // 1>
        else if (seg[i] == '>' && !buf.empty() && buf.back() == '1') {
            buf.pop_back(); // remove 1

            flush_buf();
            bool append = (i + 1 < seg.size() && seg[i + 1] == '>'); // 1> is 1>>
            if (append) i++;
            pieces.push_back({ "", true, append, Redirect::Fd::Stdout });
        }

        // Detect plain > or >>
        else if (seg[i] == '>') {
            flush_buf();
            bool append = (i + 1 < seg.size() && seg[i + 1] == '>'); // > is >>
            if (append) i++;
            pieces.push_back({ "", true, append, Redirect::Fd::Stdout });
        }

        else {
            buf.push_back(seg[i]);
        }

        i++;
    }

    flush_buf();    // flush the last piece
    return pieces;
}


std::optional<std::vector<Command>> parse_line(const str& line) {
    std::vector<Command> commands;

    for (const str& seg : split(line, ';')) {
        str trimmed = trim(seg);
        if (trimmed.empty()) continue;

        // break into text-segments and redirection-operators
        auto pieces = split_redirects(trimmed);

        Command cmd;
        bool expect_target = false;
        Redirect pending;

        for (auto& piece : pieces) {
            if (piece.is_redirect_op) {
                if (expect_target) {
                    std::cerr << "Syntax error: consecutive redirect operators\n";
                    return std::nullopt;
                }
                pending = Redirect{ piece.fd, {}, piece.append };
                expect_target = true;
            }
            else {
                auto tokens = token_values(tokenize(piece.text));

                if (expect_target) {
                    if (tokens.empty()) {
                        std::cerr << "Syntax error: missing redirect target\n";
                        return std::nullopt;
                    }
                    // First token is the target; any remainder goes back to words
                    pending.target = tokens.front();

                    cmd.redirects.push_back(pending);
                    expect_target = false;
                    for (std::size_t k = 1; k < tokens.size(); ++k) {
                        cmd.insert(tokens[k]);
                    }
                }
                else {
                    for (const auto& t : tokens) {
                        cmd.insert(t);
                    }
                }
            }
        }
        if (expect_target) {
            std::cerr << "Syntax error: redirect with no target\n";
            return std::nullopt;
        }
        if (!cmd.name.empty())
            commands.push_back(std::move(cmd));
    }
    return commands;
}

bool isExecutable(const std::filesystem::path& p) {
    std::error_code ec;
    if (!std::filesystem::is_regular_file(p, ec)) return false;

    auto perms = std::filesystem::status(p, ec).permissions();
    auto exec_mask =
        std::filesystem::perms::owner_exec |
        std::filesystem::perms::group_exec |
        std::filesystem::perms::others_exec;

    return (perms & exec_mask) != std::filesystem::perms::none;

}

static std::vector<std::filesystem::path> path_dirs() {
    const char* env = std::getenv("PATH");

    std::vector<std::filesystem::path> dirs;
    for (const str& d : split(env, PATH_DELIM))
        if (!d.empty()) dirs.emplace_back(d);
    return dirs;
}

std::optional<str> find_in_path(const str& cmd) {
    for (const auto& dir : path_dirs()) {
        auto full = dir / cmd;
        if (isExecutable(full))
            return full.string();
    }
    return std::nullopt;
}

std::vector<str> completions_for(const str& prefix) {
    std::vector<str> matches;
    std::error_code ec;
    
    for (const auto& dir : path_dirs()) {
        for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
            const str name = entry.path().filename().string();
            if (name.rfind(prefix, 0) == 0 && isExecutable(entry.path()))
                matches.push_back(name);
        }
    }

    std::sort(matches.begin(), matches.end());
    matches.erase(std::unique(matches.begin(), matches.end()), matches.end());
    return matches;
}
