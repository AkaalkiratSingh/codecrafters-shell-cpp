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

std::vector<std::string> split(std::string &s)
{
    std::vector<std::string> res;
    std::stringstream ss(s);
    std::string word;

    while (ss >> word)
        res.push_back(word);

    return res;
}