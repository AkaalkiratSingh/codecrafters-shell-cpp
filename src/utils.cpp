#include "utils.hpp"

#include <cctype>
#include <string>
#include <filesystem>

str trim(const str &s)
{
    int front = 0;
    int back = s.size() - 1;

    while (front <= back && s[front] == ' ')
        front++;
    while (back >= front && s[back] == ' ')
        back--;

    return s.substr(front, back - front + 1);
}

std::vector<str> split(const str &s)
{
    str t = ' ' + s + ' ';
    std::vector<str> res;
    str cur;

    int i = 1;
    enum state
    {
        READING,
        IN_QUOTES,
        WHITE_SPACE
    };

    state currentState = WHITE_SPACE;

    while (i < t.size())
    {
        switch (currentState)
        {
        case READING:
            if (std::isspace(t[i]))
            {
                res.push_back(cur);
                cur.clear();

                currentState = WHITE_SPACE;
            }
            else if (t[i] == '\'')
            {
                res.push_back(cur);
                cur.clear();

                currentState = IN_QUOTES;
            }
            else
                cur.push_back(t[i]);
            break;

        case IN_QUOTES:
            if (t[i] == '\'')
            {
                res.push_back(cur);
                cur.clear();

                currentState = WHITE_SPACE;
            }
            else
                cur.push_back(t[i]);
            break;

        case WHITE_SPACE:
            if (t[i] == '\'')
                currentState = IN_QUOTES;
            else if (!std::isspace(t[i]))
            {
                cur.push_back(t[i]);
                currentState = READING;
            }
            break;
        }

        i++;
    }
    if (currentState == IN_QUOTES)
    {
        cur.pop_back();
        res.push_back(cur);
    }
    return res;
}

str echofi(const str &s)
{
    str t = trim(s);
    str res;
    bool inQuote = false;
    for (char c : t)
    {
        if (c == '\'')
        {
            inQuote = !inQuote;
            continue;
        }
        if (c == ' ' && !inQuote && res.back() == ' ')
            continue;

        res.push_back(c);
    }
    return res;
}

std::pair<str, str> get_cmd(const str &s)
{
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
    return true;
#else
    auto prms = std::filesystem::status(item, ec).permissions();
    auto exec_mask = std::filesystem::perms::owner_exec |
                     std::filesystem::perms::group_exec |
                     std::filesystem::perms::others_exec;
    return (prms & exec_mask) != std::filesystem::perms::none;
#endif
}

str get_executable_path(const str &cmd)
{
    const char *path_env = std::getenv("PATH");
    if (!path_env)
        return "";

    std::stringstream ss(path_env);
    str dir;

    while (std::getline(ss, dir, PATH_DELIMITER))
    {
        std::filesystem::path full_path = std::filesystem::path(dir) / cmd;

#ifdef _WIN32
        if (isExecutable(full_path))
            return full_path.string();

        std::filesystem::path exe_path = full_path.string() + ".exe";
        if (isExecutable(exe_path))
            return exe_path.string();
#else
        if (isExecutable(full_path))
            return full_path.string();
#endif
    }
    return "";
}