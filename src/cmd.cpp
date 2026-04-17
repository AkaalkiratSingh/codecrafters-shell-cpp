#include "utils.hpp"

#include <iostream>
#include <cstdlib>

#include <filesystem>

#ifndef _WIN32
#include <sys/wait.h>
#include <unistd.h>
#endif

#include <string>

#include <map>
#include <functional>
void execute_external(const str &exec_path, const str &cmd, const str &rest, const str &raw);

bool command_runner::isActive = true;
std::map<str, std::function<void(std::vector<str> &)>> command_runner::cmd_map;

bool command_runner::repl()
{
    std::cout << "$ ";

    str s;
    if (!std::getline(std::cin, s))
        return false;

    if (s.empty())
        return isActive;

    auto [cmd, rest] = get_cmd(s);

    if (cmd.empty())
        return isActive;

    if (cmd_map.contains(cmd))
    {
        auto func = cmd_map[cmd];
        func(rest);
    }
    else
    {
        str exec_path = get_executable_path(cmd);
        if (!exec_path.empty())
            execute_external(exec_path, cmd, stringify(rest), s);
        else
            std::cerr << cmd << ": command not found\n";
    }
    return isActive;
}

void command_runner::exit(std::vector<str> &args)
{
    if (!args.empty())
        std::cerr << stringify(args) << ": command not found\n";

    command_runner::isActive = false;
}
void command_runner::echo(std::vector<str> &args) { std::cout << stringify(args) << '\n'; }

void command_runner::type(std::vector<str> &args)
{
    for (const auto &cmd : args)
    {
        if (command_runner::cmd_map.contains(cmd))
            std::cout << cmd << " is a shell builtin\n";
        else
        {
            str exec_path = get_executable_path(cmd);
            if (!exec_path.empty())
                std::cout << cmd << " is " << exec_path << '\n';
            else
                std::cerr << cmd << ": not found\n";
        }
    }
}

void command_runner::pwd(std::vector<str> &args) { std::cout << std::filesystem::current_path().string() << '\n'; }

void command_runner::cd(std::vector<str> &args)
{
    if (args.size() > 1)
    {
        std::cerr << "cd: too many arguments\n";
        return;
    }

    str target = args[0];

    try
    {
        if (target == "~")
        {
            char *home_ = std::getenv("HOME");
            target = home_;
        }
        std::filesystem::current_path(target);
    }
    catch (const std::exception &e)
    {
        std::cerr << "cd: " << target << ": No such file or directory\n";
    }
}

void command_runner::setup()
{
    cmd_map["echo"] = echo;
    cmd_map["type"] = type;
    cmd_map["exit"] = exit;
    cmd_map["pwd"] = pwd;
    cmd_map["cd"] = cd;
}

// Update the signature to accept the resolved exec_path
void execute_external(const str &exec_path, const str &cmd, const str &rest, const str &raw)
{

    // Hack to bypass VS Code not identifying the Linux/WSL functions/libraries
#ifndef _WIN32
    str full_cmd = raw;

    auto tkns = tokenize(full_cmd);
    std::vector<str> string_args;
    for (const auto &t : tkns)
        string_args.push_back(t);
    std::vector<char *> args;

    for (auto &s : string_args)
        args.push_back(const_cast<char *>(s.c_str()));

    args.push_back(nullptr);

    pid_t pid = fork();

    if (pid == 0)
    {
        // Use execv (no 'p') and pass the exact path we found
        execv(exec_path.c_str(), args.data());

        std::cerr << cmd << ": command not found\n";
        std::exit(1);
    }
    else if (pid > 0)
    {
        int status;
        waitpid(pid, &status, 0);
    }
    else
        std::cerr << "Fork failed\n";
#endif
}
