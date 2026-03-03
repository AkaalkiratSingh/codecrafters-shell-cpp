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
    using namespace std;
    using namespace std::filesystem;

    if (is_directory(item))
        return false;

    auto prms = status(item).permissions();

    bool b1 = (prms & perms::owner_exec) != perms::none;
    bool b2 = (prms & perms::group_exec) != perms::none;
    bool b3 = (prms & perms::others_exec) != perms::none;

    return b1 || b2 || b3;
}