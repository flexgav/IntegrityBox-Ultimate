#include <android/log.h>
#include <sys/system_properties.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <cerrno>
#include <climits>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include <dlfcn.h>
#include "zygisk.hpp"
#include "shadowhook.h"

// ShadowHook tracks the Bionic linker via its own sh_linker_init; on newer
// Android that tracking can crash the host. PIFork v18 neutralizes it with a
// dummy strong symbol (CMake renames the library's own sh_linker_init to
// sh_linker_init_dummy so this one wins). Keep in sync with the CMake PATCH_COMMAND.
extern "C" int sh_linker_init(void) { return 0; }

#define DEX_FILE_PATH "/data/adb/modules/playintegrityfix/classes.dex"
#define CUSTOM_PROP_FILE_PATH "/data/adb/modules/playintegrityfix/cust.spf.prop"
#define APPS_LIST_FILE_PATH "/data/adb/modules/playintegrityfix/spfapps.list"
#define VENDING_PACKAGE "com.android.vending"
#define DROIDGUARD_PACKAGE "com.google.android.gms.unstable"
// Cheap, app-readable gate for per-app spoofing. Set to "1" at boot only when
// the feature is enabled; absent/"0" otherwise. Lets non-GMS apps skip the
// companion round-trip entirely when the feature is off (zero overhead).
// Deliberately neutral name: the literal survives --strip-all in .rodata, so it
// must not carry tokens (spoof/pif/...) that property-name detectors look for.
#define SPOOFAPPS_PROP "sys.spfapps.cfg"

// Runtime-gated logging: with verboseLogs=0 nothing reaches logcat (D1: smaller
// detection surface). The tag literal survives strip in .rodata, so it is kept
// neutral ("sysprop") to avoid product/keyword tokens detectors scan for; grep
// logcat by this tag for diagnostics (verboseLogs>0).
#define LOG_TAG "sysprop"
static int g_verboseLogs = 0;

#define LOGD(...)                                                              \
    do                                                                         \
    {                                                                          \
        if (g_verboseLogs > 0)                                                 \
            __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__); \
    } while (0)
#define LOGI(...)                                                             \
    do                                                                        \
    {                                                                         \
        if (g_verboseLogs > 0)                                                \
            __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__); \
    } while (0)
#define LOGE(...)                                                              \
    do                                                                         \
    {                                                                          \
        if (g_verboseLogs > 0)                                                 \
            __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__); \
    } while (0)

static int spoofBuild = 1;
static int spoofProps = 1;
static int spoofProvider = 1;
static int spoofSignature = 0;
static int spoofVendingFinger = 0;
static int spoofVendingSdk = 0;
static int spoofPixel = 0;

static std::string pixelManufacturer;
static std::string pixelModel;
static std::string pixelDevice;
static std::string pixelBrand;
static std::string vendingFingerprintValue;

static std::unordered_map<std::string, std::string> propMap;

// Native-hook spoof targets, split for a hot read path that runs on every
// property read in GMS:
//   - jsonExactProps:    full dotted property names, O(1) exact match
//   - jsonWildcardProps: '*'-prefixed entries, stored as (suffix, value)
// Both are populated once in readConfig and only read after the hook is set
// (never mutated/cleared while hooked) -> lock-free in modify_callback,
// otherwise the dotted-property spoof would race GMS worker threads (A2).
static std::unordered_map<std::string, std::string> jsonExactProps;
static std::vector<std::pair<std::string, std::string>> jsonWildcardProps;

typedef void (*T_Callback)(void *, const char *, const char *, uint32_t);

// __system_property_read_callback invokes its callback synchronously on the
// calling thread, so the original callback reaches modify_callback through a
// thread-local instead of a shared map+mutex. This removes lock contention on
// every property read and the unbounded cookie-map growth (A1). Saved/restored
// around the orig() call to stay correct under reentrant reads.
static thread_local T_Callback tls_orig_callback = nullptr;

