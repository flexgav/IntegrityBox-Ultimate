// Bionic property-area formats for adding a property in a private mapping.
// Mirrors AOSP main (aosp-mirror): bionic libc/system_properties/prop_area.cpp
// (find_property, find_prop_trie_node, allocate_obj), prop_info.h, and
// system/core property_service/libpropertyinfoparser/property_info_parser.cpp
// (GetPropertyInfoIndexes). Every offset read from the data is bounds-checked.
#include "property_area.hpp"

#include <sys/system_properties.h>

#include <cstdint>
#include <cstring>

namespace property_area {
namespace {

// prop_area header: bytes_used, serial, magic, version, reserved[28].
constexpr size_t kAreaHeaderSize = 128;
// prop_trie_node: namelen, prop, left, right, children, then the name.
constexpr size_t kNodeSize = 20;
constexpr size_t kNodeProp = 4;
constexpr size_t kNodeLeft = 8;
constexpr size_t kNodeRight = 12;
constexpr size_t kNodeChildren = 16;
// prop_info: serial and the PROP_VALUE_MAX value union, then the full name.
constexpr size_t kInfoSize = 4 + PROP_VALUE_MAX;
// Bounds a walk through a damaged or cyclic tree.
constexpr size_t kMaxSteps = 1U << 16;
constexpr uint32_t kNoIndex = ~0U;

size_t align4(size_t size) { return (size + 3U) & ~size_t(3U); }

uint32_t load32(const char *address) {
    uint32_t value;
    memcpy(&value, address, sizeof(value));
    return value;
}

// Splits `rest` at the next '.'. An empty segment makes the name invalid.
bool nextSegment(std::string_view &rest, std::string_view &segment, bool &last) {
    const auto dot = rest.find('.');
    last = dot == rest.npos;
    segment = rest.substr(0, dot);
    rest = last ? std::string_view() : rest.substr(dot + 1);
    return !segment.empty();
}

// bionic cmp_prop_name: shorter names sort first, equal lengths by bytes.
int compareSegment(std::string_view one, std::string_view two) {
    if (one.size() != two.size()) return one.size() < two.size() ? -1 : 1;
    return memcmp(one.data(), two.data(), one.size());
}

// Read-only view of the trie stored after the area header.
class Trie {
public:
    Trie(const char *area, size_t length)
        : data_(length > kAreaHeaderSize ? area + kAreaHeaderSize : nullptr),
          size_(length > kAreaHeaderSize ? length - kAreaHeaderSize : 0) {}

    // Name of the node at `offset`; nullopt when the node does not fit the area.
    // Only the root has an empty name, and it is never terminated: bionic places
    // the dirty backup area (old values during updates) right after it.
    std::optional<std::string_view> name(uint32_t offset) const {
        if (!data_ || offset % 4 != 0 || offset > size_ || size_ - offset < kNodeSize) return std::nullopt;
        const uint32_t length = load32(data_ + offset);
        if (length == 0) return std::string_view();
        if (length >= size_ - offset - kNodeSize || data_[offset + kNodeSize + length] != '\0')
            return std::nullopt;
        return std::string_view(data_ + offset + kNodeSize, length);
    }

    // Field of a node already validated by name().
    uint32_t field(uint32_t offset, size_t field) const { return load32(data_ + offset + field); }

private:
    const char *data_;
    size_t size_;
};

// Where the path of a name ends in the trie. A complete walk stops at the
// leaf node. Otherwise `node`'s `link` field receives the first new node, for
// `segment`, followed by the segments in `rest`.
struct Walk {
    uint32_t node = 0;
    size_t link = kNodeChildren;
    std::string_view segment;
    std::string_view rest;
    bool last = false;
    bool complete = false;
};

// Follows bionic find_property without allocating.
std::optional<Walk> walk(const Trie &trie, std::string_view name) {
    if (name.empty() || name.find('\0') != name.npos || !trie.name(0)) return std::nullopt;
    Walk state;
    std::string_view rest = name;
    size_t steps = 0;
    while (true) {
        std::string_view segment;
        bool last = false;
        if (!nextSegment(rest, segment, last)) return std::nullopt;
        uint32_t parent = state.node;
        size_t link = kNodeChildren;
        uint32_t current = trie.field(parent, link);
        while (current != 0) {
            if (++steps > kMaxSteps) return std::nullopt;
            const auto currentName = trie.name(current);
            if (!currentName) return std::nullopt;
            const int order = compareSegment(segment, *currentName);
            if (order == 0) break;
            parent = current;
            link = order < 0 ? kNodeLeft : kNodeRight;
            current = trie.field(current, link);
        }
        if (current == 0) {
            state.node = parent;
            state.link = link;
            state.segment = segment;
            state.rest = rest;
            state.last = last;
            return state;
        }
        state.node = current;
        if (last) {
            state.complete = true;
            return state;
        }
    }
}

// Bounds-checked reader for the serialized property_info data.
class PropertyInfo {
public:
    explicit PropertyInfo(std::string_view bytes) : bytes_(bytes) {}

