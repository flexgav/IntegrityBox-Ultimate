#include "dex_payload.hpp"
#include "jni_utils.hpp"

namespace dex_payload {
using runtime_jni::clearException;
// Only Provider/Signature use the legacy-compatible dex entry point. Native
// Build writes already ran; build=0 also disables CustomProvider lazy re-spoofing.
bool inject(JNIEnv *env, const runtime::Config &config, const std::vector<char> &dex) {
    if (dex.empty() || env->PushLocalFrame(16) != JNI_OK) {
        clearException(env, "dex local frame");
        return false;
    }
    bool applied = false;
    do {
        jclass loaderClass = env->FindClass("java/lang/ClassLoader");
        if (clearException(env, "ClassLoader") || !loaderClass) break;
        jmethodID systemMethod = env->GetStaticMethodID(loaderClass, "getSystemClassLoader", "()Ljava/lang/ClassLoader;");
        if (clearException(env, "system loader method") || !systemMethod) break;
        jobject systemLoader = env->CallStaticObjectMethod(loaderClass, systemMethod);
        if (clearException(env, "system loader") || !systemLoader) break;
        jclass dexClass = env->FindClass("dalvik/system/InMemoryDexClassLoader");
        if (clearException(env, "InMemoryDexClassLoader") || !dexClass) break;
        jmethodID constructor = env->GetMethodID(dexClass, "<init>", "(Ljava/nio/ByteBuffer;Ljava/lang/ClassLoader;)V");
        if (clearException(env, "dex constructor") || !constructor) break;
        jobject buffer = env->NewDirectByteBuffer(const_cast<char *>(dex.data()), static_cast<jlong>(dex.size()));
        if (clearException(env, "dex buffer") || !buffer) break;
        jobject loader = env->NewObject(dexClass, constructor, buffer, systemLoader);
        if (clearException(env, "dex loader") || !loader) break;
        jmethodID load = env->GetMethodID(loaderClass, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
        if (clearException(env, "loadClass method") || !load) break;
        jstring name = env->NewStringUTF("es.chiteroman.playintegrityfix.EntryPoint");
        if (clearException(env, "entry name") || !name) break;
        jclass entry = static_cast<jclass>(env->CallObjectMethod(loader, load, name));
        if (clearException(env, "entry class") || !entry) break;
        jmethodID init = env->GetStaticMethodID(entry, "initAttestation", "(III)I");
        if (!clearException(env) && init) {
            const int expected = (config.provider ? 1 : 0) | (config.signature ? 2 : 0);
            const int completed = env->CallStaticIntMethod(entry, init, config.verbose, config.provider ? 1 : 0, config.signature ? 1 : 0);
            applied = !clearException(env, "attestation init") && completed == expected;
            if (!applied) LOGE("attestation operations requested=%d completed=%d", expected, completed);
        } else {
            // Retain the entry contract for an older dex during module updates.
            init = env->GetStaticMethodID(entry, "init", "(IIII)V");
            if (clearException(env, "legacy entry init method") || !init) break;
            env->CallStaticVoidMethod(entry, init, config.verbose, 0, config.provider ? 1 : 0, config.signature ? 1 : 0);
            applied = !clearException(env, "legacy attestation init");
        }
    } while (false);
    env->PopLocalFrame(nullptr);
    return applied;
}

} // namespace dex_payload