static void modify_callback(void *cookie, const char *name, const char *value, uint32_t serial)
{
    if (cookie == nullptr || name == nullptr || value == nullptr)
        return;

    T_Callback orig = tls_orig_callback;
    if (orig == nullptr)
        return;

    if (!jsonExactProps.empty() || !jsonWildcardProps.empty())
    {
        const char *oldValue = value;
        bool modified = false;

        if (!jsonExactProps.empty())
        {
            // Reused thread-local buffer -> no per-read heap allocation in steady state.
            static thread_local std::string key;
            key.assign(name);
            auto it = jsonExactProps.find(key);
            if (it != jsonExactProps.end())
            {
                value = it->second.c_str();
                modified = true;
            }
        }
        if (!modified)
        {
            std::string_view prop(name);
            for (const auto &w : jsonWildcardProps)
            {
                const std::string &suffix = w.first;
                if (prop.size() >= suffix.size() &&
                    prop.compare(prop.size() - suffix.size(), suffix.size(), suffix) == 0)
                {
                    value = w.second.c_str();
                    modified = true;
                    break;
                }
            }
        }

        if (modified)
        {
            LOGD("prop [%s]: %s -> %s", name, oldValue, value);
        }
        else if (g_verboseLogs > 2)
        {
            LOGD("prop [%s]: %s (unchanged)", name, oldValue);
        }
    }
    orig(cookie, name, value, serial);
}

static void (*o_system_property_read_callback)(const prop_info *, T_Callback, void *);

static void my_system_property_read_callback(const prop_info *pi, T_Callback callback, void *cookie)
{
    if (pi == nullptr || callback == nullptr || cookie == nullptr)
    {
        if (o_system_property_read_callback)
            o_system_property_read_callback(pi, callback, cookie);
        return;
    }
    T_Callback prev = tls_orig_callback;
    tls_orig_callback = callback;
    if (o_system_property_read_callback)
        o_system_property_read_callback(pi, modify_callback, cookie);
    tls_orig_callback = prev;
}

static void doHook()
{
    // PIFork v18 switched from Dobby to ShadowHook: GMS began detecting Dobby,
    // which broke <A13 Play Integrity, Wallet and RCS. ShadowHook resolves the
    // target itself; dlsym gives us the address to hook (lighter than an ELF
    // symbol scan, and no Dobby symbols in the dynamic table).
    shadowhook_init(SHADOWHOOK_MODE_UNIQUE, false);

    void *sym_addr = dlsym(RTLD_DEFAULT, "__system_property_read_callback");
    if (sym_addr == nullptr)
    {
        LOGE("hook failed: __system_property_read_callback not found via dlsym");
        return;
    }
    void *handle = shadowhook_hook_sym_addr(
        sym_addr,
        reinterpret_cast<void *>(my_system_property_read_callback),
        reinterpret_cast<void **>(&o_system_property_read_callback));
    if (handle == nullptr)
    {
        LOGE("hook failed: shadowhook errno=%d", shadowhook_get_errno());
        return;
    }
    LOGI("hooked __system_property_read_callback at %p", sym_addr);
}

// Android 17: ART blocks writing a static final field through plain reflection
// even after the hidden-API lift, so Build/Build.VERSION spoofing silently
// stopped applying. PIFork v18's fix (ported here): a JNI setter that bypasses
// the Java-language final check. The matching Java side (BuildFields) first
// clears the field's FINAL access flag, then calls this. Registered on the
// BuildFields class in inject().
static void setFieldNative(JNIEnv *env, jclass, jobject targetClass, jobject fieldObj, jstring typeObj, jobject valueObj)
{
    if (!targetClass || !fieldObj || !typeObj)
        return;
    jfieldID fieldID = env->FromReflectedField(fieldObj);
    if (!fieldID)
        return;
    const char *typeName = env->GetStringUTFChars(typeObj, nullptr);
    if (!typeName)
        return;
    jclass cls = static_cast<jclass>(targetClass);

    if (strcmp(typeName, "java.lang.String") == 0)
    {
        env->SetStaticObjectField(cls, fieldID, valueObj);
    }
    else if (strcmp(typeName, "int") == 0 || strcmp(typeName, "java.lang.Integer") == 0)
    {
        jclass box = env->FindClass("java/lang/Integer");
        jmethodID m = env->GetMethodID(box, "intValue", "()I");
        env->SetStaticIntField(cls, fieldID, env->CallIntMethod(valueObj, m));
        env->DeleteLocalRef(box);
    }
    else if (strcmp(typeName, "long") == 0 || strcmp(typeName, "java.lang.Long") == 0)
    {
        jclass box = env->FindClass("java/lang/Long");
        jmethodID m = env->GetMethodID(box, "longValue", "()J");
        env->SetStaticLongField(cls, fieldID, env->CallLongMethod(valueObj, m));
        env->DeleteLocalRef(box);
    }
    else if (strcmp(typeName, "boolean") == 0 || strcmp(typeName, "java.lang.Boolean") == 0)
    {
        jclass box = env->FindClass("java/lang/Boolean");
        jmethodID m = env->GetMethodID(box, "booleanValue", "()Z");
        env->SetStaticBooleanField(cls, fieldID, env->CallBooleanMethod(valueObj, m));
        env->DeleteLocalRef(box);
    }

    env->ReleaseStringUTFChars(typeObj, typeName);
}

