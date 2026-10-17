#pragma once

#include "runtime_config.hpp"
#include <chrono>
#include <cstddef>
#include <string>
#include <vector>

namespace companion_protocol {
inline constexpr uint32_t kMagic = 0x49425532;
inline constexpr uint32_t kVersion = 1;
inline constexpr uint32_t kMaxConfig = 512 * 1024;
inline constexpr uint32_t kMaxDex = 4 * 1024 * 1024;
inline constexpr uint32_t kMaxApps = 256 * 1024;
inline constexpr int kTimeoutMs = 3000;

struct Request { uint32_t magic = kMagic, version = kVersion, nameSize = 0; };
struct Reply {
    uint32_t magic = kMagic, version = kVersion;
    runtime::Role role = runtime::Role::None;
    uint32_t configSize = 0, dexSize = 0;
};
static_assert(sizeof(Request) == 12 && sizeof(Reply) == 20);

// Owns the companion socket. One absolute deadline covers the entire exchange,
// including partial transfers; EINTR or a slow sender cannot restart the timer.
class Channel {
public:
    explicit Channel(int fd, int timeoutMs = kTimeoutMs);
    ~Channel();
    Channel(const Channel &) = delete;
    Channel &operator=(const Channel &) = delete;
    bool read(void *data, size_t size);
    bool write(const void *data, size_t size);
private:
    bool transfer(void *data, size_t size, bool sending);
    int fd_;
    std::chrono::steady_clock::time_point deadline_;
};

struct Payload { runtime::Role role = runtime::Role::None; std::vector<char> config, dex; };
bool sendRequest(Channel &channel, const std::string &process);
bool receiveRequest(Channel &channel, std::string &process);
bool sendPayload(Channel &channel, const Payload &payload);
bool receivePayload(Channel &channel, Payload &payload);
bool readFile(const char *path, std::vector<char> &data, size_t maximum);
bool preparePayload(const std::string &process, const char *configPath, const char *appsPath, const char *dexPath, Payload &payload);
} // namespace companion_protocol
