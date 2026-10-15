cmake_minimum_required(VERSION 3.22.1)

project(playintegrityfix)

enable_language(C CXX ASM)

# --- Inline hook: ShadowHook (ByteDance android-inline-hook) ------------------
# PIFork v18 switched from Dobby to ShadowHook because GMS started detecting
# Dobby (broke <A13 Play Integrity, Wallet, RCS). Fetched at configure time (the
# build kit already allows network for -FetchTools); the PATCH_COMMAND renames
# the library's own sh_linker_init so our dummy strong symbol in main.cpp wins
# and ShadowHook's Bionic-linker tracking (which can crash newer Android) is
# bypassed. ShadowHook has no x86/x86_64 support, hence ARM-only ABIs.
include(FetchContent)
FetchContent_Declare(
  shadowhook
  GIT_REPOSITORY https://github.com/bytedance/android-inline-hook.git
  GIT_TAG        v2.0.1
  PATCH_COMMAND  perl -pi -e "s/sh_linker_init/sh_linker_init_dummy/g" shadowhook/src/main/cpp/sh_linker.c
)
FetchContent_GetProperties(shadowhook)
if(NOT shadowhook_POPULATED)
  FetchContent_Populate(shadowhook)
endif()

file(GLOB SHADOWHOOK_SRC
    ${shadowhook_SOURCE_DIR}/shadowhook/src/main/cpp/*.c
    ${shadowhook_SOURCE_DIR}/shadowhook/src/main/cpp/common/*.c
    ${shadowhook_SOURCE_DIR}/shadowhook/src/main/cpp/third_party/xdl/*.c
)

if(CMAKE_ANDROID_ARCH_ABI STREQUAL "armeabi-v7a")
    file(GLOB SHADOWHOOK_ARCH_SRC
        ${shadowhook_SOURCE_DIR}/shadowhook/src/main/cpp/arch/arm/*.c
        ${shadowhook_SOURCE_DIR}/shadowhook/src/main/cpp/arch/arm/*.S)
    include_directories(${shadowhook_SOURCE_DIR}/shadowhook/src/main/cpp/arch/arm)
elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a")
    file(GLOB SHADOWHOOK_ARCH_SRC
        ${shadowhook_SOURCE_DIR}/shadowhook/src/main/cpp/arch/arm64/*.c
        ${shadowhook_SOURCE_DIR}/shadowhook/src/main/cpp/arch/arm64/*.S)
    include_directories(${shadowhook_SOURCE_DIR}/shadowhook/src/main/cpp/arch/arm64)
endif()

include_directories(
    ${shadowhook_SOURCE_DIR}/shadowhook/src/main/cpp
    ${shadowhook_SOURCE_DIR}/shadowhook/src/main/cpp/common
    ${shadowhook_SOURCE_DIR}/shadowhook/src/main/cpp/include
    ${shadowhook_SOURCE_DIR}/shadowhook/src/main/cpp/third_party/bsd
    ${shadowhook_SOURCE_DIR}/shadowhook/src/main/cpp/third_party/lss
    ${shadowhook_SOURCE_DIR}/shadowhook/src/main/cpp/third_party/xdl
)

add_library(${CMAKE_PROJECT_NAME} SHARED
    main.cpp
    local_cxa_atexit_finalize_impl/atexit.cpp
    ${SHADOWHOOK_SRC} ${SHADOWHOOK_ARCH_SRC})

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE log)

# --- Release: размер/стелс/надёжность (C1-C4) -------------------------------
# -Oz + dead-strip уменьшают .so (меньше поверхность детекта), exclude-libs
# прячет внутренние символы из динамической таблицы, relro/now/stack-protector —
# дешёвое усиление целостности. LTO только для нашего кода: исходники ShadowHook
# (C/ASM) собираются своими настройками, thin-LTO на них не навязываем.
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set_source_files_properties(main.cpp local_cxa_atexit_finalize_impl/atexit.cpp PROPERTIES
        COMPILE_OPTIONS "-Oz;-ffunction-sections;-fdata-sections;-fno-unwind-tables;-fno-asynchronous-unwind-tables;-fstack-protector-strong;-flto=thin")
    # -flto=thin must be on the link too, or thin-LTO codegen of our objects runs
    # without it and (on arm32) emits ELF-TLS __tls_get_addr for our thread_local
    # instead of the NDK's emulated TLS, which fails to link.
    target_link_options(${CMAKE_PROJECT_NAME} PRIVATE
        -flto=thin
        -Wl,--gc-sections
        -Wl,--exclude-libs,ALL
        -Wl,-z,relro -Wl,-z,now
        -Wl,--strip-all)
endif()
