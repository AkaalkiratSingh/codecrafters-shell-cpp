// tab_complete.hpp  –  drop-in readline loop with Tab completion
//
// Compile with:  g++ -std=c++20 main.cpp cmd.cpp utils.cpp tab_complete.cpp -o shell
// (no extra libraries needed – uses raw termios on Linux/macOS)
//
// HOW IT WORKS
// ─────────────
// We put the terminal into "raw" mode so we receive every keypress
// immediately (no line buffering, no echo).  When the user presses Tab
// we call completions_for() – already implemented in utils.cpp – and
// either complete the word or print the list of alternatives.
// Every other key is handled by our own mini line-editor that keeps a
// mutable `line` buffer and a cursor position.
//
// INTEGRATION
// ───────────
// Replace   std::getline(std::cin, line)   in command_runner::repl()
// with      readline_with_completion(line)
// That's the only change needed in cmd.cpp.

#pragma once
#include "utils.hpp"
#include <optional>

/// Read one line from stdin with Tab completion support.
/// Returns false on EOF (Ctrl-D on an empty line).
bool readline_with_completion(str &line);