// ---- I/O helpers: full read/write with EINTR retry (A3) -----------------------

static bool readFull(int fd, void *buf, size_t n)
{
    auto *p = static_cast<char *>(buf);
    size_t total = 0;
    while (total < n)
    {
        ssize_t r = read(fd, p + total, n - total);
        if (r < 0)
        {
            if (errno == EINTR)
                continue;
            return false;
        }
        if (r == 0)
            return false; // EOF before n bytes were read
        total += static_cast<size_t>(r);
    }
    return true;
}

static bool writeFull(int fd, const void *buf, size_t n)
{
    auto *p = static_cast<const char *>(buf);
    size_t total = 0;
    while (total < n)
    {
        ssize_t w = write(fd, p + total, n - total);
        if (w < 0)
        {
            if (errno == EINTR)
                continue;
            return false;
        }
        total += static_cast<size_t>(w);
    }
    return true;
}

static bool readFileToVector(const char *path, std::vector<char> &out)
{
    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0)
        return false;
    struct stat st;
    if (fstat(fd, &st) < 0 || st.st_size <= 0)
    {
        close(fd);
        return false;
    }
    out.resize(static_cast<size_t>(st.st_size));
    bool ok = readFull(fd, out.data(), out.size());
    close(fd);
    if (!ok)
        out.clear();
    return ok;
}

static inline std::string trim(std::string_view s)
{
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string_view::npos)
        return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return std::string(s.substr(start, end - start + 1));
}

static int safeParseInt(const std::string &value, int defaultValue)
{
    if (value.empty())
        return defaultValue;
    size_t pos = 0;
    bool negative = false;
    if (value[pos] == '-' || value[pos] == '+')
    {
        negative = (value[pos] == '-');
        pos++;
    }
    if (pos >= value.length())
        return defaultValue;
    long long result = 0; // 64-bit on every ABI, including 32-bit ones
    for (; pos < value.length(); pos++)
    {
        if (value[pos] < '0' || value[pos] > '9')
            return defaultValue;
        result = result * 10 + (value[pos] - '0');
        if (result > static_cast<long long>(INT_MAX) + 1)
        {
            return negative ? INT_MIN : INT_MAX;
        }
    }
    long long signed_result = negative ? -result : result;
    if (signed_result > INT_MAX)
        return INT_MAX;
    if (signed_result < INT_MIN)
        return INT_MIN;
    return static_cast<int>(signed_result);
}

static void parseProps(const std::vector<char> &data, std::unordered_map<std::string, std::string> &props)
{
    if (data.empty())
        return;
    std::string_view content(data.data(), data.size());
    size_t pos = 0;
    while (pos < content.size())
    {
        size_t end = content.find('\n', pos);
        if (end == std::string_view::npos)
            end = content.size();
        std::string_view line = content.substr(pos, end - pos);
        pos = end + 1;
        if (!line.empty() && line.back() == '\r')
            line.remove_suffix(1);
        if (line.empty() || line[0] == '#')
            continue;
        size_t eq = line.find('=');
        if (eq == std::string_view::npos || eq == 0)
            continue;
        std::string name = trim(line.substr(0, eq));
        std::string value = trim(line.substr(eq + 1));
        size_t hash = value.find('#');
        if (hash != std::string::npos)
            value = trim(std::string_view(value).substr(0, hash));
        if (!name.empty())
            props[name] = std::move(value);
    }
    LOGD("parsed %zu properties from config", props.size());
}

