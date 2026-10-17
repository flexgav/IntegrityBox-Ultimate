// Adapted from KOWX712/PlayIntegrityFix inject_s v4.9 (GPL-3.0).
// Keep virtual mappings confined to the specialized target child.
#include "property_virtualizer.hpp"

#include "logging.hpp"
#include <climits>
#include <fcntl.h>
#include <linux/futex.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/sysmacros.h>
#include <sys/system_properties.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

namespace property_virtualizer {
namespace {

constexpr std::string_view kPropertyDirectory = "/dev/__properties__/";
constexpr uint32_t kPropertyAreaMagic = 0x504f5250;
constexpr uint32_t kPropertyAreaVersion = 0xfc6ed0ab;
constexpr uint32_t kLongPropertyFlag = 1U << 16;

struct PropertyAreaHeader {
    uint32_t bytesUsed;
    uint32_t serial;
    uint32_t magic;
    uint32_t version;
    uint32_t reserved[28];
};

struct InlinePropertyInfo {
    uint32_t serial;
    char value[PROP_VALUE_MAX];
};

static_assert(sizeof(PropertyAreaHeader) == 128);
static_assert(sizeof(InlinePropertyInfo) == 4 + PROP_VALUE_MAX);

struct Mapping {
    uintptr_t start = 0;
    uintptr_t end = 0;
    int prot = 0;
    uint64_t offset = 0;
    std::string path;
    unsigned devMajor = 0;
    unsigned devMinor = 0;
    uint64_t inode = 0;
    char permLetter = 's';
    int fd = -1;
    bool replaced = false;
    uint32_t bytesUsed = 0;
    uint32_t nextBytesUsed = 0;
    std::vector<char> privateSnapshot;
};

struct Override {
    Mapping *mapping = nullptr;
    uintptr_t offset = 0;
    std::string name;
    std::string value;
    bool wasLong = false;
    uintptr_t longOffset = 0;
    size_t longLen = 0;
    uintptr_t newLongOffset = 0;
};

struct State {
    const std::unordered_map<std::string, std::string> &exact;
    const std::vector<std::pair<std::string, std::string>> &suffixes;
    std::vector<Mapping> mappings;
    std::vector<Override> overrides;
    Status failure = Status::Applied;
    size_t matched = 0;
};

struct MapsLine {
    uintptr_t start = 0;
    uintptr_t end = 0;
    uint64_t offset = 0;
    uint64_t inode = 0;
    char permissions[5] = {};
    unsigned devMajor = 0;
    unsigned devMinor = 0;
    std::string path;
};

bool parseMapsLine(const char *line, MapsLine &out) {
    unsigned long start = 0;
    unsigned long end = 0;
    unsigned long long offset = 0;
    unsigned long long inode = 0;
    char permissions[5] = {};
    char device[32] = {};
    char path[256] = {};
    if (sscanf(line, "%lx-%lx %4s %llx %31s %llu %255[^\n]", &start, &end, permissions, &offset,
               device, &inode, path) != 7) {
        return false;
    }

    std::string pathname(path);
    const size_t first = pathname.find_first_not_of(' ');
    if (first == std::string::npos) {
        return false;
    }
    pathname.erase(0, first);

    unsigned devMajor = 0;
    unsigned devMinor = 0;
    if (sscanf(device, "%x:%x", &devMajor, &devMinor) != 2) {
        return false;
    }

    out.start = start;
    out.end = end;
    out.offset = offset;
    out.inode = inode;
    memcpy(out.permissions, permissions, sizeof(out.permissions));
    out.devMajor = devMajor;
    out.devMinor = devMinor;
    out.path = std::move(pathname);
    return true;
}

int protFromPermissions(const char *permissions) {
    int prot = 0;
    if (permissions[0] == 'r') prot |= PROT_READ;
    if (permissions[1] == 'w') prot |= PROT_WRITE;
    if (permissions[2] == 'x') prot |= PROT_EXEC;
    return prot;
}

bool parseMappings(std::vector<Mapping> &mappings) {
    FILE *maps = fopen("/proc/self/maps", "r");
    if (maps == nullptr) {
        LOGE("[PROP-VIRT] cannot open /proc/self/maps: %s", strerror(errno));
        return false;
    }

    char line[512];
    while (fgets(line, sizeof(line), maps) != nullptr) {
        MapsLine entry = {};
        if (!parseMapsLine(line, entry) || !entry.path.starts_with(kPropertyDirectory)) {
            continue;
        }

        const int prot = protFromPermissions(entry.permissions);
        if (prot != PROT_READ || entry.start >= entry.end || entry.offset != 0) {
            continue;
        }

        auto *header = reinterpret_cast<const PropertyAreaHeader *>(entry.start);
        const size_t length = entry.end - entry.start;
        if (length < sizeof(*header) || header->magic != kPropertyAreaMagic ||
            header->version != kPropertyAreaVersion || header->bytesUsed > length - sizeof(*header)) {
            continue;
        }

        mappings.push_back({
                .start = entry.start,
                .end = entry.end,
                .prot = prot,
                .offset = entry.offset,
                .path = std::move(entry.path),
                .devMajor = entry.devMajor,
                .devMinor = entry.devMinor,
                .inode = entry.inode,
                .permLetter = entry.permissions[3],
                .bytesUsed = header->bytesUsed,
                .nextBytesUsed = header->bytesUsed,
        });
    }

    fclose(maps);
    if (mappings.empty()) {
        LOGE("[PROP-VIRT] no valid property areas found");
        return false;
    }
    if (g_verboseLogs > 0) {
        LOGD("[PROP-VIRT] parsed %zu property area mappings", mappings.size());
    }
    return true;
}

Mapping *mappingFor(State &state, const prop_info *info) {
    const uintptr_t address = reinterpret_cast<uintptr_t>(info);
    for (auto &mapping : state.mappings) {
        if (address >= mapping.start && address + sizeof(InlinePropertyInfo) <= mapping.end) {
            return &mapping;
        }
    }
    return nullptr;
}

const std::string *overrideFor(const State &state, std::string_view name) {
    const auto exact = state.exact.find(std::string(name));
    if (exact != state.exact.end()) return &exact->second;
    for (const auto &[suffix, value] : state.suffixes) {
        if (name.ends_with(suffix)) return &value;
    }
    return nullptr;
}

void enumerateProperty(void *cookie, const char *name, const char *value, uint32_t serial) {
    auto &state = *static_cast<State *>(cookie);
    if (!name || state.failure != Status::Applied) return;
    const auto *replacement = overrideFor(state, name);
    if (!replacement) return;
    ++state.matched;

    // Bionic only supports long encoding for read-only properties. Reject the
    // entire preparation instead of silently installing a partial profile.
    const bool longValue = replacement->size() >= PROP_VALUE_MAX;
    const bool readOnly = std::string_view(name).starts_with("ro.");
    if (replacement->find('\0') != std::string::npos ||
        (longValue && !readOnly)) {
        LOGE("unsupported property value for %s", name);
        state.failure = Status::UnsupportedValue;
        return;
    }
    const prop_info *info = __system_property_find(name);
    Mapping *mapping = info ? mappingFor(state, info) : nullptr;
    if (!mapping) {
        LOGE("no validated property area for %s", name);
        state.failure = Status::InvalidLayout;
        return;
    }

    Override entry{.mapping = mapping,
                   .offset = reinterpret_cast<uintptr_t>(info) - mapping->start,
                   .name = name, .value = *replacement};
    const size_t length = mapping->end - mapping->start;
    if (readOnly && (serial & kLongPropertyFlag) != 0) {
        const uintptr_t infoAddr = reinterpret_cast<uintptr_t>(info);
        const uintptr_t valueAddr = reinterpret_cast<uintptr_t>(value);
        if (!value || valueAddr < infoAddr + sizeof(InlinePropertyInfo) ||
            valueAddr >= mapping->end) {
            state.failure = Status::InvalidLayout;
            return;
        }
        entry.wasLong = true;
        entry.longOffset = valueAddr - infoAddr;
        entry.longLen = strnlen(value, mapping->end - valueAddr);
        if (entry.longLen == mapping->end - valueAddr) {
            state.failure = Status::InvalidLayout;
            return;
        }
    }
    if (longValue) {
        // Reuse an existing allocation when possible; otherwise reserve aligned
        // space in the private area's free tail using Bionic's data-relative ABI.
        if (entry.wasLong && replacement->size() <= entry.longLen) {
            entry.newLongOffset = entry.longOffset;
        } else {
            const size_t available = length - sizeof(PropertyAreaHeader);
            if (replacement->size() >= available) {
                state.failure = Status::UnsupportedValue;
                return;
            }
            const size_t allocation = (replacement->size() + 1U + 3U) & ~size_t(3U);
            const size_t used = mapping->nextBytesUsed;
            if (used > available || allocation > available - used ||
                sizeof(PropertyAreaHeader) + used < entry.offset) {
                state.failure = Status::UnsupportedValue;
                return;
            }
            entry.newLongOffset = sizeof(PropertyAreaHeader) + used - entry.offset;
            mapping->nextBytesUsed += static_cast<uint32_t>(allocation);
        }
    }
    state.overrides.push_back(std::move(entry));
}

void enumerateCallback(const prop_info *info, void *cookie) {
    if (info == nullptr) {
        return;
    }
    __system_property_read_callback(info, enumerateProperty, cookie);
}

// bionic opens property areas lazily on first access (ContextNode::CheckAccessAndOpen),
// so a warm-up pass forces every accessible area open BEFORE parseMappings snapshots
// /proc/self/maps. Without this, areas first opened during the real enumeration were
// missing from state.mappings and every property from them failed validation.
void warmupCallback(const prop_info *, void *) {}

bool preflightOpen(Mapping &mapping) {
    mapping.fd = open(mapping.path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (mapping.fd < 0) {
        LOGE("[PROP-VIRT] cannot open %s: %s", mapping.path.c_str(), strerror(errno));
        return false;
    }

    struct stat st = {};
    if (fstat(mapping.fd, &st) != 0) {
        LOGE("[PROP-VIRT] cannot stat %s: %s", mapping.path.c_str(), strerror(errno));
        return false;
    }
    if (major(st.st_dev) != mapping.devMajor || minor(st.st_dev) != mapping.devMinor ||
        static_cast<uint64_t>(st.st_ino) != mapping.inode ||
        st.st_size < static_cast<off_t>(mapping.end - mapping.start)) {
        LOGE("[PROP-VIRT] %s identity mismatch dev=%u:%u ino=%" PRIu64 " (mapped %u:%u ino=%" PRIu64 ")",
             mapping.path.c_str(), major(st.st_dev), minor(st.st_dev),
             static_cast<uint64_t>(st.st_ino), mapping.devMajor, mapping.devMinor, mapping.inode);
        return false;
    }

    PropertyAreaHeader header = {};
    const size_t length = mapping.end - mapping.start;
    const ssize_t read = pread(mapping.fd, &header, sizeof(header), 0);
    if (read != static_cast<ssize_t>(sizeof(header)) || header.magic != kPropertyAreaMagic ||
        header.version != kPropertyAreaVersion || header.bytesUsed > length - sizeof(header)) {
        LOGE("[PROP-VIRT] %s failed area header revalidation", mapping.path.c_str());
        return false;
    }
    const auto *current = reinterpret_cast<const PropertyAreaHeader *>(mapping.start);
    if (current->bytesUsed != mapping.bytesUsed) return false;
    if (mapping.permLetter == 'p') {
        // Another backend may have already changed this process's private view.
        // Preserve it across both installation and rollback; never write it to
        // the shared backing file. Copy only differing pages to limit COW cost.
        const auto *start = reinterpret_cast<const char *>(mapping.start);
        mapping.privateSnapshot.assign(start, start + length);
    }
    return true;
}

void restorePrivatePages(const Mapping &mapping) {
    if (mapping.privateSnapshot.empty()) return;
    const long pageSize = sysconf(_SC_PAGESIZE);
    const size_t step = pageSize > 0 ? size_t(pageSize) : mapping.privateSnapshot.size();
    for (size_t offset = 0; offset < mapping.privateSnapshot.size(); offset += step) {
        const size_t count = std::min(step, mapping.privateSnapshot.size() - offset);
        void *page = reinterpret_cast<void *>(mapping.start + offset);
        const void *original = mapping.privateSnapshot.data() + offset;
        if (memcmp(page, original, count) != 0) memcpy(page, original, count);
    }
}

bool remapPrivate(Mapping &mapping) {
    const size_t length = mapping.end - mapping.start;
    // MAP_FIXED may discard the old VMA before an eventual mmap failure.
    // Include the attempted area in rollback even when mmap returns MAP_FAILED.
    mapping.replaced = true;
    void *remapped = mmap(reinterpret_cast<void *>(mapping.start), length,
                          PROT_READ | PROT_WRITE, MAP_FIXED | MAP_PRIVATE, mapping.fd, 0);
    if (remapped == MAP_FAILED) {
        // Logging may itself read properties. Defer it until rollback restores
        // a usable address space and the caller receives the result.
        return false;
    }
    restorePrivatePages(mapping);
    return true;
}

void applyOverride(const Mapping &mapping, const Override &override) {
    auto *info = reinterpret_cast<InlinePropertyInfo *>(mapping.start + override.offset);
    const uint32_t oldSerial = __atomic_load_n(&info->serial, __ATOMIC_RELAXED);
    __atomic_store_n(&info->serial, oldSerial | 1U, __ATOMIC_RELAXED);
    __atomic_thread_fence(__ATOMIC_RELEASE);

    if (override.wasLong) {
        memset(reinterpret_cast<void *>(mapping.start + override.offset + override.longOffset),
               0, override.longLen + 1);
    }
    memset(info->value, 0, sizeof(info->value));
    uint32_t lengthFlags;
    if (override.newLongOffset != 0) {
        constexpr char legacyError[] = "Must use __system_property_read_callback() to read";
        constexpr size_t offsetField = 56;
        static_assert(sizeof(legacyError) < offsetField);
        memcpy(info->value, legacyError, sizeof(legacyError));
        const auto offset = static_cast<uint32_t>(override.newLongOffset);
        memcpy(info->value + offsetField, &offset, sizeof(offset));
        memcpy(reinterpret_cast<void *>(mapping.start + override.offset + offset),
               override.value.c_str(), override.value.size() + 1);
        lengthFlags = (uint32_t(sizeof(legacyError) - 1) << 24) | kLongPropertyFlag;
    } else {
        memcpy(info->value, override.value.c_str(), override.value.size() + 1);
        lengthFlags = uint32_t(override.value.size()) << 24;
    }
    // The long flag occupies counter space only for read-only properties.
    const uint32_t counterMask = override.name.starts_with("ro.")
        ? 0x00ffffffU & ~kLongPropertyFlag : 0x00ffffffU;
    const uint32_t serial = lengthFlags | ((oldSerial + 2U) & counterMask & ~1U);
    __atomic_store_n(&info->serial, serial, __ATOMIC_RELEASE);
    syscall(__NR_futex, &info->serial, FUTEX_WAKE, INT_MAX, nullptr, nullptr, 0);
}

bool verifyOverride(const Override &override) {
    const prop_info *info = __system_property_find(override.name.c_str());
    const auto address = override.mapping->start + override.offset;
    if (reinterpret_cast<uintptr_t>(info) != address) return false;
    struct Verification {
        const Override &rule;
        bool matches = false;
    } check{override};
    __system_property_read_callback(info, [](void *cookie, const char *name, const char *value, uint32_t) {
        auto &check = *static_cast<Verification *>(cookie);
        check.matches = name && value && check.rule.name == name && check.rule.value == value;
    }, &check);
    if (!check.matches) return false;
    const auto *raw = reinterpret_cast<const InlinePropertyInfo *>(address);
    const char *rawValue = override.newLongOffset
        ? reinterpret_cast<const char *>(address + override.newLongOffset) : raw->value;
    if (override.value != rawValue) return false;
    if (override.newLongOffset == 0) {
        char value[PROP_VALUE_MAX] = {};
        return __system_property_get(override.name.c_str(), value) >= 0 && override.value == value;
    }
    return true;
}

bool verifyMappingsIdentity(const std::vector<Mapping *> &affected) {
    FILE *maps = fopen("/proc/self/maps", "r");
    if (maps == nullptr) {
        LOGE("[PROP-VIRT] cannot reopen /proc/self/maps: %s", strerror(errno));
        return false;
    }

    std::vector<Mapping *> pending = affected;
    bool ok = true;
    char line[512];
    while (!pending.empty() && fgets(line, sizeof(line), maps) != nullptr) {
        MapsLine entry = {};
        if (!parseMapsLine(line, entry)) {
            continue;
        }
        auto it = std::find_if(pending.begin(), pending.end(),
                               [&](const Mapping *m) { return m->start == entry.start; });
        if (it == pending.end()) {
            continue;
        }
        const Mapping &mapping = **it;
        pending.erase(it);

        const char access[4] = {
                (mapping.prot & PROT_READ) != 0 ? 'r' : '-',
                (mapping.prot & PROT_WRITE) != 0 ? 'w' : '-',
                (mapping.prot & PROT_EXEC) != 0 ? 'x' : '-',
                '\0',
        };

        const bool match = entry.end == mapping.end && entry.offset == 0 &&
                           entry.devMajor == mapping.devMajor && entry.devMinor == mapping.devMinor &&
                           entry.inode == mapping.inode && entry.path == mapping.path &&
                           entry.permissions[3] == 'p' &&
                           strncmp(entry.permissions, access, 3) == 0;
        if (!match) {
            LOGE("[PROP-VIRT] identity check failed for %s: got %s dev=%u:%u ino=%" PRIu64
                 " path=%s (expected dev=%02x:%02x ino=%" PRIu64 ")",
                 mapping.path.c_str(), entry.permissions, entry.devMajor, entry.devMinor,
                 entry.inode, entry.path.c_str(), mapping.devMajor, mapping.devMinor,
                 mapping.inode);
            ok = false;
        }
    }
    fclose(maps);

    for (const Mapping *mapping : pending) {
        LOGE("[PROP-VIRT] VMA at start=%p for %s vanished from /proc/self/maps",
             reinterpret_cast<void *>(mapping->start), mapping->path.c_str());
        ok = false;
    }
    return ok;
}

void cleanup(std::vector<Mapping> &mappings) {
    for (auto &mapping : mappings) {
        if (mapping.fd >= 0) {
            close(mapping.fd);
            mapping.fd = -1;
        }
    }
}

bool rollback(std::vector<Mapping> &mappings) {
    bool ok = true;
    for (auto &mapping : mappings) {
        if (!mapping.replaced || mapping.fd < 0) continue;
        const int flags = MAP_FIXED | (mapping.permLetter == 's' ? MAP_SHARED : MAP_PRIVATE);
        const int prot = mapping.privateSnapshot.empty() ? mapping.prot : PROT_READ | PROT_WRITE;
        void *restored = mmap(reinterpret_cast<void *>(mapping.start), mapping.end - mapping.start,
                             prot, flags, mapping.fd, 0);
        if (restored == MAP_FAILED) {
            ok = false;
        } else {
            restorePrivatePages(mapping);
            if (prot != mapping.prot &&
                mprotect(restored, mapping.end - mapping.start, mapping.prot) != 0) {
                ok = false;
                continue;
            }
            mapping.replaced = false;
        }
    }
    return ok;
}

} // namespace

Result install(const std::unordered_map<std::string, std::string> &exact,
               const std::vector<std::pair<std::string, std::string>> &suffixes) {
    if (exact.empty() && suffixes.empty()) return {Status::NoMatches};
    State state{.exact = exact, .suffixes = suffixes};
    // Open lazily mapped areas before collecting their addresses.
    if (__system_property_foreach(warmupCallback, nullptr) != 0 ||
        !parseMappings(state.mappings)) return {Status::InvalidLayout};
    if (__system_property_foreach(enumerateCallback, &state) != 0)
        state.failure = Status::InvalidLayout;
    if (state.failure != Status::Applied) return {state.failure, state.matched};
    if (state.overrides.empty()) return {Status::NoMatches};

    std::vector<Mapping *> affected;
    for (const auto &override : state.overrides) {
        if (std::find(affected.begin(), affected.end(), override.mapping) == affected.end())
            affected.push_back(override.mapping);
    }
    auto finish = [&](Status status) -> Result {
        if (status != Status::Applied && !rollback(state.mappings)) status = Status::RollbackFailed;
        cleanup(state.mappings);
        return {status, state.matched, status == Status::Applied ? state.overrides.size() : 0};
    };
    for (Mapping *mapping : affected)
        if (!preflightOpen(*mapping)) return finish(Status::MappingFailed);
    for (Mapping *mapping : affected)
        if (!remapPrivate(*mapping)) return finish(Status::MappingFailed);
    for (Mapping *mapping : affected) {
        auto *header = reinterpret_cast<PropertyAreaHeader *>(mapping->start);
        header->bytesUsed = mapping->nextBytesUsed;
    }
    for (const auto &override : state.overrides)
        applyOverride(*override.mapping, override);
    for (Mapping *mapping : affected)
        if (mprotect(reinterpret_cast<void *>(mapping->start), mapping->end - mapping->start,
                     mapping->prot) != 0) return finish(Status::MappingFailed);
    for (const auto &override : state.overrides)
        if (!verifyOverride(override)) return finish(Status::VerificationFailed);
    if (!verifyMappingsIdentity(affected)) return finish(Status::VerificationFailed);
    return finish(Status::Applied);
}

const char *statusName(Status status) {
    switch (status) {
        case Status::Applied: return "applied";
        case Status::NoMatches: return "no_matches";
        case Status::InvalidLayout: return "invalid_layout";
        case Status::UnsupportedValue: return "unsupported_value";
        case Status::MappingFailed: return "mapping_failed";
        case Status::VerificationFailed: return "verification_failed";
        case Status::RollbackFailed: return "rollback_failed";
    }
    return "unknown";
}

} // namespace property_virtualizer

