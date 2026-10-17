#pragma once
#include "runtime_config.hpp"
#include <jni.h>

namespace dex_payload {
// The caller owns and retains the byte buffer for the loader's lifetime.
bool inject(JNIEnv *env, const runtime::Config &config, const std::vector<char> &dex);
}
