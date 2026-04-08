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

std::vector<Token> tokenize(const str &s)
{
    str t = ' ' + s + ' ';
    std::vector<Token> res;
    str cur;

    int i = 1;
    enum state
    {
        DEF,
        SIN_Q,
        DBL_Q,
        WHITE
    };

    state currentState = WHITE;

    auto insert = [&](TokenType tp = DEFAULT, bool isTerminated = false)
    {
        if (!res.empty() && !res.back().isTerminated)
            res.back().raw += cur;
        else
            res.emplace_back(cur, tp, isTerminated);
        cur.clear();
    };

    while (i < t.size())
    {
        switch (currentState)
        {
        case DEF:
            if (std::isspace(t[i]))
            {
                insert(DEFAULT, true);
                currentState = WHITE;
            }
            else if (t[i] == '\'')
            {
                insert();
                currentState = SIN_Q;
            }
            else if (t[i] == '\"')
            {
                insert();
                currentState = DBL_Q;
            }
            else if (t[i] == '\\')
                cur.push_back(t[++i]);
            else
                cur.push_back(t[i]);
            break;

        case SIN_Q:
            if (t[i] == '\'')
            {
                insert(SINGLE_QUOTES);
                currentState = WHITE;
            }
            else
                cur.push_back(t[i]);
            break;

        case DBL_Q:
            if (t[i] == '\"')
            {
                insert(DOUBLE_QUOTES);
                currentState = WHITE;
            }
            else if (t[i] == '\\')
            {
                if (t[i + 1] == '\\' || t[i + 1] == '\"')
                    cur.push_back(t[++i]);
            }
            else
                cur.push_back(t[i]);
            break;

        case WHITE:
            if (t[i] == '\'')
                currentState = SIN_Q;
            else if (t[i] == '\"')
                currentState = DBL_Q;
            else if (!std::isspace(t[i]))
            {
                if (t[i] == '\\')
                    i++;
                cur.push_back(t[i]);
                currentState = DEF;
            }
            else if (!res.empty())
                res.back().isTerminated = true;
            break;
        }
        i++;
    }

    if (currentState == SIN_Q || currentState == DBL_Q)
    {
        TokenType tp = (currentState == SIN_Q) ? SINGLE_QUOTES : DOUBLE_QUOTES;
        insert(tp, true);
        cur.clear();
    }
    return res;
}

str echofi(const str &s)
{
    auto tkns = tokenize(s);
    str res;

    int n = tkns.size();
    for (int i = 0; i < n; i++)
    {
        res += tkns[i].raw;
        if (tkns[i].isTerminated)
            res.push_back(' ');
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