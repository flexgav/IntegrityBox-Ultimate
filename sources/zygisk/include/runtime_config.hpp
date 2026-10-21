#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace runtime {
inline constexpr char kDroidGuard[] = "com.google.android.gms.unstable";
inline constexpr char kVending[] = "com.android.vending";
// Android packs the user into the UID: uid = userId * kPerUserRange + appId.
inline constexpr uint32_t kPerUserRange = 100000;
inline constexpr uint32_t kMaxUserId = INT32_MAX / kPerUserRange;
enum class Role : uint32_t { None, DroidGuard, Vending, App };
using Properties = std::unordered_map<std::string, std::string>;

struct Config {
    Properties values;
    int verbose = 0;
    bool build = true;
    bool props = true;
    bool provider = true;
    bool signature = false;
    bool vendingFinger = false;
    bool vendingSdk = false;
    bool pixel = false;
    bool apps = false;

    bool needsDex(Role role) const;
    Properties buildFields(Role role) const;
    Properties exactProperties() const;
    std::vector<std::pair<std::string, std::string>> suffixProperties() const;
};

bool parseConfig(std::string_view text, Config &out);
bool listedApp(std::string_view list, std::string_view process, uint32_t userId);
Role selectRole(std::string_view process, uint32_t userId, const Config &config, std::string_view apps);
bool validProcess(std::string_view process);
} // namespace runtime
