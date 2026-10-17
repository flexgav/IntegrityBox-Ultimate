#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace property_virtualizer {

enum class Status {
    Applied, NoMatches, InvalidLayout, UnsupportedValue, MappingFailed,
    VerificationFailed, RollbackFailed
};

struct Result {
    Status status;
    size_t matched = 0;
    size_t applied = 0;
};

// Install in the target child before specialization drops property-file access.
// Exact rules win; suffix rules must be ordered longest-first by the caller.
// A failed preparation changes nothing. Failed installation restores all areas;
// RollbackFailed is reported separately and must never trigger a second engine.
Result install(const std::unordered_map<std::string, std::string> &exact,
               const std::vector<std::pair<std::string, std::string>> &suffixes);
const char *statusName(Status status);

} // namespace property_virtualizer
