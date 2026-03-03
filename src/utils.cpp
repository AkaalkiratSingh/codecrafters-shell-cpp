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
    std::string t = trim(s);

    int i = 0;
    while (i < t.size() && t[i] != ' ')
        i++;

    std::string cmd = t.substr(0, i);
    std::string rest;
    if (i < t.size())
        rest = trim(t.substr(i));

    return {cmd, rest};
}