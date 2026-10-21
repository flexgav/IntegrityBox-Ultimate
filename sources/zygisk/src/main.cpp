#include "zygisk.hpp"
#include "logging.hpp"
#include "runtime_config.hpp"
#include "companion_protocol.hpp"
#include "build_fields.hpp"
#include "dex_payload.hpp"
#include "jni_utils.hpp"
#include "property_virtualizer.hpp"
#include "app_compatibility.hpp"

#include <sys/system_properties.h>
#include <unistd.h>

namespace {
constexpr char kConfigPath[] = "/data/adb/modules/playintegrityfix/cust.spf.prop";
constexpr char kDexPath[] = "/data/adb/modules/playintegrityfix/classes.dex";
constexpr char kAppsPath[] = "/data/adb/modules/playintegrityfix/spfapps.list";
// Neutral boot-time gate; membership and the saved flag are rechecked by root.
constexpr char kAppsGate[] = "sys.spfapps.cfg";

std::string_view view(const std::vector<char> &bytes) { return {bytes.data(), bytes.size()}; }

using runtime_jni::clearException;

class PlayIntegrityFix : public zygisk::ModuleBase {
public:
    void onLoad(zygisk::Api *api, JNIEnv *env) override { api_ = api; env_ = env; }

    void preAppSpecialize(zygisk::AppSpecializeArgs *args) override {
        ready_ = false;
        role_ = runtime::Role::None;
        config_ = {};
        dex_.clear();
        g_verboseLogs = 0;
        if (!args || !args->nice_name || !args->app_data_dir || args->uid < 0) { unload(); return; }
        // Spoof list entries are per Android user (work profile, app clones).
        const auto userId = static_cast<uint32_t>(args->uid) / runtime::kPerUserRange;
        const char *name = env_->GetStringUTFChars(args->nice_name, nullptr);
        if (clearException(env_, "process name") || !name) { unload(); return; }
        process_ = name;
        env_->ReleaseStringUTFChars(args->nice_name, name);
        const char *directory = env_->GetStringUTFChars(args->app_data_dir, nullptr);
        if (clearException(env_, "app directory") || !directory) { unload(); return; }
        const std::string_view path(directory);
        const bool gms = path.ends_with("/com.google.android.gms") || path.ends_with("/com.android.vending");
        env_->ReleaseStringUTFChars(args->app_data_dir, directory);
        if (!runtime::validProcess(process_)) { unload(); return; }
        if (gms) {
            api_->setOption(zygisk::FORCE_DENYLIST_UNMOUNT);
            if (process_ != runtime::kDroidGuard && process_ != runtime::kVending) { unload(); return; }
        } else {
            char enabled[PROP_VALUE_MAX] = {};
            if (__system_property_get(kAppsGate, enabled) <= 0 || enabled[0] != '1') { unload(); return; }
        }

        companion_protocol::Payload payload;
        {
            companion_protocol::Channel channel(api_->connectCompanion());
            if (!companion_protocol::sendRequest(channel, process_, userId) || !companion_protocol::receivePayload(channel, payload) ||
                payload.role == runtime::Role::None) { unload(); return; }
        }
        const auto expected = process_ == runtime::kDroidGuard ? runtime::Role::DroidGuard :
            process_ == runtime::kVending ? runtime::Role::Vending : runtime::Role::App;
        if (payload.role != expected || !runtime::parseConfig(view(payload.config), config_)) { unload(); return; }
        role_ = payload.role;
        g_verboseLogs = config_.verbose;
        if (role_ == runtime::Role::App) api_->setOption(zygisk::FORCE_DENYLIST_UNMOUNT);
        const auto compatibility = app_compatibility::inspectPairIP(role_);
        if (compatibility == app_compatibility::PairIPStatus::Detected) {
            LOGI("PairIP detected; per-app spoofing skipped process=%s", process_.c_str());
            unload();
            return;
        }
        if (compatibility == app_compatibility::PairIPStatus::Unavailable)
            LOGE("PairIP check unavailable; per-app spoofing retained process=%s", process_.c_str());
        if (config_.needsDex(role_)) dex_ = std::move(payload.dex);

        if (config_.props && (role_ == runtime::Role::DroidGuard || role_ == runtime::Role::App)) {
            const auto result = property_virtualizer::install(config_.exactProperties(), config_.suffixProperties());
            if (result.status == property_virtualizer::Status::RollbackFailed) _exit(1);
            if (result.status == property_virtualizer::Status::Applied || result.status == property_virtualizer::Status::NoMatches)
                LOGI("properties status=%s matched=%zu applied=%zu", property_virtualizer::statusName(result.status), result.matched, result.applied);
            else LOGE("properties status=%s matched=%zu applied=%zu", property_virtualizer::statusName(result.status), result.matched, result.applied);
        }
        ready_ = true;
    }

    void postAppSpecialize(const zygisk::AppSpecializeArgs *) override {
        if (!ready_) return;
        const auto result = build_fields::apply(env_, config_.buildFields(role_));
        LOGI("Build fields applied=%zu skipped=%zu failed=%zu", result.applied, result.skipped, result.failed);
        if (config_.needsDex(role_)) {
            if (!dex_payload::inject(env_, config_, dex_)) LOGE("Provider/Signature payload unavailable or failed; native operations retained");
        }
        // The dex loader may retain its direct buffer. Keep those bytes and the
        // library in that case; native-only children can unload fully.
        const bool keepLibrary = !dex_.empty() && config_.needsDex(role_);
        config_ = {};
        if (!keepLibrary) { dex_.clear(); unload(); }
    }

    void preServerSpecialize(zygisk::ServerSpecializeArgs *) override { unload(); }

private:
    void unload() { api_->setOption(zygisk::DLCLOSE_MODULE_LIBRARY); }
    zygisk::Api *api_ = nullptr;
    JNIEnv *env_ = nullptr;
    runtime::Config config_;
    runtime::Role role_ = runtime::Role::None;
    std::vector<char> dex_;
    std::string process_;
    bool ready_ = false;
};

void companion(int fd) {
    companion_protocol::Channel channel(fd);
    std::string process;
    uint32_t userId = 0;
    if (!companion_protocol::receiveRequest(channel, process, userId)) return;
    companion_protocol::Payload payload;
    if (!companion_protocol::preparePayload(process, userId, kConfigPath, kAppsPath, kDexPath, payload)) return;
    companion_protocol::sendPayload(channel, payload);
}
} // namespace

REGISTER_ZYGISK_MODULE(PlayIntegrityFix)
REGISTER_ZYGISK_COMPANION(companion)
