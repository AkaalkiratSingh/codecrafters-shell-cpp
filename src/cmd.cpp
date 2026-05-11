#include "utils.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <functional>

#ifndef _WIN32
#   include <fcntl.h>
#   include <sys/wait.h>
#   include <unistd.h>
#endif

// ── Forward declarations ──────────────────────────────────────────────────────

static void execute_external(const str &exec_path, const Command &cmd);

// ── Statics ───────────────────────────────────────────────────────────────────

bool command_runner::isActive = true;
std::map<str, std::function<void(std::vector<str> &)>> command_runner::cmd_map;

// ── I/O redirection RAII guard ────────────────────────────────────────────────

/// Temporarily redirects cout/cerr to files, restoring them on destruction.
struct RedirectGuard {
    std::streambuf *saved_cout = std::cout.rdbuf();
    std::streambuf *saved_cerr = std::cerr.rdbuf();
    std::ofstream   out_file, err_file;

    RedirectGuard(const std::vector<Redirect> &redirects) {
        for (const auto &r : redirects) {
            auto mode = std::ios::out | (r.append ? std::ios::app : std::ios::trunc);
            if (r.fd == Redirect::Fd::Stdout) {
                out_file.open(r.target, mode);
                std::cout.rdbuf(out_file.rdbuf());
            } else {
                err_file.open(r.target, mode);
                std::cerr.rdbuf(err_file.rdbuf());
            }
        }
    }

    ~RedirectGuard() {
        std::cout.rdbuf(saved_cout);
        std::cerr.rdbuf(saved_cerr);
    }
};

// ── REPL ──────────────────────────────────────────────────────────────────────
bool command_runner::repl() {
    std::cout << "$ ";

    str line;
    if (!std::getline(std::cin, line)) return false;
    if (trim(line).empty())            return isActive;

    auto cmds = parse_line(line);
    // cmds -> nullopt => if there is some parse error, the error is already printed simply exit
    if (!cmds) return isActive;

    for (auto &cmd : *cmds) {
        RedirectGuard guard(cmd.redirects);

        if (auto it = cmd_map.find(cmd.name); it != cmd_map.end()) {
            it->second(cmd.args);
        } else if (auto path = find_in_path(cmd.name)) {
            execute_external(*path, cmd);
        } else {
            std::cerr << cmd.name << ": command not found\n";
        }
    }

    return isActive;
}

// ── Built-in commands ─────────────────────────────────────────────────────────

void command_runner::builtin_exit(std::vector<str> &args) {
    if (!args.empty())
        std::cerr << stringify(args) << ": command not found\n";
    isActive = false;
}

void command_runner::builtin_echo(std::vector<str> &args) {
    std::cout << stringify(args) << '\n';
}

void command_runner::builtin_type(std::vector<str> &args) {
    for (const auto &name : args) {
        if (cmd_map.contains(name))
            std::cout << name << " is a shell builtin\n";
        else if (auto path = find_in_path(name))
            std::cout << name << " is " << *path << '\n';
        else
            std::cerr << name << ": not found\n";
    }
}

void command_runner::builtin_pwd(std::vector<str> &) {
    std::cout << std::filesystem::current_path().string() << '\n';
}

void command_runner::builtin_cd(std::vector<str> &args) {
    if (args.size() > 1) { std::cerr << "cd: too many arguments\n"; return; }

    str target = args.empty() ? "~" : args[0];

    if (target == "~") {
        const char *home = std::getenv("HOME");
        if (!home) { std::cerr << "cd: HOME not set\n"; return; }
        target = home;
    }

    std::error_code ec;
    std::filesystem::current_path(target, ec);
    if (ec)
        std::cerr << "cd: " << target << ": No such file or directory\n";
}

void command_runner::setup() {
    cmd_map["echo"] = builtin_echo;
    cmd_map["type"] = builtin_type;
    cmd_map["exit"] = builtin_exit;
    cmd_map["pwd"] = builtin_pwd;
    cmd_map["cd"] = builtin_cd;
}

// ── External command execution ────────────────────────────────────────────────

#ifndef _WIN32

static int open_redirect_fd(const Redirect &r) {
    int flags = O_WRONLY | O_CREAT | (r.append ? O_APPEND : O_TRUNC);
    int fd = open(r.target.c_str(), flags, 0644);
    if (fd < 0) { std::perror("open"); std::exit(1); }
    return fd;
}

static void execute_external(const str &exec_path, const Command &cmd) {
    // Build argv: [name, args..., nullptr]
    std::vector<str>    argv_strs = { cmd.name };
    argv_strs.insert(argv_strs.end(), cmd.args.begin(), cmd.args.end());

    std::vector<char *> argv;
    argv.reserve(argv_strs.size() + 1);
    for (auto &s : argv_strs) argv.push_back(s.data());
    argv.push_back(nullptr);

    pid_t pid = fork();
    if (pid < 0) { std::cerr << "fork failed\n"; return; }

    if (pid == 0) {
        // Child: apply redirections then exec
        for (const auto &r : cmd.redirects) {
            int fd = open_redirect_fd(r);
            int target = (r.fd == Redirect::Fd::Stdout) ? STDOUT_FILENO : STDERR_FILENO;
            dup2(fd, target);
            close(fd);
        }
        execv(exec_path.c_str(), argv.data());
        std::perror("execv");
        std::exit(1);
    }

    // Parent: wait for child
    int status;
    waitpid(pid, &status, 0);
}

#else
static void execute_external(const str &, const Command &) {
    std::cerr << "External commands not supported on Windows\n";
}
#endif
