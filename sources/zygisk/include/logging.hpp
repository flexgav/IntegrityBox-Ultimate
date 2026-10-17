#pragma once

#include <android/log.h>

// Shared runtime gate: no logcat output with verboseLogs=0, including failures.
// Keep the tag neutral because string literals remain in stripped libraries.
inline int g_verboseLogs = 0;
#define LOG_TAG "sysprop"
#define LOGD(...) do { if (g_verboseLogs > 0) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__); } while (0)
#define LOGI(...) do { if (g_verboseLogs > 0) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__); } while (0)
#define LOGE(...) do { if (g_verboseLogs > 0) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__); } while (0)
