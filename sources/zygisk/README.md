# Zygisk Native Sources

First-party native source of the Zygisk module (a PlayIntegrityFork-engine fork:
private property areas + companion protocol + JNI inject of the spoofing dex).

## Layout

- `src/main.cpp` — Zygisk lifecycle and orchestration.
- `src/runtime_config.cpp` — saved settings, target roles and operation planning.
- `src/companion_protocol.cpp` — bounded files and versioned socket transport.
- `src/build_fields.cpp` — native, typed Build/Build.VERSION writes.
- `src/dex_payload.cpp` — optional Provider/Signature payload integration.
- `src/property_virtualizer.cpp` — property-area installer adapted from
  [KOWX712/PlayIntegrityFix inject_s v4.9](https://github.com/KOWX712/PlayIntegrityFix/tree/1138c6f3cebb).
- `include/property_virtualizer.hpp`, `include/logging.hpp` — installer result
  contract and the shared runtime logging gate.
- `include/zygisk.hpp` — Zygisk API header.
- `include/local_cxa_atexit_finalize_impl/` — atexit finalizer helper.
- No inline-hook dependency or configure-time source downloads.
- `build/CMakeLists.txt` — build definition + release flags
  (`-Oz`, thin LTO, `--gc-sections`, `--exclude-libs,ALL`, `-z relro -z now`, `--strip-all`).

## Building

Use the standalone build kit at the repo root — no Android Studio / Gradle / SDK needed:

```
zygisk-buildkit/build_zygisk.ps1 -FetchTools
```

It builds arm64-v8a, armeabi-v7a and x86_64 via the NDK + CMake and stages
them under `zygisk-buildkit/out/`. Build the matching Java payload with
`dex-buildkit/build_dex.ps1` first; the native kit copies that result.
See `zygisk-buildkit/README.md` for prerequisites and details.

## Build environment (record per release)

- **NDK:** r29 (`29.0.14206865`).
- **C++ flags:** `-std=c++23 -fno-exceptions -fno-rtti -fvisibility=hidden`
  (mirrored by the build kit so the standalone `.so` matches the shipped one).
- **Upstream base:** PlayIntegrityFork v18 for the companion/dex integration,
  KOWX712 inject_s v4.9 for property-area virtualization, with local adaptations.
- **Property rules:** the active profile's dotted names and suffix wildcards are
  installed before specialization, only for DroidGuard and enabled per-app targets.
  Exact names win; longer suffixes win among overlapping wildcard rules.
  User toggles retain control over each operation.
- **Property safety:** every matched rule is prepared before mapping changes.
  The installer validates the area ABI and file identity, restores read-only
  protection, verifies values and rolls back failures. Existing private changes
  from other backends are preserved. A failed rollback ends only the unsafe child.
- **Long values:** read-only replacements use Bionic long encoding and available
  private-area space. Long mutable values or insufficient space reject preparation
  without partial application. Logcat remains silent unless verboseLogs is enabled.
- **Build writes:** direct JNI for String/int/long/boolean fields; strict numeric
  parsing and UTF-8 to UTF-16 conversion. Missing fields are skipped and failed
  values reported independently. Explicit profile fields override fingerprint
  expansion. Vending keeps the existing independent Pixel identity, fingerprint
  and SDK_INT=29 settings; it does not inherit upstream's SDK32 policy.
  Enabled identity writes include all five cached `*_FOR_ATTESTATION` fields;
  explicit Build overrides win. Vending's Pixel identity includes PRODUCT too.

## Runtime contract

The companion reads `cust.spf.prop` once per request and selects DroidGuard,
Vending or an exact entry in `spfapps.list` with saved `spoofApps=1`. The boot
property `sys.spfapps.cfg` only gates per-app connection attempts. Other GMS
processes are unmounted without receiving spoofing operations.

App entries are per Android user, in TEESimulator's form: `pkg` matches the
process in the primary user only, `pkg@N` in user N only. The user is the
process UID divided by 100000. DroidGuard and Vending ignore the user.

The nine existing settings remain authoritative; this runtime never migrates,
rewrites or overrides them. Dotted properties and suffix rules apply only with
`spoofProps` in DroidGuard/per-app; native Build writes obey `spoofBuild` there.
Vending obeys its own three settings. Provider/Signature are DroidGuard-only.

Per-app compatibility is checked before property replacement and Build writes.
An already-mapped `libpairipcore.so` skips all per-app spoofing for that process;
the module requests unloading, and no saved setting or app membership is changed.
Google roles remain independent. Missing/unreadable maps are logged and retain
the existing spoofing path. This ports the early v44 guard, not a complete APK
inventory: PairIP loaded after this check is not detected. Paths are matched by
exact library basename, including the kernel's deleted-file suffix.

DEX is read and transferred only when Provider or Signature is enabled for
DroidGuard. Missing DEX does not cancel native Build/property operations. The
Java result mask distinguishes successful Provider and Signature operations.
All operations disabled means no property writes, no Build writes and no DEX.
No upstream script-only marker or module.prop rewrite is introduced.

The wire header has a magic, version and fixed-width fields; version 2 requests
carry the Android user ID and version 1 requests are refused. Caps are 512 KiB
for configuration, 4 MiB for DEX and 256 KiB for app lists. A single 3-second
deadline covers socket I/O, including partial transfers; failures/EOF reject
incomplete replies. Files must be nonempty regular files within their cap.
Descriptors close through RAII; broken peers cannot trigger SIGPIPE.

The DEX direct-buffer backing storage must live as long as its class loader.
DEX targets retain the buffer/library; native-only children can unload after
installation. Rebuild the native and Java artifacts together. The old Java
entry points remain for binary compatibility, not as a second active Build path.

The adaptation uses the KOWX712 architecture without importing its profile
sources, settings policy, updater/UI, CRC module branding check or hardcoded
USB/ADB property normalization. Pool collection/API refresh and packaging stay
in IntegrityBox's existing pipelines. See the internal architecture notes and
`tests/native/README.md` for coverage and installed-module checks.

## Shipped artifacts

Regenerate via the build kit — do not hand-edit the binaries:

- `IntegrityBox-Ultimate-Clnt/zygisk/arm64-v8a.so`
- `IntegrityBox-Ultimate-Clnt/zygisk/armeabi-v7a.so`
- `IntegrityBox-Ultimate-Clnt/zygisk/x86_64.so`

To verify a build matches the source, rebuild with the kit and compare (the
binaries are stripped; config/path string literals such as `cust.spf.prop` and
`spfapps.list` should be present, old `custom.pif.prop`/`spoofapps.list` absent).

Host and Android smoke-test sources and the remaining release checks are in
`tests/native/README.md` at the workspace root. The smoke tests do not replace
the installed module or validate Wallet/Integrity verdicts.