// Package list: one entry per line, '#' starts a comment. Reuses trim.
static void parseAppsList(const std::vector<char> &data, std::unordered_set<std::string> &apps)
{
    if (data.empty())
        return;
    std::string_view content(data.data(), data.size());
    size_t pos = 0;
    while (pos < content.size())
    {
        size_t end = content.find('\n', pos);
        if (end == std::string_view::npos)
            end = content.size();
        std::string_view line = content.substr(pos, end - pos);
        pos = end + 1;
        if (!line.empty() && line.back() == '\r')
            line.remove_suffix(1);
        if (line.empty() || line[0] == '#')
            continue;
        std::string pkg = trim(line);
        if (!pkg.empty())
            apps.insert(std::move(pkg));
    }
    LOGD("parsed %zu list entries", apps.size());
}

class PlayIntegrityFix : public zygisk::ModuleBase
{
public:
    void onLoad(zygisk::Api *api, JNIEnv *env) override
    {
        this->api = api;
        this->env = env;
    }

    void preAppSpecialize(zygisk::AppSpecializeArgs *args) override
    {
        if (!args || !args->nice_name || !args->app_data_dir)
        {
            api->setOption(zygisk::DLCLOSE_MODULE_LIBRARY);
            return;
        }
        const char *rawProcess = env->GetStringUTFChars(args->nice_name, nullptr);
        const char *rawDir = env->GetStringUTFChars(args->app_data_dir, nullptr);
        if (!rawProcess || !rawDir)
        {
            if (rawProcess)
                env->ReleaseStringUTFChars(args->nice_name, rawProcess);
            if (rawDir)
                env->ReleaseStringUTFChars(args->app_data_dir, rawDir);
            api->setOption(zygisk::DLCLOSE_MODULE_LIBRARY);
            return;
        }
        pkgName = rawProcess;
        std::string_view dir(rawDir);
        bool isGms = dir.ends_with("/com.google.android.gms") || dir.ends_with("/com.android.vending");
        bool isTargetProcess = (pkgName == DROIDGUARD_PACKAGE) || (pkgName == VENDING_PACKAGE);
        env->ReleaseStringUTFChars(args->nice_name, rawProcess);
        env->ReleaseStringUTFChars(args->app_data_dir, rawDir);

        // Per-app spoofing gate. spfapps.list lives under /data/adb (root-only),
        // so a non-GMS app process cannot read it here — membership is resolved by
        // the companion. To avoid a companion round-trip on every app launch when
        // the feature is off, first check a cheap, app-readable system property
        // that is set to "1" at boot only while spoofApps is enabled.
        bool isSpoofCandidate = false;
        if (!isGms && !isTargetProcess)
        {
            char propVal[PROP_VALUE_MAX] = {0};
            if (__system_property_get(SPOOFAPPS_PROP, propVal) > 0 && propVal[0] == '1')
            {
                isSpoofCandidate = true;
            }
        }

        if (!isGms && !isSpoofCandidate)
        {
            api->setOption(zygisk::DLCLOSE_MODULE_LIBRARY);
            return;
        }
        // Unmount module overlays from every GMS sub-process (stealth), even those
        // we won't inject into.
        if (isGms || isTargetProcess)
            api->setOption(zygisk::FORCE_DENYLIST_UNMOUNT);
        if (isGms && !isTargetProcess)
        {
            api->setOption(zygisk::DLCLOSE_MODULE_LIBRARY);
            return;
        }

        // Companion protocol (companion runs as root and can read /data/adb):
        //   ->  uint32 pkgName length, then pkgName bytes
        //   <-  1 byte role: 0 = none, 1 = GMS target, 2 = per-app spoof target
        //   <-  if role != 0: long dexSize, long configSize, dex bytes, config bytes
        int fd = api->connectCompanion();
        if (fd < 0)
        {
            LOGE("companion connect failed");
            api->setOption(zygisk::DLCLOSE_MODULE_LIBRARY);
            return;
        }
        uint32_t nameLen = static_cast<uint32_t>(pkgName.size());
        char role = 0;
        if (!writeFull(fd, &nameLen, sizeof(nameLen)) ||
            (nameLen > 0 && !writeFull(fd, pkgName.data(), nameLen)) ||
            !readFull(fd, &role, 1))
        {
            close(fd);
            LOGE("companion handshake failed");
            api->setOption(zygisk::DLCLOSE_MODULE_LIBRARY);
            return;
        }
        if (role == 0)
        {
            close(fd);
            api->setOption(zygisk::DLCLOSE_MODULE_LIBRARY);
            return;
        }
        procIsSpoofApp = (role == 2);
        if (procIsSpoofApp)
            api->setOption(zygisk::FORCE_DENYLIST_UNMOUNT);
        LOGI("preAppSpecialize pkg=%s role=%d", pkgName.c_str(), static_cast<int>(role));

        long dexSize = 0, configSize = 0;
        if (!readFull(fd, &dexSize, sizeof(long)) || !readFull(fd, &configSize, sizeof(long)))
        {
            LOGE("companion read sizes failed");
            close(fd);
            api->setOption(zygisk::DLCLOSE_MODULE_LIBRARY);
            return;
        }
        if (dexSize < 1)
        {
            close(fd);
            LOGE("invalid dex size %ld", dexSize);
            api->setOption(zygisk::DLCLOSE_MODULE_LIBRARY);
            return;
        }
        LOGI("companion dex=%ld config=%ld", dexSize, configSize);

        dexVector.resize(static_cast<size_t>(dexSize));
        if (!readFull(fd, dexVector.data(), dexVector.size()))
        {
            close(fd);
            LOGE("companion read dex failed");
            dexVector.clear();
            api->setOption(zygisk::DLCLOSE_MODULE_LIBRARY);
            return;
        }
        if (configSize > 0)
        {
            std::vector<char> configVector(static_cast<size_t>(configSize));
            if (!readFull(fd, configVector.data(), configVector.size()))
            {
                close(fd);
                LOGE("companion read config failed");
                dexVector.clear();
                api->setOption(zygisk::DLCLOSE_MODULE_LIBRARY);
                return;
            }
            close(fd);
            parseProps(configVector, propMap);
            // Learn verboseLogs as early as possible so post-phase logs work.
            auto vit = propMap.find("verboseLogs");
            if (vit != propMap.end())
                g_verboseLogs = safeParseInt(vit->second, 0);
        }
        else
        {
            close(fd);
            LOGE("empty config");
        }
    }

