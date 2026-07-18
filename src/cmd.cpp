#include "utils.hpp"
#include "CustomReadline/CustomReadline.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <functional>

#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

static void execute_external(const str& exec_path, const Command& cmd);
static int  open_redirect_fd(const Redirect& r);

namespace command_runner {
    bool isActive = true;
    std::map<str, std::function<void(std::vector<str>&)>> cmd_map;
    std::map<str, std::vector<str>> alias_map;
    std::vector<str> historyLogs;

    // Containers to manage Redirection
    struct RedirectGuard {
        std::streambuf* saved_cout = std::cout.rdbuf();
        std::streambuf* saved_cerr = std::cerr.rdbuf();
        std::ofstream   out_file, err_file;

        RedirectGuard(const std::vector<Redirect>& redirects) {
            for (const auto& r : redirects) {
                auto mode = std::ios::out | (r.append ? std::ios::app : std::ios::trunc);
                if (r.fd == Redirect::Fd::Stdout) {
                    out_file.open(r.target, mode);
                    std::cout.rdbuf(out_file.rdbuf());
                }
                else {
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

    static void execute_pipeline(std::vector<Command>& cmds) {
        std::size_t n = cmds.size();
        std::vector<std::array<int, 2>> pipes(n - 1);

        for (auto& p : pipes) {
            if (pipe(p.data()) < 0) { std::perror("pipe"); return; }
        }

        std::vector<pid_t> pids;
        pids.reserve(n);

        for (std::size_t i = 0; i < n; ++i) {
            pid_t pid = fork();
            if (pid < 0) { std::cerr << "fork failed\n"; continue; }

            if (pid == 0) {
                if (i > 0)     dup2(pipes[i - 1][0], STDIN_FILENO);
                if (i < n - 1) dup2(pipes[i][1], STDOUT_FILENO);

                for (auto& p : pipes) { close(p[0]); close(p[1]); }

                const Command& cmd = cmds[i];

                for (const auto& r : cmd.redirects) {
                    int fd = open_redirect_fd(r);
                    int target = (r.fd == Redirect::Fd::Stdout) ? STDOUT_FILENO : STDERR_FILENO;
                    dup2(fd, target);
                    close(fd);
                }

                if (cmd_map.contains(cmd.name)) {
                    std::vector<str> args = cmd.args;
                    cmd_map[cmd.name](args);
                    std::exit(0);
                }

                auto path = find_in_path(cmd.name);
                if (!path) {
                    std::cerr << cmd.name << ": command not found\n";
                    std::exit(127);
                }

                std::vector<str> argv_strs = { cmd.name };
                argv_strs.insert(argv_strs.end(), cmd.args.begin(), cmd.args.end());
                std::vector<char*> argv;
                argv.reserve(argv_strs.size() + 1);
                for (auto& s : argv_strs) argv.push_back(s.data());
                argv.push_back(nullptr);

                execv(path->c_str(), argv.data());
                std::perror("execv");
                std::exit(1);
            }

            pids.push_back(pid);
        }

        for (auto& p : pipes) { close(p[0]); close(p[1]); }
        for (pid_t pid : pids) {
            int status;
            waitpid(pid, &status, 0);
        }
    }

    bool repl() {
        std::cout << "$ ";

        str line;
        // if (!std::getline(std::cin, line)) return false;
        if (!readline_with_completion(line))    return false;
        if (trim(line).empty())                 return isActive;

        historyLogs.push_back(line);

        auto pipelines = parse_line(line);

        // cmds -> nullopt => if there was some parse error, the error is already printed
        if (!pipelines) return isActive;

        for (auto& pipeline : *pipelines) {
            if (pipeline.size() == 1) {
                auto& cmd = pipeline.front();
                RedirectGuard guard(cmd.redirects);

                if (cmd_map.contains(cmd.name)) {
                    cmd_map[cmd.name](cmd.args);
                }
                else {
                    auto path = find_in_path(cmd.name);

                    if (path)   execute_external(*path, cmd);
                    else        std::cerr << cmd.name << ": command not found\n";
                }
            }
            else {
                execute_pipeline(pipeline);
            }
        }
        return isActive;
    }


    void builtin_exit(std::vector<str>& args) {
        if (!args.empty())
            std::cerr << stringify(args) << ": command not found\n";
        isActive = false;
    }

    void builtin_echo(std::vector<str>& args) {
        std::cout << stringify(args) << '\n';
    }

    void builtin_type(std::vector<str>& args) {
        for (const auto& name : args) {
            if (cmd_map.contains(name))
                std::cout << name << " is a shell builtin\n";
            else if (auto path = find_in_path(name))
                std::cout << name << " is " << *path << '\n';
            else
                std::cerr << name << ": not found\n";
        }
    }

    void builtin_pwd(std::vector<str>& args) { std::cout << std::filesystem::current_path().string() << '\n'; }

    void builtin_cd(std::vector<str>& args) {
        if (args.size() > 1) { std::cerr << "cd: too many arguments\n"; return; }

        str target = args.empty() ? "~" : args[0];

        if (target == "~") {
            const char* home = std::getenv("HOME");
            if (!home) { std::cerr << "cd: HOME not set\n"; return; }
            target = home;
        }

        std::error_code ec;
        std::filesystem::current_path(target, ec);
        if (ec)
            std::cerr << "cd: " << target << ": No such file or directory\n";
    }

    void builtin_history(std::vector<str>& args) {
        if (args.empty()) {
            for (int i = 0;i < historyLogs.size();i++)
                std::cout << i + 1 << ' ' << historyLogs[i] << '\n';
            return;
        }

        // Read Flag
        if (args[0] == "-r") {
            if (args.size() == 1) {    // -r ke baad file nhi di
                std::cerr << "history: target file not provided\n";
                return;
            }

            std::ifstream file(args[1]);
            if (!file) {
                std::cerr << "history: cannot open " << args[1] << '\n';
                return;
            }

            str line;
            while (std::getline(file, line))
                if (!line.empty())
                    historyLogs.push_back(line);
            
            return;
        }

        // Write Flag
        if (args[0] == "-w") {
            if (args.size() == 1) {
                std::cerr << "history: target file not provided\n";
                return;
            }

            std::ofstream file(args[1]);
            if (!file) {
                std::cerr << "history: cannot open " << args[1] << '\n';
                return;
            }

            for (const str& line : historyLogs)
                file << line << '\n';
            file << '\n';

            return;
        }

        // Numerical input
        int num;
        try {
            num = std::stoi(args[0]);
        }
        catch (const std::exception& e) {
            std::cerr << "history: Invalid Syntax \nCorrect Usage : `history [n]` where n is a positive number \n";
            return;
        }

        int start = std::max(0, (int)historyLogs.size() - num);
        for (int i = start;i < historyLogs.size();i++)
            std::cout << i + 1 << ' ' << historyLogs[i] << '\n';

    }


    void setupCmdMap() {
        cmd_map["echo"] = builtin_echo;
        cmd_map["type"] = builtin_type;
        cmd_map["exit"] = builtin_exit;
        cmd_map["pwd"] = builtin_pwd;
        cmd_map["cd"] = builtin_cd;
        cmd_map["history"] = builtin_history;
    }
    void setupAliasMap() {
        // alias_map["ls"] = { "ls","--color=auto" };
        // alias_map["grep"] = { "grep","--color=auto" };
        // alias_map["egrep"] = { "egrep","--color=auto" };
        // alias_map["fgrep"] = { "fgrep","--color=auto" };
    }

    void setup() {
        setupCmdMap();
        setupAliasMap();
    }
}


static int open_redirect_fd(const Redirect& r) {
    int flags = O_WRONLY | O_CREAT | (r.append ? O_APPEND : O_TRUNC);
    int fd = open(r.target.c_str(), flags, 0644);
    if (fd < 0) { std::perror("open"); std::exit(1); }
    return fd;
}

static void execute_external(const str& exec_path, const Command& cmd) {
    std::vector<str>    argv_strs = { cmd.name };
    argv_strs.insert(argv_strs.end(), cmd.args.begin(), cmd.args.end());

    std::vector<char*> argv;
    argv.reserve(argv_strs.size() + 1);
    for (auto& s : argv_strs) argv.push_back(s.data());
    argv.push_back(nullptr);

    pid_t pid = fork();
    if (pid < 0) { std::cerr << "fork failed\n"; return; }

    if (pid == 0) {
        // Child: apply redirections then exec
        for (const auto& r : cmd.redirects) {
            int fd = open_redirect_fd(r);

            int target;
            if (r.fd == Redirect::Fd::Stdout)   target = STDOUT_FILENO;
            else                                target = STDERR_FILENO;

            dup2(fd, target);
            close(fd);
        }
        execv(exec_path.c_str(), argv.data());
        std::perror("execv");
        std::exit(1);
    }

    int status;
    waitpid(pid, &status, 0);
}