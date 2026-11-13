# Installed HMA model reader

`HmaModel` loads the config model from the detected manager APK in a separate
`app_process`. It does not install an APK, start the manager application, connect
to its service or write native storage. No HMA classes or third-party runtime
libraries are bundled in the reader.

`schema` serializes a default `JsonConfig` and `JsonConfig.AppConfig` using the
installed APK's own implementation. Existing config values are read through
the same implementation, projected onto the current root fields and combined
with new defaults. The config version comes from the installed default model.
`scope.integritybox.schema` is private schema metadata, never an app assignment
written to native storage.

`verify` reads the prepared config through the actual `Companion.parse(String)`
and serializes it again. Every supplied key and value must survive. Added native
defaults are allowed; discarded/renamed fields, changed values, invalid native
types and unsupported constructors/readers fail with exit code 3. The shell API
checks this before stopping the manager or writing the config.

Known class locations are package adapters, not version tables:

- Original HMA / HMA-OSS: `icu.nullptr.hidemyapplist.common.JsonConfig`.
- HMAL: `com.android.hmal.common.JsonConfig`, with the earlier
  `icu.nullptr.hidemyapplist.common.JsonConfig` namespace used by release 4.1.1.

The verified upstream keep rules preserve these models and their nested classes.
A future incompatible model API fails explicitly. Field-name guesses and
fallbacks to stale static schemas are deliberately excluded.

Build on Windows with a JDK and the existing build kit:

```powershell
.\IntegrityBox-Ultimate-Serv\sources\hma-model\build.ps1 -Jdk 'C:\Program Files\Java\jdk-25.0.5'
```

`-AndroidJar` and `-R8Jar` override the build kit paths. Output is
`IntegrityBox-Ultimate-Clnt/common_scripts/hma_api/model.dex`, min API 28.
The ordinary module packer includes it and regenerates hashes. Rebuild this
artifact when changing the Java source; no module build or publication is run
by this script.

On Android, run `tests/device/cases/hma-model.sh` with the staged common_scripts,
APK path, package name and optionally the full bundled template. It exercises
the real reader, projection, complete round-trip and refusal of silently dropped
or renamed nested fields without changing an installed HMA configuration.
