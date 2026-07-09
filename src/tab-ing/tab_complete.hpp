#pragma once
#include "../utils.hpp"
#include <optional>

// Read one line from stdin with Tab completion support.
// Returns false on EOF (Ctrl-D on an empty line).
bool readline_with_completion(str &line);

