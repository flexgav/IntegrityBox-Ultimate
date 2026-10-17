#pragma once
#include "logging.hpp"
#include <jni.h>

namespace runtime_jni {
inline bool clearException(JNIEnv *env, const char *operation = nullptr) {
    if (!env->ExceptionCheck()) return false;
    env->ExceptionClear();
    if (operation) LOGE("JNI exception during %s", operation);
    return true;
}
}
