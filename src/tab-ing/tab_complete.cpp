// ─────────────────────────────────────────────────────────────────────────────
// tab_complete.cpp
// ─────────────────────────────────────────────────────────────────────────────
// #include "tab_complete.hpp"
//
// #ifndef _WIN32
// #include <termios.h>
// #include <unistd.h>
// #endif
//
// #include <algorithm>
// #include <iostream>
//
// #ifndef _WIN32
//
// namespace {
//
// struct RawMode {
//     termios saved{};
//     RawMode()  {
//         tcgetattr(STDIN_FILENO, &saved);
//         termios raw = saved;
//         raw.c_lflag &= ~(ECHO | ICANON);  // no echo, read byte-by-byte
//         raw.c_cc[VMIN]  = 1;
//         raw.c_cc[VTIME] = 0;
//         tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
//     }
//     ~RawMode() { tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved); }
// };
//
// // Return the prefix of the last word in `line` (the part we complete).
// str last_word(const str &line) {
//     auto pos = line.find_last_of(" \t");
//     return (pos == str::npos) ? line : line.substr(pos + 1);
// }
//
// void redraw(const str &line, std::size_t cursor) {
//     // Move to column 0, print prompt + line, position cursor.
//     std::cout << "\r$ " << line;
//     // Erase to end of line (handles deletions)
//     std::cout << "\033[K";
//     // Move cursor back to right position
//     int back = static_cast<int>(line.size()) - static_cast<int>(cursor);
//     if (back > 0) std::cout << "\033[" << back << "D";
//     std::cout.flush();
// }
//
// } // namespace
//
// bool readline_with_completion(str &out) {
//     RawMode raw;
//     str line;
//     std::size_t cursor = 0;
//
//     while (true) {
//         char c;
//         if (read(STDIN_FILENO, &c, 1) <= 0) { out = line; return !line.empty(); }
//
//         if (c == '\n' || c == '\r') {
//             std::cout << '\n';
//             out = line;
//             return true;
//         }
//
//         if (c == 4 /* Ctrl-D */ && line.empty()) { std::cout << '\n'; return false; }
//
//         if (c == 127 || c == '\b') {          // Backspace
//             if (cursor > 0) {
//                 line.erase(--cursor, 1);
//                 redraw(line, cursor);
//             }
//             continue;
//         }
//
//         if (c == '\t') {                       // ← Tab: the interesting part
//             str prefix = last_word(line);
//             auto matches = completions_for(prefix);
//
//             // Also offer built-in command names
//             for (const auto &[name, _] : command_runner::cmd_map)
//                 if (name.rfind(prefix, 0) == 0)
//                     matches.push_back(name);
//             std::sort(matches.begin(), matches.end());
//             matches.erase(std::unique(matches.begin(), matches.end()), matches.end());
//
//             if (matches.empty()) {
//                 // Bell: nothing to complete
//                 std::cout << '\a'; std::cout.flush();
//             } else if (matches.size() == 1) {
//                 // Unique match → complete it (append the rest + a space)
//                 str suffix = matches[0].substr(prefix.size()) + ' ';
//                 line.insert(cursor, suffix);
//                 cursor += suffix.size();
//                 redraw(line, cursor);
//             } else {
//                 // Multiple matches → show list, redraw prompt
//                 std::cout << '\n';
//                 for (const auto &m : matches) std::cout << m << "  ";
//                 std::cout << '\n';
//                 redraw(line, cursor);
//             }
//             continue;
//         }
//
//         // Ordinary printable character
//         line.insert(cursor++, 1, c);
//         redraw(line, cursor);
//     }
// }
//
// #else  // _WIN32 – fall back to plain getline (implement with ReadConsole later)
// bool readline_with_completion(str &out) {
//     std::cout << "$ ";
//     return static_cast<bool>(std::getline(std::cin, out));
// }
// #endif