    void postAppSpecialize(const zygisk::AppSpecializeArgs *args) override
    {
        if (dexVector.empty() || propMap.empty())
            return;
        readConfig();

        bool isVending = (pkgName == VENDING_PACKAGE);
        bool isDroidGuard = (pkgName == DROIDGUARD_PACKAGE);
        bool isPerApp = procIsSpoofApp;
        if (isVending)
        {
            spoofBuild = spoofProps = spoofProvider = spoofSignature = 0;
        }
        else if (isDroidGuard)
        {
            spoofVendingFinger = spoofVendingSdk = spoofPixel = 0;
        }
        else if (isPerApp)
        {
            // An arbitrary app only needs the build/props/pixel profile;
            // attestation/vending specifics are not applied.
            spoofProvider = spoofSignature = spoofVendingFinger = spoofVendingSdk = 0;
        }

        bool hooked = false;
        if (spoofProps > 0 && (isDroidGuard || isPerApp))
        {
            doHook();
            hooked = true;
        }
        int totalSpoofs = spoofBuild + spoofProvider + spoofSignature + spoofVendingFinger + spoofVendingSdk + spoofPixel;
        if (totalSpoofs > 0 || isPerApp)
        {
            inject();
        }

        // dexVector and propMap are no longer needed by the hook. jsonExact/
        // WildcardProps must stay alive while the hook is installed (A2),
        // otherwise clear them.
        dexVector.clear();
        dexVector.shrink_to_fit();
        propMap.clear();
        if (!hooked)
        {
            jsonExactProps.clear();
            jsonWildcardProps.clear();
        }
    }

    void preServerSpecialize(zygisk::ServerSpecializeArgs *args) override
    {
        api->setOption(zygisk::DLCLOSE_MODULE_LIBRARY);
    }

private:
    zygisk::Api *api = nullptr;
    JNIEnv *env = nullptr;
    std::vector<char> dexVector;
    std::string pkgName;
    bool procIsSpoofApp = false;

