# Binary Source Layout

This directory is reserved for source code of compiled artifacts shipped with IntegrityBox Ultimate.

Plain WebUI, JavaScript, shell scripts, and config files are already readable inside the module ZIP, so they do not need a separate source mirror here.

## Directory Map

- `zygisk/` - source code and build notes for native Zygisk libraries shipped as `.so` files.
- `dex/` - Java source of the spoofing engine compiled to `classes.dex`.
- `rkp/` - Rust source of the device RKP helper shipped as `bin/ib-rkp`.
- `boot-hash-attestation/` - source code and build notes for the Boot Hash Java/Dex helper shipped as `boot_attest.jar`.
- `licenses/` - third-party licenses, notices, and attribution files for compiled parts.

Prebuilt binaries remain in the module/release package. This directory should explain how those binaries are produced and which source revision they came from.
