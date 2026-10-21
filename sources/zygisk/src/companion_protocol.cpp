#include "companion_protocol.hpp"

#include <algorithm>
#include <cerrno>
#include <climits>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

namespace companion_protocol {
Channel::Channel(int fd, int timeoutMs) : fd_(fd), deadline_(std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs)) {}
Channel::~Channel() { if (fd_ >= 0) close(fd_); }
bool Channel::read(void *data, size_t size) { return transfer(data, size, false); }
bool Channel::write(const void *data, size_t size) { return transfer(const_cast<void *>(data), size, true); }

bool Channel::transfer(void *data, size_t size, bool sending) {
    if (fd_ < 0) return false;
    auto *bytes = static_cast<char *>(data);
    size_t done = 0;
    while (done < size) {
        const auto left = std::chrono::duration_cast<std::chrono::milliseconds>(deadline_ - std::chrono::steady_clock::now()).count();
        if (left <= 0) return false;
        pollfd descriptor{fd_, static_cast<short>(sending ? POLLOUT : POLLIN), 0};
        const int ready = poll(&descriptor, 1, static_cast<int>(std::min<int64_t>(left, INT_MAX)));
        if (ready < 0 && errno == EINTR) continue;
        if (ready <= 0 || (descriptor.revents & (POLLNVAL | POLLERR))) return false;
        const auto count = sending ? send(fd_, bytes + done, size - done, MSG_DONTWAIT | MSG_NOSIGNAL)
                                   : recv(fd_, bytes + done, size - done, MSG_DONTWAIT);
        if (count < 0 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)) continue;
        if (count <= 0) return false;
        done += static_cast<size_t>(count);
    }
    return true;
}

bool sendRequest(Channel &channel, const std::string &process, uint32_t userId) {
    if (!runtime::validProcess(process) || userId > runtime::kMaxUserId) return false;
    const Request request{kMagic, kVersion, static_cast<uint32_t>(process.size()), userId};
    return channel.write(&request, sizeof(request)) && channel.write(process.data(), process.size());
}

bool receiveRequest(Channel &channel, std::string &process, uint32_t &userId) {
    Request request;
    if (!channel.read(&request, sizeof(request)) || request.magic != kMagic || request.version != kVersion ||
        !request.nameSize || request.nameSize > 1024 || request.userId > runtime::kMaxUserId) return false;
    std::string name(request.nameSize, '\0');
    if (!channel.read(name.data(), name.size()) || !runtime::validProcess(name)) return false;
    process = std::move(name);
    userId = request.userId;
    return true;
}

static bool validReply(const Reply &reply) {
    if (reply.magic != kMagic || reply.version != kVersion || reply.configSize > kMaxConfig || reply.dexSize > kMaxDex) return false;
    if (reply.role == runtime::Role::None) return !reply.configSize && !reply.dexSize;
    return reply.role <= runtime::Role::App && reply.configSize &&
        (reply.role == runtime::Role::DroidGuard || !reply.dexSize);
}

bool sendPayload(Channel &channel, const Payload &payload) {
    if (payload.config.size() > kMaxConfig || payload.dex.size() > kMaxDex) return false;
    const Reply reply{kMagic, kVersion, payload.role, static_cast<uint32_t>(payload.config.size()), static_cast<uint32_t>(payload.dex.size())};
    return validReply(reply) && channel.write(&reply, sizeof(reply)) &&
        channel.write(payload.config.data(), payload.config.size()) && channel.write(payload.dex.data(), payload.dex.size());
}

bool receivePayload(Channel &channel, Payload &payload) {
    Reply reply;
    if (!channel.read(&reply, sizeof(reply)) || !validReply(reply)) return false;
    Payload parsed;
    parsed.role = reply.role;
    parsed.config.resize(reply.configSize);
    parsed.dex.resize(reply.dexSize);
    if (!channel.read(parsed.config.data(), parsed.config.size()) || !channel.read(parsed.dex.data(), parsed.dex.size())) return false;
    payload = std::move(parsed);
    return true;
}

bool readFile(const char *path, std::vector<char> &data, size_t maximum) {
    data.clear();
    const int fd = open(path, O_RDONLY | O_CLOEXEC | O_NONBLOCK);
    if (fd < 0) return false;
    struct stat info{};
    bool ok = fstat(fd, &info) == 0 && S_ISREG(info.st_mode) && info.st_size > 0 && static_cast<uint64_t>(info.st_size) <= maximum;
    std::vector<char> bytes;
    if (ok) {
        bytes.resize(static_cast<size_t>(info.st_size));
        size_t done = 0;
        while (done < bytes.size()) {
            const auto count = ::read(fd, bytes.data() + done, bytes.size() - done);
            if (count < 0 && errno == EINTR) continue;
            if (count <= 0) { ok = false; break; }
            done += static_cast<size_t>(count);
        }
        char trailing;
        if (ok && ::read(fd, &trailing, 1) != 0) ok = false;
    }
    close(fd);
    if (ok) data = std::move(bytes);
    return ok;
}

bool preparePayload(const std::string &process, uint32_t userId, const char *configPath, const char *appsPath, const char *dexPath, Payload &payload) {
    Payload prepared;
    runtime::Config config;
    if (!readFile(configPath, prepared.config, kMaxConfig) ||
        !runtime::parseConfig({prepared.config.data(), prepared.config.size()}, config)) return false;
    std::vector<char> apps;
    if (process != runtime::kDroidGuard && process != runtime::kVending && config.apps)
        readFile(appsPath, apps, kMaxApps);
    prepared.role = runtime::selectRole(process, userId, config, {apps.data(), apps.size()});
    if (prepared.role == runtime::Role::None) prepared.config.clear();
    else if (config.needsDex(prepared.role)) {
        // An optional payload failure does not discard independent native work.
        readFile(dexPath, prepared.dex, kMaxDex);
    }
    payload = std::move(prepared);
    return true;
}
} // namespace companion_protocol
