#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

// Bionic property storage, read and extended only inside a private mapping of
// the current process. Layouts follow AOSP bionic (prop_area, prop_trie_node,
// prop_info) and libpropertyinfoparser (/dev/__properties__/property_info).
namespace property_area {

// Context that bionic selects for `name` from the serialized property_info
// data (ContextsSerialized::GetPropAreaForName). nullopt when the data is
// malformed or names no context.
std::optional<std::string> contextFor(std::string_view propertyInfo, std::string_view name);

// Bytes that inserting `name` would allocate in the area mapped at `area`:
// 0 when the name already has a property, nullopt for an invalid name or a
// malformed trie.
std::optional<size_t> insertionSize(const char *area, size_t length, std::string_view name);

// Adds a property with a short value to a writable copy of an area, as bionic's
// prop_area::add does. Writes nothing and returns false when the name exists,
// is invalid, the trie is malformed or the area lacks free space.
bool insert(char *area, size_t length, std::string_view name, std::string_view value);

} // namespace property_area
