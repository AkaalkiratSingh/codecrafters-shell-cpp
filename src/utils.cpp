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

std::vector<str> split(const str &s, char c)
{
    std::vector<str> res;
    str cur;
    for (int i = 0; i <= s.size(); i++)
    {
        if (i == s.size() || s[i] == c)
        {
            res.push_back(cur);
            cur.clear();
        }
        else
            cur.push_back(s[i]);
    }

    return res;
}

std::vector<str> tokenize(const str &s)
{
    str t = ' ' + s + ' ';
    std::vector<str> res;
    std::vector<bool> terminationStatus;
    str cur;

    int i = 1;
    enum state
    {
        DEF,
        SIN_Q,
        DBL_Q,
        WHITE
    };

    state curState = WHITE;

    auto insert = [&](bool isTerminated = false)
    {
        if (!res.empty() && !terminationStatus.back())
        {
            res.back() += cur;
            terminationStatus.back() = isTerminated;
        }
        else
        {
            res.push_back(cur);
            terminationStatus.push_back(isTerminated);
        }

        cur.clear();
    };

    while (i < t.size())
    {
        switch (curState)
        {
        case DEF:
            if (std::isspace(t[i]))
            {
                insert(true);
                curState = WHITE;
            }
            else if (t[i] == '\'')
            {
                insert();
                curState = SIN_Q;
            }
            else if (t[i] == '\"')
            {
                insert();
                curState = DBL_Q;
            }
            else if (t[i] == '\\')
                cur.push_back(t[++i]);
            else
                cur.push_back(t[i]);

            break;

        case SIN_Q:
            if (t[i] == '\'')
            {
                insert();
                curState = WHITE;
            }
            else
                cur.push_back(t[i]);

            break;

        case DBL_Q:
            if (t[i] == '\"')
            {
                insert();
                curState = WHITE;
            }
            else if (t[i] == '\\' && (t[i + 1] == '\\' || t[i + 1] == '\"'))
                cur.push_back(t[++i]);
            else
                cur.push_back(t[i]);

            break;

        case WHITE:
            if (t[i] == '\'')
                curState = SIN_Q;
            else if (t[i] == '\"')
                curState = DBL_Q;
            else if (!std::isspace(t[i]))
            {
                if (t[i] == '\\')
                    i++;
                cur.push_back(t[i]);
                curState = DEF;
            }
            else if (!res.empty())
                terminationStatus.back() = true;

            break;
        }
        i++;
    }
    if (curState == SIN_Q || curState == DBL_Q)
    {
        insert();
        cur.clear();
    }
    return res;
}

str stringify(const std::vector<str> &tkns, int x)
{
    if (tkns.empty())
        return "";
    str res = "";

    for (int i = x; i < tkns.size(); i++)
        res += tkns[i] + ' ';

    res.pop_back();
    return res;
}

str echofi(const str &s) { return stringify(tokenize(s)); }

std::pair<str, std::vector<str>> get_cmd(const str &s)
{
    auto v = tokenize(s);

    if (v.empty())
        return {"", {}};

    str cmd = v[0];
    v.erase(v.begin());
    return {cmd, v};
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