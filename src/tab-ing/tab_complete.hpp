#pragma once
#include "../utils.hpp"
#include <optional>

struct WordContext {
    str  word;                 
    bool is_command_position;  
};

bool readline_with_completion(str& line);

