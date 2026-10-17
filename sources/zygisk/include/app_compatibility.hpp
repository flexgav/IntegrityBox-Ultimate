#pragma once

#include "runtime_config.hpp"

namespace app_compatibility {
enum class PairIPStatus { NotDetected, Detected, Unavailable };

// Only per-app spoofing is subject to this guard; Google roles remain independent.
PairIPStatus inspectPairIP(runtime::Role role, const char *mapsPath = "/proc/self/maps");
} // namespace app_compatibility