    std::string getPropValue(const std::string &key, const std::string &defaultValue = "")
    {
        auto it = propMap.find(key);
        return (it != propMap.end() && !it->second.empty()) ? it->second : defaultValue;
    }

    int getPropInt(const std::string &key, int defaultValue = 0)
    {
        return safeParseInt(getPropValue(key), defaultValue);
    }

    void readConfig()
    {
        LOGI("config has %zu keys", propMap.size());

        g_verboseLogs = getPropInt("verboseLogs");
        spoofVendingSdk = getPropInt("spoofVendingSdk");

        std::string spoofVendingFingerValue = getPropValue("spoofVendingFinger");
        if (!spoofVendingFingerValue.empty())
        {
            if (spoofVendingFingerValue.find_first_not_of("01") != std::string::npos)
            {
                spoofVendingFinger = 1;
                vendingFingerprintValue = spoofVendingFingerValue;
            }
            else
            {
                spoofVendingFinger = safeParseInt(spoofVendingFingerValue, 0);
                if (spoofVendingFinger > 0)
                {
                    vendingFingerprintValue = getPropValue("FINGERPRINT");
                }
            }
        }

        spoofPixel = getPropInt("spoofPixel");
        if (spoofPixel > 0)
        {
            pixelManufacturer = getPropValue("MANUFACTURER");
            pixelModel = getPropValue("MODEL");
            pixelDevice = getPropValue("DEVICE");
            pixelBrand = getPropValue("BRAND");
        }

        if (pkgName == VENDING_PACKAGE)
        {
            propMap.clear();
            return;
        }

        spoofBuild = getPropInt("spoofBuild", 1);
        spoofProps = getPropInt("spoofProps", 1);
        spoofProvider = getPropInt("spoofProvider", 1);
        spoofSignature = getPropInt("spoofSignature");

        // Wildcard / dotted properties go to the native hook, the rest to Java.
        for (const auto &pair : propMap)
        {
            const std::string &k = pair.first;
            if (pair.second.empty() || k.find_first_of("*.") == std::string::npos)
                continue;
            if (k[0] == '*')
            {
                jsonWildcardProps.emplace_back(k.substr(1), pair.second);
            }
            else
            {
                jsonExactProps[k] = pair.second;
            }
        }
    }

    std::string buildJsonString()
    {
        std::string jsonStr = "{";
        bool first = true;
        for (const auto &pair : propMap)
        {
            if (pair.first.find_first_of("*.") != std::string::npos)
                continue;
            if (!first)
                jsonStr += ",";
            first = false;
            std::string escaped;
            escaped.reserve(pair.second.length() + 8);
            static const char hex[] = "0123456789abcdef";
            for (char c : pair.second)
            {
                switch (c)
                {
                case '\\':
                    escaped += "\\\\";
                    break;
                case '"':
                    escaped += "\\\"";
                    break;
                case '\n':
                    escaped += "\\n";
                    break;
                case '\r':
                    escaped += "\\r";
                    break;
                case '\t':
                    escaped += "\\t";
                    break;
                case '\b':
                    escaped += "\\b";
                    break;
                case '\f':
                    escaped += "\\f";
                    break;
                default:
                {
                    unsigned char uc = static_cast<unsigned char>(c);
                    if (uc < 0x20)
                    {
                        // Remaining control chars must be \u00XX in JSON.
                        escaped += "\\u00";
                        escaped += hex[(uc >> 4) & 0xF];
                        escaped += hex[uc & 0xF];
                    }
                    else
                    {
                        escaped += c; // includes valid UTF-8 continuation bytes
                    }
                }
                }
            }
            jsonStr += "\"" + pair.first + "\":\"" + escaped + "\"";
        }
        jsonStr += "}";
        return jsonStr;
    }

    // Clears a pending JNI exception (if any) so it can't corrupt later JNI
    // calls or the host process; returns true when one was present.
    bool checkException(const char *ctx)
    {
        if (env->ExceptionCheck())
        {
            env->ExceptionClear();
            LOGE("JNI exception during %s", ctx);
            return true;
        }
        return false;
    }

