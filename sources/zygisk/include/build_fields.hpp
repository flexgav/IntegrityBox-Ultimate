#pragma once
#include "runtime_config.hpp"
#include <jni.h>

namespace build_fields {
struct Result { size_t applied = 0, skipped = 0, failed = 0; };
// Direct JNI writes preserve the existing flags without loading a dex helper.
Result apply(JNIEnv *env, const runtime::Properties &fields);
} // namespace build_fields
