#include "runtime_config.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <climits>

namespace runtime {
namespace {
constexpr const char *kIdentityFields[] = {"BRAND", "MANUFACTURER", "MODEL", "DEVICE", "PRODUCT"};

std::string_view trim(std::string_view value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == value.npos) return {};
    return value.substr(first, value.find_last_not_of(" \t\r\n") - first + 1);
}

std::string value(const Properties &values, const char *key) {
    const auto it = values.find(key);
    return it == values.end() ? std::string() : it->second;
}

int number(const Properties &values, const char *key, int fallback = 0) {
    const auto text = value(values, key);
    if (text == "true") return 1;
    if (text == "false") return 0;
    int result = 0;
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), result);
    return parsed.ec == std::errc() && parsed.ptr == text.data() + text.size() ? result : fallback;
}

bool fieldName(std::string_view name) {
    return !name.empty() && name.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789") == name.npos;
}
} // namespace

bool parseConfig(std::string_view text, Config &out) {
    if (text.empty() || text.find('\0') != text.npos) return false;
    Config parsed;
    while (!text.empty()) {
        const auto end = text.find('\n');
        auto line = trim(text.substr(0, end));
        text = end == text.npos ? std::string_view() : text.substr(end + 1);
        line = trim(line.substr(0, line.find('#')));
        const auto separator = line.find('=');
        if (separator == line.npos) continue;
        const auto key = trim(line.substr(0, separator));
        const auto entry = trim(line.substr(separator + 1));
        if (key.empty() || key.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_.*-") != key.npos)
            return false;
        parsed.values[std::string(key)] = std::string(entry);
    }
    if (parsed.values.empty()) return false;
    parsed.verbose = std::max(0, number(parsed.values, "verboseLogs"));
    parsed.build = number(parsed.values, "spoofBuild", 1) > 0;
    parsed.props = number(parsed.values, "spoofProps", 1) > 0;
    parsed.provider = number(parsed.values, "spoofProvider", 1) > 0;
    parsed.signature = number(parsed.values, "spoofSignature") > 0;
    parsed.vendingSdk = number(parsed.values, "spoofVendingSdk") > 0;
    parsed.pixel = number(parsed.values, "spoofPixel") > 0;
    parsed.apps = number(parsed.values, "spoofApps") > 0;
    const auto finger = value(parsed.values, "spoofVendingFinger");
    parsed.vendingFinger = !finger.empty() && (finger.find_first_not_of("01") != finger.npos || number(parsed.values, "spoofVendingFinger") > 0);
    // Minimal imported profiles may omit expanded Build fields. Explicit values
    // remain authoritative; only missing fields are derived from the fingerprint.
    auto fingerprint = value(parsed.values, "FINGERPRINT");
    std::string_view remaining(fingerprint);
    std::array<std::string_view, 8> parts{};
    bool complete = true;
    constexpr char separators[] = "//://:/";
    for (size_t i = 0; i < parts.size(); ++i) {
        const auto end = remaining.find_first_of("/:");
        parts[i] = remaining.substr(0, end);
        if (parts[i].empty() || (i + 1 < parts.size() ? end == remaining.npos || remaining[end] != separators[i] : end != remaining.npos)) {
            complete = false;
            break;
        }
        remaining = end == remaining.npos ? std::string_view() : remaining.substr(end + 1);
    }
    const char *keys[] = {"BRAND", "PRODUCT", "DEVICE", "RELEASE", "ID", "INCREMENTAL", "TYPE", "TAGS"};
    if (complete) for (size_t i = 0; i < parts.size(); ++i)
        if (value(parsed.values, keys[i]).empty()) parsed.values[keys[i]] = std::string(parts[i]);
    out = std::move(parsed);
    return true;
}

bool Config::needsDex(Role role) const { return role == Role::DroidGuard && (provider || signature); }

Properties Config::buildFields(Role role) const {
    Properties result;
    if ((role == Role::DroidGuard || role == Role::App) && build) {
        for (const auto &[key, entry] : values)
            if (fieldName(key) && !entry.empty()) result[key] = entry;
    } else if (role == Role::Vending) {
        if (vendingSdk) result["SDK_INT"] = "29";
        if (vendingFinger) {
            const auto setting = value(values, "spoofVendingFinger");
            const auto fingerprint = setting.find_first_not_of("01") != setting.npos ? setting : value(values, "FINGERPRINT");
            if (!fingerprint.empty()) result["FINGERPRINT"] = fingerprint;
        }
        if (pixel) for (const char *key : kIdentityFields) {
            const auto entry = value(values, key);
            if (!entry.empty()) result[key] = entry;
        }
    }
    // These fields may already be cached in Build before property replacement.
    // KeyStore reads them ahead of the ordinary identity fields for attestation.
    // Derive only identities this role's enabled operations actually selected.
    for (const char *key : kIdentityFields) {
        const auto identity = result.find(key);
        if (identity == result.end()) continue;
        const auto name = std::string(key) + "_FOR_ATTESTATION";
        const auto explicitValue = value(values, name.c_str());
        result.try_emplace(name, explicitValue.empty() ? identity->second : explicitValue);
    }
    return result;
}

Properties Config::exactProperties() const {
    Properties result;
    for (const auto &[key, entry] : values)
        if (key.front() != '*' && key.find('.') != key.npos && !entry.empty()) result[key] = entry;
    return result;
}

std::vector<std::pair<std::string, std::string>> Config::suffixProperties() const {
    std::vector<std::pair<std::string, std::string>> result;
    for (const auto &[key, entry] : values)
        if (key.front() == '*' && !entry.empty()) result.emplace_back(key.substr(1), entry);
    std::sort(result.begin(), result.end(), [](const auto &a, const auto &b) {
        return a.first.size() != b.first.size() ? a.first.size() > b.first.size() : a.first < b.first;
    });
    return result;
}

bool validProcess(std::string_view process) {
    return !process.empty() && process.size() <= 1024 &&
        process.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_.:") == process.npos;
}

// A plain entry names the process in the primary user; "name@N" names it in
// user N only. A malformed user suffix matches nothing.
static bool listedEntry(std::string_view entry, std::string_view process, uint32_t userId) {
    uint32_t entryUser = 0;
    const auto at = entry.rfind('@');
    if (at != entry.npos) {
        const auto digits = entry.substr(at + 1);
        const auto parsed = std::from_chars(digits.data(), digits.data() + digits.size(), entryUser);
        if (digits.empty() || parsed.ec != std::errc() || parsed.ptr != digits.data() + digits.size()) return false;
        entry = entry.substr(0, at);
    }
    return entry == process && entryUser == userId;
}

bool listedApp(std::string_view list, std::string_view process, uint32_t userId) {
    while (!list.empty()) {
        const auto end = list.find('\n');
        auto line = list.substr(0, end);
        line = trim(line.substr(0, line.find('#')));
        if (!line.empty() && listedEntry(line, process, userId)) return true;
        list = end == list.npos ? std::string_view() : list.substr(end + 1);
    }
    return false;
}

Role selectRole(std::string_view process, uint32_t userId, const Config &config, std::string_view apps) {
    if (!validProcess(process) || userId > kMaxUserId) return Role::None;
    if (process == kDroidGuard) return Role::DroidGuard;
    if (process == kVending) return Role::Vending;
    return config.apps && listedApp(apps, process, userId) ? Role::App : Role::None;
}
} // namespace runtime
