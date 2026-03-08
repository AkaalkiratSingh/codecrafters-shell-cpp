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

bool isExecutable(std::filesystem::path &item)
{
    std::error_code ec;
    // Just check if it's a regular file. On Windows, if it's an .exe and it exists,
    // it's generally considered executable.
    return std::filesystem::is_regular_file(item, ec);
}

std::string get_executable_path(const std::string &cmd)
{
    using namespace std;
    using namespace std::filesystem;

    const char *path_env = getenv("PATH");
    if (!path_env)
        return "";

    stringstream ss(path_env);
    string dir;

    while (getline(ss, dir, ';'))
    {
        // Check both the raw command (e.g., "ping.exe") and the appended extension ("ping" -> "ping.exe")
        std::vector<std::string> to_check = { cmd };
        
        // If the command doesn't already end in .exe, try appending it
        if (cmd.length() < 4 || cmd.substr(cmd.length() - 4) != ".exe") {
            to_check.push_back(cmd + ".exe");
        }

        for (const auto& file_name : to_check) {
            std::filesystem::path full_path = std::filesystem::path(dir) / file_name;

            if (isExecutable(full_path))
            {
                return full_path.string();
            }
        }
    }
    return "";
}