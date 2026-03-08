#include "utils.hpp"
#include <cstdlib>

bool command_runner::isActive = true;
std::map<std::string, std::function<void(std::string &)>> command_runner::cmd_map;

bool command_runner::repl()
{
    std::cout << "$ ";

    std::string s;
    std::getline(std::cin, s);

    auto [cmd, rest] = get_cmd(s);
    if (cmd_map.contains(cmd))
    {
        auto func = cmd_map[cmd];
        func(rest);
    }
    else
        std::cout << cmd << ": command not found\n";

    return isActive;
}

void command_runner::exit(std::string &input)
{
    if (input != "")
        std::cout << input << ": command not found\n";

    command_runner::isActive = false;
}
void command_runner::echo(std::string &input) { std::cout << input << '\n'; }

void command_runner::type(std::string &input)
{
    if (input.empty())
        return;

    auto [cmd, rest] = get_cmd(input);

    if (command_runner::cmd_map.contains(cmd))
        std::cout << cmd << " is a shell builtin\n";
    else
    {
        std::string exec_path = get_executable_path(cmd);
        if (!exec_path.empty())
            std::cout << cmd << " is " << exec_path << '\n';
        else
            std::cout << cmd << ": not found\n";
    }

    if (!rest.empty())
        type(rest);
}

void command_runner::pwd(std::string &input) { std::cout << std::filesystem::current_path() << '\n'; }

void command_runner::setup()
{
    cmd_map["echo"] = echo;
    cmd_map["type"] = type;
    cmd_map["exit"] = exit;
    cmd_map["pwd"] = pwd;
}


void execute_external(const std::string &cmd, const std::string &rest)
{
#ifdef _WIN32
    // ==========================================
    // WINDOWS IMPLEMENTATION
    // ==========================================
    std::string command_line = cmd;
    if (!rest.empty())
        command_line += " " + rest;

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    std::vector<char> cmd_buffer(command_line.begin(), command_line.end());
    cmd_buffer.push_back('\0');

    if (CreateProcessA(NULL, cmd_buffer.data(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
    {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    else
    {
        std::cout << cmd << ": command not found\n";
    }

#else
    // ==========================================
    // LINUX / MACOS (POSIX) IMPLEMENTATION
    // ==========================================
    std::string full_cmd = cmd;
    if (!rest.empty())
        full_cmd += " " + rest;

    // Using your split utility
    std::vector<std::string> string_args = split(full_cmd);
    std::vector<char *> args;

    for (auto &s : string_args)
    {
        args.push_back(const_cast<char *>(s.c_str()));
    }
    args.push_back(nullptr);

    pid_t pid = fork();

    if (pid == 0)
    {
        // Child process
        execvp(args[0], args.data());
        std::cout << cmd << ": command not found\n";
        std::exit(1);
    }
    else if (pid > 0)
    {
        // Parent process
        int status;
        waitpid(pid, &status, 0);
    }
    else
    {
        std::cerr << "Fork failed\n";
    }
#endif
}