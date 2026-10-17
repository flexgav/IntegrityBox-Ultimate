cmake_minimum_required(VERSION 3.22.1)

project(playintegrityfix LANGUAGES CXX)

# Property-area virtualization has no inline-hook dependency or configure-time downloads.
add_library(${CMAKE_PROJECT_NAME} SHARED
    main.cpp
    app_compatibility.cpp
    runtime_config.cpp
    companion_protocol.cpp
    build_fields.cpp
    dex_payload.cpp
    property_virtualizer.cpp
    local_cxa_atexit_finalize_impl/atexit.cpp)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE log)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE
        -Oz -ffunction-sections -fdata-sections -fno-unwind-tables
        -fno-asynchronous-unwind-tables -fstack-protector-strong -flto=thin)
    target_link_options(${CMAKE_PROJECT_NAME} PRIVATE
        -flto=thin
        -Wl,--gc-sections
        -Wl,--exclude-libs,ALL
        -Wl,-z,relro -Wl,-z,now
        -Wl,--strip-all)
endif()
