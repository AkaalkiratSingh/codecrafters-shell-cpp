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

std::vector<Token> tokenize(const str& input, bool force_terminate_at_end, bool alias_complete) {
    enum class State {
        Whitespace,
        Default,
        SingleQ,
        DoubleQ
    };

    auto isCommandBoundary = [](char c) {return c == '|' || c == ';';};

    std::vector<Token> result;
    str   cur;
    State state = State::Whitespace;

    auto flush = [&](bool term) {

        if (alias_complete && command_runner::alias_map.contains(cur)) {
            for (str w : command_runner::alias_map[cur])
                result.push_back({ w,true });
        }
        else if (!result.empty() && !result.back().terminated) {
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
            else if (isCommandBoundary(c)) {
                result.push_back({ str(1, c), true });
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
            else if (isCommandBoundary(c)) {
                flush(true);
                result.push_back({ str(1, c), true });
                state = State::Whitespace;
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
        flush(force_terminate_at_end);

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


// Parse a single pipeline-stage (no `|` or `;` left in it) into a Command.
static std::optional<Command> parse_command(const str& stage, bool alias_complete) {
    // break into text-segments and redirection-operators
    auto pieces = split_redirects(stage);

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
            auto tokens = token_values(tokenize(piece.text, true, true));

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
    return cmd;
}

std::optional<std::vector<std::vector<Command>>> parse_line(const str& line, bool alias_complete) {
    std::vector<std::vector<Command>> pipelines;

    for (const str& seg : split(line, ';')) {
        str trimmed = trim(seg);
        if (trimmed.empty()) continue;

        std::vector<Command> pipeline;

        for (const str& stage : split(trimmed, '|')) {
            str stage_trimmed = trim(stage);
            if (stage_trimmed.empty()) {
                std::cerr << "Syntax error: empty command in pipeline\n";
                return std::nullopt;
            }

            auto cmd = parse_command(stage_trimmed, alias_complete);
            if (!cmd) return std::nullopt;

            if (!cmd->name.empty())
                pipeline.push_back(std::move(*cmd));
        }

        if (!pipeline.empty())
            pipelines.push_back(std::move(pipeline));
    }
    return pipelines;
}

str resolve_path(const str& p) {
    std::error_code ec;
    auto resolved = std::filesystem::weakly_canonical(p, ec);
    return ec ? p : resolved.string();   // fall back to raw path if resolution fails
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

bool CompletionItem::operator<(const CompletionItem& other) const { return full < other.full; }
bool CompletionItem::operator==(const CompletionItem& other) const { return full == other.full; }

std::vector<CompletionItem> completions_for(const str& prefix) {
    std::vector<CompletionItem> matches;
    std::error_code ec;

    for (const auto& dir : path_dirs()) {
        for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
            const str name = entry.path().filename().string();
            if (name.rfind(prefix, 0) == 0 && isExecutable(entry.path()))
                matches.push_back({ name,name,false });
        }
    }

    std::sort(matches.begin(), matches.end());
    matches.erase(std::unique(matches.begin(), matches.end()), matches.end());
    return matches;
}

std::vector<CompletionItem> file_completions(const str& prefix) {
    using namespace std::filesystem;

    std::vector<CompletionItem> matches;
    std::error_code ec;

    auto slash_pos = prefix.find_last_of('/');
    str dir_part = (slash_pos == str::npos) ? "" : prefix.substr(0, slash_pos + 1);
    str file_part = (slash_pos == str::npos) ? prefix : prefix.substr(slash_pos + 1);

    path search_dir = dir_part.empty() ? current_path() : path(dir_part);


    for (const auto& entry : directory_iterator(search_dir, ec)) {
        str name = entry.path().filename();

        if (name.starts_with('.') && !file_part.starts_with('.'))
            continue;

        if (name.rfind(file_part, 0) == 0) {
            str display_name = name;
            if (entry.is_directory())
                display_name += '/';

            str full_name = dir_part + display_name;

            matches.push_back({
                full_name,
                display_name,
                entry.is_directory()
                });
        }
    }

    std::sort(matches.begin(), matches.end());
    matches.erase(std::unique(matches.begin(), matches.end()), matches.end());
    return matches;
}

// Take in an array of sorted strings and give the longest_common_prefix
str longest_common_prefix(const std::vector<CompletionItem>& arr) {
    if (arr.empty())
        return "";
    if (arr.size() == 1)
        return arr.front().full;

    const str& a = arr.front().full;
    const str& b = arr.back().full;

    int i = 0;
    while (i < a.size() && i < b.size() && a[i] == b[i])    i++;

    return a.substr(0, i);
}