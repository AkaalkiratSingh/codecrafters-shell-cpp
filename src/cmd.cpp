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
    if (input == "")
        return;
    auto [cmd, rest] = get_cmd(input);

    if (command_runner::cmd_map.contains(cmd))
    {
        std::cout << cmd << " is a shell builtin\n";
        type(rest);
    }
    else
    {

        using namespace std::filesystem;

        const char *path_env = std::getenv("PATH");
        if (path_env != nullptr)
        {
            std::string path_str(path_env);
            std::stringstream ss(path_str);
            std::string dir;
            while (std::getline(ss, dir, ':'))
            {
                path full_path = path(dir) / cmd;
                if (exists(full_path) && is_regular_file(full_path))
                {
                    if (isExecutable(full_path))
                    {
                        std::cout << cmd << " is " << full_path.string() << '\n';
                        return;
                    }
                }
            }
        }
        std::cout << cmd << ": not found\n";
    }
}
void command_runner::pwd(std::string &input) { std::cout << std::filesystem::current_path() << '\n'; }
void command_runner::setup()
{
    cmd_map["echo"] = echo;
    cmd_map["type"] = type;
    cmd_map["exit"] = exit;
    cmd_map["pwd"] = pwd;
}