    void inject()
    {
        bool isVending = (pkgName == VENDING_PACKAGE);
        const char *niceName = isVending ? "PS" : (pkgName == DROIDGUARD_PACKAGE ? "DG" : "APP");

        LOGI("jni %s: get system classloader", niceName);
        jclass clClass = env->FindClass("java/lang/ClassLoader");
        if (!clClass)
        {
            LOGE("ClassLoader not found");
            return;
        }
        jmethodID getSystemClassLoader = env->GetStaticMethodID(clClass, "getSystemClassLoader", "()Ljava/lang/ClassLoader;");
        jobject systemClassLoader = env->CallStaticObjectMethod(clClass, getSystemClassLoader);

        LOGI("jni %s: create dex classloader", niceName);
        jclass dexClClass = env->FindClass("dalvik/system/InMemoryDexClassLoader");
        if (!dexClClass)
        {
            LOGE("InMemoryDexClassLoader not found");
            env->DeleteLocalRef(clClass);
            if (systemClassLoader)
                env->DeleteLocalRef(systemClassLoader);
            return;
        }
        jmethodID dexClInit = env->GetMethodID(dexClClass, "<init>", "(Ljava/nio/ByteBuffer;Ljava/lang/ClassLoader;)V");
        jobject buffer = env->NewDirectByteBuffer(dexVector.data(), static_cast<jlong>(dexVector.size()));
        jobject dexCl = env->NewObject(dexClClass, dexClInit, buffer, systemClassLoader);
        if (checkException("dex classloader") || !dexCl)
        {
            cleanupRefs(clClass, systemClassLoader, dexClClass, buffer, dexCl, nullptr, nullptr);
            return;
        }

        LOGI("jni %s: load entry class", niceName);
        jmethodID loadClass = env->GetMethodID(clClass, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
        const char *className = isVending ? "es.chiteroman.playintegrityfix.EntryPointVending" : "es.chiteroman.playintegrityfix.EntryPoint";
        jstring entryClassName = env->NewStringUTF(className);
        jobject entryClassObj = env->CallObjectMethod(dexCl, loadClass, entryClassName);
        if (checkException("loadClass") || !entryClassObj)
        {
            LOGE("entry class not found");
            cleanupRefs(clClass, systemClassLoader, dexClClass, buffer, dexCl, entryClassName, entryClassObj);
            return;
        }
        jclass entryClass = static_cast<jclass>(entryClassObj);

        // Bind the Android-17 JNI field setter on BuildFields (the single
        // reflective writer both entry paths use) before init() writes any
        // Build field. Non-fatal if it fails: BuildFields falls back to plain
        // reflection, which still works pre-17.
        {
            jstring bfName = env->NewStringUTF("es.chiteroman.playintegrityfix.BuildFields");
            jobject bfClassObj = env->CallObjectMethod(dexCl, loadClass, bfName);
            if (!checkException("loadClass BuildFields") && bfClassObj)
            {
                static const JNINativeMethod bfMethods[] = {
                    {"setFieldNative",
                     "(Ljava/lang/Class;Ljava/lang/reflect/Field;Ljava/lang/String;Ljava/lang/Object;)V",
                     reinterpret_cast<void *>(setFieldNative)}};
                if (env->RegisterNatives(static_cast<jclass>(bfClassObj), bfMethods, 1) != 0)
                    checkException("RegisterNatives BuildFields");
                env->DeleteLocalRef(bfClassObj);
            }
            env->DeleteLocalRef(bfName);
        }

        if (isVending)
        {
            LOGI("jni %s: call EntryPointVending.init", niceName);
            jmethodID entryInit = env->GetStaticMethodID(entryClass, "init", "(IIILjava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
            jstring javaFinger = env->NewStringUTF(vendingFingerprintValue.c_str());
            jstring javaManuf = env->NewStringUTF(pixelManufacturer.c_str());
            jstring javaModel = env->NewStringUTF(pixelModel.c_str());
            jstring javaDevice = env->NewStringUTF(pixelDevice.c_str());
            jstring javaBrand = env->NewStringUTF(pixelBrand.c_str());
            env->CallStaticVoidMethod(entryClass, entryInit, g_verboseLogs, spoofVendingFinger, spoofVendingSdk, javaFinger, javaManuf, javaModel, javaDevice, javaBrand);
            env->DeleteLocalRef(javaFinger);
            env->DeleteLocalRef(javaManuf);
            env->DeleteLocalRef(javaModel);
            env->DeleteLocalRef(javaDevice);
            env->DeleteLocalRef(javaBrand);
            checkException("EntryPointVending.init");
        }
        else
        {
            LOGI("jni %s: send json", niceName);
            jmethodID receiveJson = env->GetStaticMethodID(entryClass, "receiveJson", "(Ljava/lang/String;)V");
            std::string jsonStr = buildJsonString();
            jstring javaStr = env->NewStringUTF(jsonStr.c_str());
            env->CallStaticVoidMethod(entryClass, receiveJson, javaStr);
            env->DeleteLocalRef(javaStr);
            if (!checkException("receiveJson"))
            {
                LOGI("jni %s: call EntryPoint.init", niceName);
                jmethodID entryInit = env->GetStaticMethodID(entryClass, "init", "(IIII)V");
                env->CallStaticVoidMethod(entryClass, entryInit, g_verboseLogs, spoofBuild, spoofProvider, spoofSignature);
                checkException("EntryPoint.init");
            }
        }
        cleanupRefs(clClass, systemClassLoader, dexClClass, buffer, dexCl, entryClassName, entryClassObj);
    }

    void cleanupRefs(jobject clClass, jobject systemClassLoader, jobject dexClClass, jobject buffer,
                     jobject dexCl, jobject entryClassName, jobject entryClassObj)
    {
        if (clClass)
            env->DeleteLocalRef(clClass);
        if (systemClassLoader)
            env->DeleteLocalRef(systemClassLoader);
        if (dexClClass)
            env->DeleteLocalRef(dexClClass);
        if (buffer)
            env->DeleteLocalRef(buffer);
        if (dexCl)
            env->DeleteLocalRef(dexCl);
        if (entryClassName)
            env->DeleteLocalRef(entryClassName);
        if (entryClassObj)
            env->DeleteLocalRef(entryClassObj);
    }
};

static void companion(int fd)
{
    // Receive the requesting package name.
    uint32_t nameLen = 0;
    if (!readFull(fd, &nameLen, sizeof(nameLen)))
        return;
    if (nameLen == 0 || nameLen > 1024)
        return; // sane bound on a package name
    std::string pkg(nameLen, '\0');
    if (!readFull(fd, &pkg[0], nameLen))
        return;

    // Decide the role from the privileged side, where /data/adb is readable.
    char role = 0; // 0 = none, 1 = GMS target, 2 = per-app spoof target
    if (pkg == DROIDGUARD_PACKAGE || pkg == VENDING_PACKAGE)
    {
        role = 1;
    }
    else
    {
        // Per-app: confirm the feature is enabled and the package is listed.
        std::vector<char> cfgData;
        if (readFileToVector(CUSTOM_PROP_FILE_PATH, cfgData))
        {
            std::unordered_map<std::string, std::string> cfg;
            parseProps(cfgData, cfg);
            auto sit = cfg.find("spoofApps");
            if (sit != cfg.end() && safeParseInt(sit->second, 0) > 0)
            {
                std::vector<char> appsData;
                if (readFileToVector(APPS_LIST_FILE_PATH, appsData))
                {
                    std::unordered_set<std::string> apps;
                    parseAppsList(appsData, apps);
                    if (apps.count(pkg) > 0)
                        role = 2;
                }
            }
        }
    }

    if (!writeFull(fd, &role, 1))
        return;
    if (role == 0)
        return; // nothing to inject

    std::vector<char> dexVector, configVector;
    readFileToVector(DEX_FILE_PATH, dexVector);
    readFileToVector(CUSTOM_PROP_FILE_PATH, configVector);

    long dexSize = static_cast<long>(dexVector.size());
    long configSize = static_cast<long>(configVector.size());

    if (!writeFull(fd, &dexSize, sizeof(long)) || !writeFull(fd, &configSize, sizeof(long)))
        return;
    if (dexSize > 0 && !writeFull(fd, dexVector.data(), dexVector.size()))
        return;
    if (configSize > 0)
        writeFull(fd, configVector.data(), configVector.size());
}

REGISTER_ZYGISK_MODULE(PlayIntegrityFix)

REGISTER_ZYGISK_COMPANION(companion)
