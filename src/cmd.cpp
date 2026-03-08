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
    std::string fullCmd = cmd;
    if (!rest.empty())
        fullCmd += " " + rest;

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    std::vector<char> cmd_buffer(fullCmd.begin(), fullCmd.end());
    cmd_buffer.push_back('\0');

    if (CreateProcessA(
            NULL,              // Application name (NULL means use command line)
            cmd_buffer.data(), // Command line string
            NULL,              // Process handle not inheritable
            NULL,              // Thread handle not inheritable
            FALSE,             // Set handle inheritance to FALSE
            0,                 // No creation flags
            NULL,              // Use parent's environment block
            NULL,              // Use parent's starting directory
            &si,               // Pointer to STARTUPINFO structure
            &pi                // Pointer to PROCESS_INFORMATION structure
            ))
    {
        // --- PARENT PROCESS ---
        // Wait for the child process to finish
        WaitForSingleObject(pi.hProcess, INFINITE);

        // Close handles to avoid memory leaks
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    else
        std::cout << cmd << ": command not found\n";
    
}