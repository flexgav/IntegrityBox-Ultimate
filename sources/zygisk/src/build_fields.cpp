#include "build_fields.hpp"
#include "logging.hpp"
#include "jni_utils.hpp"

#include <charconv>

namespace build_fields {
namespace {
using runtime_jni::clearException;
template<typename T> bool parse(const std::string &value, T &out) {
    const char *start = value.data();
    if (!value.empty() && value.front() == '+') ++start;
    const auto result = std::from_chars(start, value.data() + value.size(), out);
    return result.ec == std::errc() && result.ptr == value.data() + value.size();
}

// Profiles contain UTF-8, whereas NewStringUTF expects modified UTF-8. Convert
// explicitly so supplementary characters never reach JNI in an invalid encoding.
jstring javaString(JNIEnv *env, const std::string &value) {
    std::vector<jchar> units;
    for (size_t i = 0; i < value.size();) {
        uint32_t code = static_cast<unsigned char>(value[i++]);
        unsigned continuation = 0;
        if (code >= 0xC2 && code <= 0xDF) { code &= 0x1F; continuation = 1; }
        else if (code >= 0xE0 && code <= 0xEF) { code &= 0x0F; continuation = 2; }
        else if (code >= 0xF0 && code <= 0xF4) { code &= 7; continuation = 3; }
        else if (code == 0 || code >= 0x80) return nullptr;
        if (i + continuation > value.size()) return nullptr;
        for (unsigned part = 0; part < continuation; ++part) {
            const auto byte = static_cast<unsigned char>(value[i++]);
            if ((byte & 0xC0) != 0x80) return nullptr;
            code = (code << 6) | (byte & 0x3F);
        }
        if ((continuation == 1 && code < 0x80) || (continuation == 2 && code < 0x800) ||
            (continuation == 3 && code < 0x10000) || code > 0x10FFFF || (code >= 0xD800 && code <= 0xDFFF)) return nullptr;
        if (code >= 0x10000) {
            code -= 0x10000;
            units.push_back(static_cast<jchar>(0xD800 + (code >> 10)));
            units.push_back(static_cast<jchar>(0xDC00 + (code & 0x3FF)));
        } else units.push_back(static_cast<jchar>(code));
    }
    return env->NewString(units.data(), static_cast<jsize>(units.size()));
}
} // namespace

Result apply(JNIEnv *env, const runtime::Properties &fields) {
    Result result;
    if (fields.empty()) return result;
    jclass owners[] = {env->FindClass("android/os/Build"), nullptr};
    if (clearException(env) || !owners[0]) { result.failed = fields.size(); return result; }
    owners[1] = env->FindClass("android/os/Build$VERSION");
    if (clearException(env) || !owners[1]) { env->DeleteLocalRef(owners[0]); result.failed = fields.size(); return result; }
    for (const auto &[name, value] : fields) {
        if (value.empty()) { ++result.skipped; continue; }
        bool found = false, written = false;
        for (jclass owner : owners) {
            for (const char *type : {"Ljava/lang/String;", "I", "J", "Z"}) {
                const jfieldID field = env->GetStaticFieldID(owner, name.c_str(), type);
                const bool lookupFailed = clearException(env);
                if (lookupFailed || !field) continue;
                found = true;
                if (type[0] == 'L') {
                    jstring text = javaString(env, value);
                    if (!clearException(env) && text) {
                        env->SetStaticObjectField(owner, field, text);
                        written = !clearException(env);
                        env->DeleteLocalRef(text);
                    }
                } else if (type[0] == 'I') {
                    jint number;
                    if (parse(value, number)) { env->SetStaticIntField(owner, field, number); written = !clearException(env); }
                } else if (type[0] == 'J') {
                    jlong number;
                    if (parse(value, number)) { env->SetStaticLongField(owner, field, number); written = !clearException(env); }
                } else if (value == "true" || value == "false" || value == "1" || value == "0") {
                    env->SetStaticBooleanField(owner, field, value == "true" || value == "1");
                    written = !clearException(env);
                }
                break;
            }
            if (found) break;
        }
        if (written) { ++result.applied; LOGD("Build field %s applied", name.c_str()); }
        else if (found) { ++result.failed; LOGE("Build field %s rejected", name.c_str()); }
        else ++result.skipped;
    }
    for (jclass owner : owners) env->DeleteLocalRef(owner);
    return result;
}
} // namespace build_fields
