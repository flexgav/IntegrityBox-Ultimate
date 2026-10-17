# Spoofing dex — first-party Java sources

First-party Java source of the spoofing engine that the native module
(`sources/zygisk/src/main.cpp`) injects into target processes via
`InMemoryDexClassLoader`. Compiled to `classes.dex` (Java counterpart of the
zygisk `.so`).

## Layout

- `java/es/chiteroman/playintegrityfix/`
  - `EntryPoint.java` — DroidGuard `initAttestation`: KeyStore provider and
    package signature operations, each returning its actual completion bit.
  - `EntryPointVending.java`, `BuildFields.java` — legacy Java Build path,
    retained for old native/dex pairs during updates.
  - `CustomProvider.java`, `CustomKeyStoreSpi.java` — AndroidKeyStore JCA shim.
  - `CustomPackageInfoCreator.java` — package signature swap.
- `proguard-rules.pro` — R8 keep rules (the JNI/JCA contract surface).

## JNI / dex contract (do NOT break)

`dex_payload.cpp` loads DEX only for DroidGuard Provider/Signature. Native Build
writes already ran, so `initAttestation` disables lazy Java Build reapplication.
The current entry contract is:

| Class (package `es.chiteroman.playintegrityfix`) | Method | Descriptor |
| --- | --- | --- |
| `EntryPoint` | `initAttestation` | `(III)I` — verbose, provider, signature; result bits 1=Provider, 2=Signature |

An unavailable Provider or failed creator replacement returns a missing bit,
which native handles as failure. No exception alone is not proof of success.
Signature first tries reflection; if ART blocks the final-field write it uses
platform `jdk.internal.misc.Unsafe` field accessors, then verifies the value and
clears PackageManager/Parcel caches. Direct JNI writes to PackageInfo.CREATOR
are deliberately avoided: Android 17 can abort rather than throw an exception.

These old descriptors remain available for compatibility:

| Class (package `es.chiteroman.playintegrityfix`) | Method | Descriptor |
| --- | --- | --- |
| `EntryPoint` | `receiveJson` | `(Ljava/lang/String;)V` |
| `EntryPoint` | `init` | `(IIII)V` — verbose, build, provider, signature |
| `EntryPointVending` | `init` | `(IIILjava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V` — verbose, finger, sdk, fpValue, manuf, model, device, brand |

Class names, public method names/descriptors and the package are kept by
`proguard-rules.pro`. Changing an entry requires matching native changes and a
rebuild of both artifacts. The new native path falls back to old `init` with
build=0 only if `initAttestation` is absent; old DEX cannot report completion bits.

## Divergence from upstream

- **No third-party dependency.** Upstream uses
  `org.lsposed.hiddenapibypass:hiddenapibypass`. Here the hidden-API lift is
  inlined in `EntryPoint.liftHiddenApiRestrictions()` (meta-reflection →
  `dalvik.system.VMRuntime#setHiddenApiExemptions(["L"])`). The dex carries no
  `org.lsposed.*` classes.
- **Shared `Logger`** (`Logger.java`) replaces the duplicated static
  `LOG/LOGI/LOGE` scaffolding in `EntryPoint` / `EntryPointVending`.
- **Legacy `BuildFields`** (`BuildFields.java`) unifies the reflective
  `android.os.Build` / `Build.VERSION` writers (was `setField` /
  `setBuildField` / `spoofSdkInt`) and **caches** resolved `Field`s.
- **Legacy `spoofDevice()` applies once per profile** — a guard makes the repeated
  calls from `CustomProvider.getService()` (every KeyStore lookup) a no-op;
  `receiveJson()` resets the guard for a re-injected profile.
- **`CustomKeyStoreSpi`** guards a null delegate (`spi()`) instead of throwing a
  bare `NullPointerException`.
- Current Build/Vending/per-app operations run in native `build_fields.cpp`
  without loading DEX. The retained Java Build helpers are compatibility code;
  they are not invoked by the current runtime.

## Building

Use the standalone build kit at the repo root — no Android Studio / Gradle:

```
dex-buildkit/build_dex.ps1 -FetchTools
```

It compiles these sources with `javac` and runs **R8** (matching the upstream
`minifyReleaseWithR8` release pipeline) to produce `dex-buildkit/out/classes.dex`.
See `dex-buildkit/README.md` for prerequisites and details.

## Build environment (record per release)

- **JDK:** 21+ (compiled with `--release 21`, matching upstream
  `sourceCompatibility = VERSION_21`).
- **compileSdk:** 36 (any `android.jar` >= API 28 works for compilation).
- **minSdk / d8 `--min-api`:** 26.
- **Upstream base:** PlayIntegrityFork (chiteroman / osm0sis lineage).
