#include "utils.hpp"

std::string trim(const std::string &s)
{
    int front = 0;
    int back = s.size() - 1;

    while (front <= back && s[front] == ' ')
        front++;
    while (back >= front && s[back] == ' ')
        back--;

    return s.substr(front, back - front + 1);
}

std::vector<std::string> split(const std::string &s)
{
    std::vector<std::string> res;
    std::stringstream ss(s);
    std::string word;

    while (ss >> word)
        res.push_back(word);

    return res;
}

std::pair<std::string, std::string> get_cmd(const std::string &s)
{
    typedef std::string str;

    str t = trim(s);

    int i = 0;
    while (i < t.size() && t[i] != ' ')
        i++;

    str cmd = t.substr(0, i);
    str rest;
    if (i < t.size())
        rest = trim(t.substr(i));

    return {cmd, rest};
}

#ifdef _WIN32
const char PATH_DELIMITER = ';';
#else
const char PATH_DELIMITER = ':';
#endif

bool isExecutable(std::filesystem::path &item)
{
    std::error_code ec;
    if (!std::filesystem::is_regular_file(item, ec))
        return false;

#ifdef _WIN32
    // Windows: If it's a regular file in PATH, we assume it's executable.
    return true;
#else
    // Linux/macOS: Check POSIX executable permissions
    auto prms = std::filesystem::status(item, ec).permissions();
    auto exec_mask = std::filesystem::perms::owner_exec |
                     std::filesystem::perms::group_exec |
                     std::filesystem::perms::others_exec;
    return (prms & exec_mask) != std::filesystem::perms::none;
#endif
}

std::string get_executable_path(const std::string &cmd)
{
    const char *path_env = std::getenv("PATH");
    if (!path_env)
        return "";

    std::stringstream ss(path_env);
    std::string dir;

    while (std::getline(ss, dir, PATH_DELIMITER))
    {
        std::filesystem::path full_path = std::filesystem::path(dir) / cmd;

#ifdef _WIN32
        // Windows: Check exact name, then try appending ".exe"
        if (isExecutable(full_path))
            return full_path.string();

        std::filesystem::path exe_path = full_path.string() + ".exe";
        if (isExecutable(exe_path))
            return exe_path.string();
#else
        // Linux: Just check the exact name with permissions
        if (isExecutable(full_path))
            return full_path.string();
#endif
    }
    return "";
}