    bool u32(uint64_t offset, uint32_t &value) const {
        if (offset > bytes_.size() || bytes_.size() - offset < sizeof(value)) return false;
        value = load32(bytes_.data() + offset);
        return true;
    }

    // NUL-terminated string at `offset`.
    bool string(uint32_t offset, std::string_view &value) const {
        if (offset >= bytes_.size()) return false;
        const char *start = bytes_.data() + offset;
        const void *end = memchr(start, '\0', bytes_.size() - offset);
        if (!end) return false;
        value = std::string_view(start, static_cast<const char *>(end) - start);
        return true;
    }

    // Element `index` of a uint32_t array that starts at `array`.
    bool element(uint64_t array, uint32_t index, uint32_t &value) const {
        return u32(array + uint64_t(index) * sizeof(uint32_t), value);
    }

    // An array of `count` uint32_t values cannot be larger than the data.
    bool fits(uint32_t count) const { return uint64_t(count) * sizeof(uint32_t) <= bytes_.size(); }

private:
    std::string_view bytes_;
};

// PropertyEntry: name_offset, namelen, context_index, type_index.
struct Entry {
    uint32_t nameLength = 0;
    uint32_t context = kNoIndex;
    std::string_view name;
};

// TrieNodeInternal: property_entry, then count/offset pairs for the sorted
// children, the prefixes (longest first) and the sorted exact matches.
struct Node {
    uint32_t entry = 0;
    uint32_t childCount = 0;
    uint32_t children = 0;
    uint32_t prefixCount = 0;
    uint32_t prefixes = 0;
    uint32_t exactCount = 0;
    uint32_t exacts = 0;
};

bool readEntry(const PropertyInfo &info, uint32_t offset, Entry &entry) {
    uint32_t nameOffset = 0;
    uint32_t type = 0;
    return info.u32(offset, nameOffset) && info.u32(uint64_t(offset) + 4, entry.nameLength) &&
           info.u32(uint64_t(offset) + 8, entry.context) && info.u32(uint64_t(offset) + 12, type) &&
           info.string(nameOffset, entry.name);
}

bool readNode(const PropertyInfo &info, uint32_t offset, Node &node) {
    uint32_t *const fields[] = {&node.entry, &node.childCount, &node.children, &node.prefixCount,
                                &node.prefixes, &node.exactCount, &node.exacts};
    for (size_t i = 0; i < sizeof(fields) / sizeof(fields[0]); ++i)
        if (!info.u32(uint64_t(offset) + i * sizeof(uint32_t), *fields[i])) return false;
    return info.fits(node.childCount) && info.fits(node.prefixCount) && info.fits(node.exactCount);
}

// CheckPrefixMatch: the first prefix that starts `remaining` wins.
bool checkPrefixes(const PropertyInfo &info, const Node &node, std::string_view remaining,
                   uint32_t &context) {
    for (uint32_t i = 0; i < node.prefixCount; ++i) {
        uint32_t offset = 0;
        Entry prefix;
        if (!info.element(node.prefixes, i, offset) || !readEntry(info, offset, prefix)) return false;
        if (prefix.nameLength > remaining.size()) continue;
        if (strncmp(prefix.name.data(), remaining.data(), prefix.nameLength) == 0) {
            if (prefix.context != kNoIndex) context = prefix.context;
            return true;
        }
    }
    return true;
}

// FindChildForString: binary search, a longer child name sorts after a prefix.
bool findChild(const PropertyInfo &info, const Node &node, std::string_view segment, Node &child,
               bool &found) {
    found = false;
    int64_t bottom = 0;
    int64_t top = int64_t(node.childCount) - 1;
    while (top >= bottom) {
        const int64_t middle = (top + bottom) / 2;
        uint32_t offset = 0;
        Node candidate;
        Entry entry;
        if (!info.element(node.children, uint32_t(middle), offset) || !readNode(info, offset, candidate) ||
            !readEntry(info, candidate.entry, entry)) return false;
        int order = strncmp(entry.name.data(), segment.data(), segment.size());
        if (order == 0 && entry.name.size() > segment.size()) order = 1;
        if (order == 0) {
            child = candidate;
            found = true;
            return true;
        }
        if (order < 0) bottom = middle + 1;
        else top = middle - 1;
    }
    return true;
}

} // namespace

std::optional<std::string> contextFor(std::string_view bytes, std::string_view name) {
    const PropertyInfo info(bytes);
    // PropertyInfoAreaHeader: current_version, minimum_supported_version, size,
    // contexts_offset, types_offset, root_offset.
    uint32_t minimumVersion = 0;
    uint32_t size = 0;
    uint32_t contexts = 0;
    uint32_t root = 0;
    if (!info.u32(4, minimumVersion) || !info.u32(8, size) || !info.u32(12, contexts) ||
        !info.u32(20, root) || minimumVersion > 1 || size != bytes.size() || name.empty() ||
        name.find('\0') != name.npos) return std::nullopt;

    uint32_t context = kNoIndex;
    Node node;
    if (!readNode(info, root, node)) return std::nullopt;
    std::string_view remaining = name;
    while (true) {
        Entry entry;
        if (!readEntry(info, node.entry, entry)) return std::nullopt;
        if (entry.context != kNoIndex) context = entry.context;
        if (!checkPrefixes(info, node, remaining, context)) return std::nullopt;
        const auto dot = remaining.find('.');
        if (dot == remaining.npos) break;
        Node child;
        bool found = false;
        if (!findChild(info, node, remaining.substr(0, dot), child, found)) return std::nullopt;
        if (!found) break;
        node = child;
        remaining = remaining.substr(dot + 1);
    }

    bool exact = false;
    for (uint32_t i = 0; i < node.exactCount && !exact; ++i) {
        uint32_t offset = 0;
        Entry entry;
        if (!info.element(node.exacts, i, offset) || !readEntry(info, offset, entry)) return std::nullopt;
        if (entry.name == remaining) {
            if (entry.context != kNoIndex) context = entry.context;
            exact = true;
        }
    }
    if (!exact && !checkPrefixes(info, node, remaining, context)) return std::nullopt;

    uint32_t count = 0;
    uint32_t offset = 0;
    std::string_view text;
    if (context == kNoIndex || !info.u32(contexts, count) || context >= count ||
        !info.element(uint64_t(contexts) + sizeof(uint32_t), context, offset) ||
        !info.string(offset, text) || text.empty()) return std::nullopt;
    return std::string(text);
}

std::optional<size_t> insertionSize(const char *area, size_t length, std::string_view name) {
    if (name.size() > length) return std::nullopt;
    const Trie trie(area, length);
    const auto state = walk(trie, name);
    if (!state) return std::nullopt;
    size_t bytes = align4(kInfoSize + name.size() + 1);
    if (state->complete) return trie.field(state->node, kNodeProp) != 0 ? 0 : bytes;
    std::string_view segment = state->segment;
    std::string_view rest = state->rest;
    bool last = state->last;
    while (true) {
        bytes += align4(kNodeSize + segment.size() + 1);
        if (last) return bytes;
        if (!nextSegment(rest, segment, last)) return std::nullopt;
    }
}

bool insert(char *area, size_t length, std::string_view name, std::string_view value) {
    if (value.size() >= PROP_VALUE_MAX || value.find('\0') != value.npos || length <= kAreaHeaderSize)
        return false;
    const auto bytes = insertionSize(area, length, name);
    if (!bytes || *bytes == 0) return false;
    const size_t dataSize = length - kAreaHeaderSize;
    uint32_t used = load32(area);
    if (used % 4 != 0 || used > dataSize || *bytes > dataSize - used) return false;

    const auto state = *walk(Trie(area, length), name);
    char *const data = area + kAreaHeaderSize;
    // allocate_obj: 4-byte aligned objects appended at bytes_used.
    const auto allocate = [&](size_t size) {
        const uint32_t offset = used;
        const size_t aligned = align4(size);
        memset(data + offset, 0, aligned);
        used += static_cast<uint32_t>(aligned);
        return offset;
    };
    // A link is stored only after its target is complete, like bionic's release stores.
    const auto publish = [&](uint32_t node, size_t field, uint32_t offset) {
        __atomic_store_n(reinterpret_cast<uint32_t *>(data + node + field), offset, __ATOMIC_RELEASE);
    };

    uint32_t leaf = state.node;
    if (!state.complete) {
        std::string_view segment = state.segment;
        std::string_view rest = state.rest;
        bool last = state.last;
        uint32_t parent = state.node;
        size_t link = state.link;
        while (true) {
            const uint32_t node = allocate(kNodeSize + segment.size() + 1);
            const auto segmentLength = static_cast<uint32_t>(segment.size());
            memcpy(data + node, &segmentLength, sizeof(segmentLength));
            memcpy(data + node + kNodeSize, segment.data(), segment.size());
            publish(parent, link, node);
            parent = node;
            link = kNodeChildren;
            if (last) break;
            nextSegment(rest, segment, last);
        }
        leaf = parent;
    }

    // prop_info for a short value: serial carries the length in the top byte.
    const uint32_t info = allocate(kInfoSize + name.size() + 1);
    memcpy(data + info + sizeof(uint32_t), value.data(), value.size());
    memcpy(data + info + kInfoSize, name.data(), name.size());
    __atomic_store_n(reinterpret_cast<uint32_t *>(data + info), uint32_t(value.size()) << 24,
                     __ATOMIC_RELAXED);
    publish(leaf, kNodeProp, info);
    memcpy(area, &used, sizeof(used));
    return true;
}

} // namespace property_area
