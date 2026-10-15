# Zygisk Native Sources

First-party native source of the Zygisk module (a PlayIntegrityFork-engine fork:
property-read hook + companion protocol + JNI inject of the spoofing dex).

## Layout

- `src/main.cpp` — the only first-party source.
- `include/zygisk.hpp` — Zygisk API header.
- `include/local_cxa_atexit_finalize_impl/` — atexit finalizer helper.
- Inline hook: ShadowHook (ByteDance android-inline-hook), fetched by CMake
  FetchContent at configure time — no longer vendored. `include/Dobby/` is the
  previous vendored hook library and is now unused (safe to delete).
- `build/CMakeLists.txt` — build definition + release flags
  (`-Oz`, thin LTO, `--gc-sections`, `--exclude-libs,ALL`, `-z relro -z now`, `--strip-all`).

## Building

Use the standalone build kit at the repo root — no Android Studio / Gradle / SDK needed:

```
zygisk-buildkit/build_zygisk.ps1 -FetchTools
```

It builds all four ABIs straight from this source via the NDK + CMake and stages
them under `zygisk-buildkit/out/`. `classes.dex` (Java side) is reused as-is.
See `zygisk-buildkit/README.md` for prerequisites and details.

## Build environment (record per release)

- **NDK:** r29 (`29.0.14206865`).
- **C++ flags:** `-std=c++23 -fno-exceptions -fno-rtti -fvisibility=hidden`
  (mirrored by the build kit so the standalone `.so` matches the shipped one).
- **Upstream base:** PlayIntegrityFork (chiteroman / osm0sis lineage) v18 +
  ShadowHook. Local divergences from upstream live entirely in `src/main.cpp`;
  keep diffs in `patches/` if the upstream base is bumped.
- **ShadowHook (PIFork v18):** the `__system_property_read_callback` hook uses
  ShadowHook instead of Dobby (GMS detects Dobby). ARM-only (no x86/x86_64).
- **Android 17 field write:** `setFieldNative` (JNI `SetStatic*Field`) is bound
  from `inject()` onto the dex's `BuildFields` class, so Build/Build.VERSION
  spoofing keeps applying where ART blocks reflective writes to static final
  fields. The Java half lives in `sources/dex` (`BuildFields`); both halves must
  land together.

## Shipped artifacts

Regenerate via the build kit — do not hand-edit the binaries:

- `IntegrityBox-Ultimate-Clnt/zygisk/arm64-v8a.so`
- `IntegrityBox-Ultimate-Clnt/zygisk/armeabi-v7a.so`

(ARM-only since the ShadowHook switch; the old x86/x86_64 `.so` were removed.)

To verify a build matches the source, rebuild with the kit and compare (the
binaries are stripped; config/path string literals such as `cust.spf.prop` and
`spfapps.list` should be present, old `custom.pif.prop`/`spoofapps.list` absent).
