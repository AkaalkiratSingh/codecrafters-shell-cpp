#include "utils.hpp"

#include <iostream>
#include <cstdlib>

#include <filesystem>
#include <fstream>
#include <streambuf>

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

    str l;
    if (!std::getline(std::cin, l))
        return false;

    if (l.empty())
        return isActive;

    for (auto &s : split(l, ';'))
    {

        std::streambuf *def_cout_buff = std::cout.rdbuf();
        std::streambuf *def_cerr_buff = std::cerr.rdbuf();

        s = ' ' + s + ' ';
        int i = 1;

        // Partition the given line into 3 strings
        // s1 -> the command
        // s2 -> the stdout redirection target (if any)
        // s3 -> the stderr redirection target (if any)
        str s1, s2, s3;
        bool append_out = false, append_err = false;
        enum state
        {
            DEF,
            _OUT,
            _ERR
        };
        state curState = DEF;
        while (i < s.size())
        {
            switch (curState)
            {
            case DEF:
                if (s[i] == '>')
                {
                    if (s[i - 1] == '2')
                    {
                        s1.pop_back();

                        // the next portion is the stderr redirection target
                        curState = _ERR;
                        if (s[i + 1] == '>')
                        {
                            append_err = true;
                            i++;
                        }
                    }
                    else
                    {
                        if (s[i - 1] == '1')
                            s1.pop_back();

                        // the next portion is the stdout redirection target
                        curState = _OUT;
                        if (s[i + 1] == '>')
                        {
                            append_out = true;
                            i++;
                        }
                    }
                }
                else
                    s1.push_back(s[i]);
                break;

            case _OUT:
                if (s[i] == '>')
                {
                    if (!s3.empty())
                    {
                        std::cerr << "Syntax error: multiple redirections\n";
                        return isActive;
                    }

                    if (s[i - 1] == '2')
                        s2.pop_back();

                    // the next portion is the stderr redirection target
                    curState = _ERR;
                    if (s[i + 1] == '>')
                    {
                        append_err = true;
                        i++;
                    }
                }
                else
                    s2.push_back(s[i]);
                break;

            case _ERR:
                if (s[i] == '>')
                {
                    if (!s2.empty())
                    {
                        std::cerr << "Syntax error: multiple redirections\n";
                        return isActive;
                    }

                    if (s[i - 1] == '1')
                        s3.pop_back();

                    // the next portion is the stdout redirection target
                    curState = _OUT;
                    if (s[i + 1] == '>')
                    {
                        append_out = true;
                        i++;
                    }
                }
                else
                    s3.push_back(s[i]);
                break;
            }
            i++;
        }

        // std::cout << "s1: " << s1 << '\n';
        // std::cout << "s2: " << s2 << '\n';
        // std::cout << "s3: " << s3 << '\n';
        s2 = trim(s2);
        s3 = trim(s3);

        std::ofstream out;
        std::ofstream err;

        if (!s2.empty())
        {
            out.open(s2, append_out ? std::ios::app : std::ios::trunc);
            std::cout.rdbuf(out.rdbuf());
        }
        if (!s3.empty())
        {
            err.open(s3, append_err ? std::ios::app : std::ios::trunc);
            std::cerr.rdbuf(err.rdbuf());
        }

        auto [cmd, rest] = get_cmd(s1);

        if (cmd.empty())
        {
            std::cout.rdbuf(def_cout_buff);
            std::cerr.rdbuf(def_cerr_buff);

            return isActive;
        }

        if (cmd_map.contains(cmd))
        {
            auto func = cmd_map[cmd];
            func(rest);
        }
        else
        {
            str exec_path = get_executable_path(cmd);
            if (!exec_path.empty())
                execute_external(exec_path, cmd, stringify(rest), s1);
            else
                std::cerr << cmd << ": command not found\n";
        }

        std::cout.rdbuf(def_cout_buff);
        std::cerr.rdbuf(def_cerr_buff);
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
