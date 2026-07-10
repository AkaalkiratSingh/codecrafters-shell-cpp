# Custom C++ Shell

A lightweight POSIX-compliant shell built from scratch in C++, featuring built-in commands, PATH resolution, Tab completion, and external program execution.

This project explores the fundamentals of Unix system programming by implementing a shell from scratch, including command parsing, process creation, file system navigation, and interactive REPL execution.

## Features

* **Interactive REPL:** A robust loop that reads standard input, parses command tokens, evaluates them, and prints the output seamlessly.
* **Built-in Commands:** Native shell commands implemented directly in C++ without spawning new processes, including:
    * `echo`: Prints text to the standard output.
    * `type`: Identifies whether a command is a built-in or an external executable.
    * `pwd`: Outputs the current working directory.
    * `cd`: Navigates the file system and changes the current working directory.
    * `exit`: Safely terminates the shell environment.
* **External Program Execution:** Parses the system `$PATH` environment variable to locate and run external binaries natively.
* **Argument Parsing:** Accurately tokenizes user input to pass arguments correctly to both built-in and external programs.
* **Command Completion:** Supports Tab-based auto-completion by dynamically searching built-in commands and external executables located within the system $PATH.
* **Filename Completions:** Enables quick traversal of the file system by auto-completing directory and file names based on the current working directory and input context.

## Tech Stack

* **Language:** C++ (C++17/C++20/C++23 recommended)
* **Build System:** CMake

## Installation and Setup

### Prerequisites
Ensure you have a `C++ compiler` (like GCC or Clang) and `CMake` installed on your local `Linux` machine.

```bash
# Verify CMake installation
cmake --version
```

### Running the Shell

- To compile and run the shell locally, you can use the provided shell script `run.sh` which automatically handles the CMake build process:

   ```bash
   # Clone the repository
   git clone https://github.com/AkaalkiratSingh/codecrafters-shell-cpp.git
   cd codecrafters-shell-cpp

   # Execute the run script
   ./run.sh
   ```

- Alternatively, you can build the project manually:
   ```bash
   mkdir build
   cd build

   cmake ..
   cmake --build .

   ./shell
   ```

Once running, you will be dropped into the custom shell prompt where you can begin typing commands:

```
$ echo "Hello World!"
Hello World!
$ type pwd
pwd is a shell builtin
$ pwd
/home/user/codecrafters-shell-cpp
```

## Project Structure
```
.
├── src/
│   ├── main.cpp                 # The entry point managing the REPL loop and initialization
│   ├── cmd.cpp                  # Core shell logic, including built-ins and I/O redirection
│   ├── utils.cpp                # Collection of helper and utility functions
│   └── tab-ing/
│       └── tab_complete.cpp     # Custom input abstraction for tab-completion and dynamic suggestions
├── CMakeLists.txt               # Build configuration for compiling the C++ source
└── run.sh                       # Wrapper script to compile and execute the shell quickly
```