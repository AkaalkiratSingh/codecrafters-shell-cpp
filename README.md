# Custom C++ Shell

A POSIX-style shell built from scratch in C++23 — no `libreadline`, no external line-editing library. Terminal input, history navigation, and Tab completion are all hand-rolled on raw `termios`.

This project started as a solution to the CodeCrafters ["Build Your Own Shell"](https://app.codecrafters.io/courses/shell/overview) challenge, whose curriculum covers pipelines, I/O redirection, persistent history, and tab completion — implemented here on top of the starter REPL skeleton the challenge provides.

## Features

**Interactive REPL with a custom line editor**

- Raw-`termios`-based input loop (`CustomReadline`)
- Up/Down arrow history recall, Ctrl+L to clear the screen
- Tab completion that's aware of _where_ the cursor is:
  - Command position → matches shell builtins and `$PATH` executables
  - Argument position → matches files/directories relative to the current word
  - Single match completes inline; multiple matches expand to their longest common prefix, then list on a second Tab

**Built-in commands**

- `echo`, `type`, `pwd`, `cd` (with `~` expansion), `exit`
- `history` — full `-r` (read), `-w` (write), `-a` (append) support, plus `history [n]` to show the last _n_ entries
- Persistent history via the `HISTFILE` environment variable: loaded automatically on startup, flushed automatically on exit

**Pipelines**

- Arbitrary-length pipelines (`cmd1 | cmd2 | cmd3 | ...`)
- Semicolon-separated command lists on one line (`cmd1 ; cmd2`)
- Single-stage commands run builtins in-process (so `cd`, `history`, etc. correctly mutate shell state instead of a throwaway child)

**I/O Redirection**

- `>` / `>>` for stdout
- `1>` / `1>>` explicitly
- `2>` / `2>>` for stderr

**Parsing**

- Single-quote, double-quote, and backslash-escape handling
- Redirection operators and pipe/semicolon boundaries are recognized correctly inside a token stream, including mid-word during tab completion

**Aliases**

- A command alias system, wired through the tokenizer via `alias_map`. Configured in `setupAliasMap()` (in `src/cmd.cpp`):

```cpp
   void setupAliasMap() {
       alias_map["ls"] = { "ls","--color=auto" };
       alias_map["grep"] = { "grep","--color=auto" };
       alias_map["egrep"] = { "egrep","--color=auto" };
       alias_map["fgrep"] = { "fgrep","--color=auto" };
   }
```

## Not yet implemented

- `$VAR` expansion and filename globbing
- Job control (background `&`, `fg`, `bg`)

## Tech Stack

- **Language:** C++23
- **Build System:** CMake
- **Dependencies:** none beyond a standard C++ toolchain

## Installation and Setup

### Prerequisites

A C++ compiler (GCC or Clang) and CMake, on Linux (or any POSIX system with `termios`, `fork`/`execv`, and `fcntl`).

```bash
# Verify CMake installation
cmake --version
```

### Running the Shell

```bash
# Clone the repository
git clone https://github.com/AkaalkiratSingh/CHELL_PLUS_PLUS.git
cd codecrafters-shell-cpp
```

- Quickest path — the provided wrapper script handles the CMake build for you:

  ```bash
  ./run.sh
  ```

- Or build manually:

  ```bash
  mkdir build && cd build
  cmake ..
  cmake --build .
  ./shell
  ```

### Example sessions

- **Builtins and PATH resolution**

  ```
  $ echo "Hello World!"
  Hello World!
  $ type cd
  cd is a shell builtin
  $ type cat
  cat is /usr/bin/cat
  $ pwd
  /home/user/codecrafters-shell-cpp
  $ cd ..
  $ pwd
  /home/user
  ```

- **Pipelines and redirection**

  ```
  $ cat file.txt | grep error | wc -l
  3
  $ echo "logged" >> run.log
  $ ls nonexistent_dir 2> errors.log
  $ cat errors.log
  ls: cannot access 'nonexistent_dir': No such file or directory
  ```

- **Quoting**

  ```
  $ echo "spaces   are preserved"
  spaces   are preserved
  $ echo 'single $HOME stays literal'
  single $HOME stays literal
  ```

- **History**

  ```
  $ history
  1 echo "Hello World!"
  2 type cd
  3 pwd
  4 cd ..
  5 pwd
  6 history
  $
  $ history 5
  3 pwd
  4 cd ..
  5 pwd
  6 history
  7 history 5
  ```

## Project Structure

```
.
├── src/
│   ├── main.cpp                        # Entry point: REPL loop + startup/shutdown history flush
│   ├── cmd.cpp                         # Builtins, pipelines, redirection, history persistence
│   ├── utils.cpp / utils.hpp           # Tokenizer, parser, PATH resolution, completion helpers
│   └── CustomReadline/
│       ├── CustomReadline.hpp
│       └── CustomReadline.cpp          # Raw-termios line editor: history nav, Tab completion
├── CMakeLists.txt                      # Build configuration
└── run.sh                              # Wrapper script to compile and run in one step
```
