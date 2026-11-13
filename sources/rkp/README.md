# Device RKP Helper Sources

First-party source of `bin/ib-rkp`, the executable that obtains the phone's own
RKP key from Google. It is a thin wrapper around the Device RKP feature of
[Duck ToolBox](https://github.com/eltavine/Duck-ToolBox) (MIT), pinned to
revision `019e1fbb5b305c7d9cb6d7b6f01a219c363ea75c`. Native V1 attribution:
https://github.com/eltavine/Duck-ToolBox/pull/19.

## Layout

- `src/main.rs` - CLI entry point. Registers only the Device RKP feature,
  queues the methods the device probe reports as eligible (V2, AIDL V1,
  native V1) and keeps a structured error for each attempt. It stops after a
  local publication failure or an unknown server submission, reports device
  refusals separately from Google/network failures, busy hardware and uncertain
  results. System RPC absence excludes only AIDL V1. Results include method
  availability, classified attempts and whether all eligible methods were
  checked. Generated XML is stripped from JSON output.
- `Cargo.toml`, `Cargo.lock` - pinned dependencies. Keep the lock file with
  the source.

The module's shell service owns policy, SLOT 1, expiry, retries and UI state.
The executable owns device discovery, firmware exchanges, Google requests and
cryptographic validation. The workbench, seed signing, extraction and other
Duck ToolBox features are not exposed.

## Building

Requirements: Rust with the `aarch64-linux-android` target, Android NDK, JDK
and SDK Build Tools 36.0.0. The executable targets Android API 26.

```
rustup target add aarch64-linux-android
ANDROID_HOME=<sdk> ANDROID_SDK_ROOT=<sdk> ANDROID_BUILD_TOOLS_VERSION=36.0.0 \
CARGO_TARGET_AARCH64_LINUX_ANDROID_LINKER=<ndk>/toolchains/llvm/prebuilt/<host>/bin/aarch64-linux-android26-clang \
CC_aarch64_linux_android=<ndk>/toolchains/llvm/prebuilt/<host>/bin/aarch64-linux-android26-clang \
AR_aarch64_linux_android=<ndk>/toolchains/llvm/prebuilt/<host>/bin/llvm-ar \
cargo build --locked --release --target aarch64-linux-android
```

The binary lands in `target/aarch64-linux-android/release/ib-rkp`. Host unit
tests: `cargo test --locked`. The build kit includes this canonical source
instead of maintaining a second copy.

## Licenses

Distributed binaries ship with Duck ToolBox's MIT license, its `rkp-v1-oracle`
MIT notice and the dependency notices generated from `Cargo.lock`; see
`../licenses/ib-rkp/`.
