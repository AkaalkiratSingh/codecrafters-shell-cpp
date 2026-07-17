#include "CustomReadline.hpp"
#include "../utils.hpp"

#include <termios.h>
#include <unistd.h>
#include <algorithm>
#include <iostream>

namespace {
    struct RawMode {
        termios saved{};
        RawMode() {
            tcgetattr(STDIN_FILENO, &saved);
            termios raw = saved;
            raw.c_lflag &= ~(ECHO | ICANON);  // no echo, read byte-by-byte
            raw.c_cc[VMIN] = 1;
            raw.c_cc[VTIME] = 0;
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
        }
        ~RawMode() { tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved); }
    };

    // Return the prefix of the last word in `line` (the part we complete).
    str last_word(const str& line) {
        auto pos = line.find_last_of(" \t");
        return (pos == str::npos) ? line : line.substr(pos + 1);
    }

    void redraw(const str& line, int cursor) {
        // Move to column 0, print prompt + line, position cursor.
        std::cout << "\r$ " << line;
        // Erase to end of line (handles deletions)
        std::cout << "\033[K";
        // Move cursor back to right position
        int back = line.size() - cursor;
        if (back > 0) std::cout << "\033[" << back << "D";
        std::cout.flush();
    }

}

static bool is_command_boundary(const str& token_value) { return token_value == "|" || token_value == ";"; }

WordContext get_word_context(const str& line, std::size_t cursor) {
    str upto = line.substr(0, cursor);
    auto tokens = tokenize(upto, false);

    if (tokens.empty())
        return {
            "",                     // word
            true                    // is_command_position
    };

    if (tokens.back().terminated) {
        bool is_command_pos = is_command_boundary(tokens.back().value);
        return {
            "",                     // word
            is_command_pos          // is_command_position
        };
    }

    // In between a word
    str word = tokens.back().value;
    bool is_first = (tokens.size() == 1) || is_command_boundary(tokens[tokens.size() - 2].value);
    return { word, is_first };
}

bool readline_with_completion(str& out) {
    RawMode raw;
    str line;
    int cursor = 0;
    bool tab_remains = false;

    int c_pointer = command_runner::historyLogs.size(); // pointer to the currently focussed command;
    str storedLine;

    while (true) {
        char c;
        if (read(STDIN_FILENO, &c, 1) <= 0) {
            out = line;
            return !line.empty();
        }

        if (c == '\033') {                 // ESC
            char seq[2];
            if (read(STDIN_FILENO, &seq[0], 1) <= 0) continue;
            if (read(STDIN_FILENO, &seq[1], 1) <= 0) continue;
            if (seq[0] == '[') {
                switch (seq[1]) {
                case 'A':                   // Up Arrow
                    if (c_pointer <= 0) break;

                    if (c_pointer == command_runner::historyLogs.size())    // was at the new command
                        storedLine = line;

                    c_pointer--;
                    line = command_runner::historyLogs[c_pointer];
                    cursor = line.size();
                    redraw(line, cursor);

                    break;
                case 'B':                   // Down Arrow
                    if (c_pointer == command_runner::historyLogs.size()) break;

                    c_pointer++;
                    if (c_pointer == command_runner::historyLogs.size())
                        line = storedLine;
                    else
                        line = command_runner::historyLogs[c_pointer];
                    cursor = line.size();
                    redraw(line, cursor);

                    break;
                case 'C':                   // Right Arrow
                    if (cursor < line.size()) {
                        cursor++;
                        redraw(line, cursor);
                    }
                    break;
                case 'D':                   // Left Arrow
                    if (cursor > 0) {
                        cursor--;
                        redraw(line, cursor);
                    }
                    break;
                }
            }
            continue;
        }

        // if none of the arrow are pressed
        c_pointer = command_runner::historyLogs.size();
        storedLine = "";

        if (c != '\t') tab_remains = false;

        if (c == '\n' || c == '\r') {
            std::cout << '\n';
            out = line;
            return true;
        }

        if (c == 12) {                        // Ctrl + L
            std::cout << "\033[H\033[2J";
            redraw(line, cursor);
            continue;
        }

        if (c == 4) {                         // Ctrl + D
            if (line.empty()) {
                std::cout << '\n';
                return false;
            }
            continue;
        }

        if (c == 127 || c == '\b') {          // Backspace
            if (cursor > 0) {
                line.erase(--cursor, 1);
                redraw(line, cursor);
            }
            continue;
        }

        if (c == '\t') {                       // Tab
            WordContext wc = get_word_context(line, cursor);

            std::vector<CompletionItem> matches;
            if (wc.is_command_position) {
                // Executables from path
                if (wc.word.empty())
                    continue;


                matches = completions_for(wc.word);

                // builtins
                for (const auto& [name, _] : command_runner::cmd_map)
                    if (name.rfind(wc.word, 0) == 0)
                        matches.push_back({ name,name,false });
            }
            else {
                ///// TODO: filesystem-based completion, wired in next
                matches = file_completions(wc.word);
            }

            // clear duplicates and order the possible matches
            std::sort(matches.begin(), matches.end());
            matches.erase(std::unique(matches.begin(), matches.end()), matches.end());

            if (matches.empty()) {
                std::cout << '\a';
                std::cout.flush();

                tab_remains = false;
            }
            else {
                str lcp = longest_common_prefix(matches);

                if (lcp.size() > wc.word.size()) {
                    str suffix = lcp.substr(wc.word.size());
                    if (matches.size() == 1 && !matches[0].is_dir) suffix += ' ';
                    line.insert(cursor, suffix);
                    cursor += suffix.size();
                    redraw(line, cursor);
                    tab_remains = false;
                }
                else if (!tab_remains) {
                    std::cout << '\a'; std::cout.flush();
                    tab_remains = true;
                }
                else {
                    std::cout << '\n';
                    for (std::size_t i = 0; i < matches.size(); ++i) {
                        std::cout << matches[i].display;
                        if (i + 1 < matches.size()) std::cout << "  ";
                    }
                    std::cout << '\n';
                    redraw(line, cursor);
                    tab_remains = false;
                }
            }
            continue;
        }

        // Ordinary printable character
        line.insert(cursor++, 1, c);
        redraw(line, cursor);
    }
}
