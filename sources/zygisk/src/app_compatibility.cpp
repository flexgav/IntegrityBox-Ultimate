#include "app_compatibility.hpp"

#include <cstdio>
#include <cstdlib>
#include <string_view>

namespace app_compatibility {
PairIPStatus inspectPairIP(runtime::Role role, const char *mapsPath) {
    if (role != runtime::Role::App) return PairIPStatus::NotDetected;
    FILE *maps = fopen(mapsPath, "r");
    if (!maps) return PairIPStatus::Unavailable;

    PairIPStatus result = PairIPStatus::NotDetected;
    char *line = nullptr;
    size_t capacity = 0;
    ssize_t length;
    while ((length = getline(&line, &capacity, maps)) >= 0) {
        std::string_view mapping(line, static_cast<size_t>(length));
        const auto end = mapping.find_last_not_of("\r\n");
        if (end == mapping.npos) continue;
        mapping = mapping.substr(0, end + 1);
        if (mapping.ends_with(" (deleted)")) mapping.remove_suffix(sizeof(" (deleted)") - 1);
        const auto slash = mapping.find_last_of('/');
        if (slash != mapping.npos && mapping.substr(slash + 1) == "libpairipcore.so") {
            result = PairIPStatus::Detected;
            break;
        }
    }
    if (result == PairIPStatus::NotDetected && !feof(maps)) result = PairIPStatus::Unavailable;
    free(line);
    if (fclose(maps) != 0 && result == PairIPStatus::NotDetected) result = PairIPStatus::Unavailable;
    return result;
}
} // namespace app_compatibility
