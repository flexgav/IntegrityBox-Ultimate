<div align="center">
  <img src="../ibu.png" alt="IntegrityBox Ultimate" width="100%">
</div>

<br>

<div align="center">
  <a href="../README.md"><img src="../assets/readme_ru_icon.png" alt="Русский" height="72"></a>
  &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
  <a href="./README.en.md"><img src="../assets/readme_en_icon.png" alt="English" height="72"></a>
  &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
  <a href="https://github.com/flexgav/IntegrityBox-Ultimate/releases/latest"><img src="../assets/download.png" alt="Download" height="72"></a>
  &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
  <a href="https://t.me/IntegrityBoxUltimateChatRU"><img src="../assets/tgru_icon.png" alt="Telegram RU" height="72"></a>
  &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
  <a href="https://t.me/IntegrityBoxUltimateChatEN"><img src="../assets/tgen_icon.png" alt="Telegram EN" height="72"></a>
</div>

---

<h1 align="center">IntegrityBox Ultimate</h1>

<p align="center">
  <b>Android Certification, Keybox, and Privacy Toolkit</b>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Recommended-Kitsune%20Magisk%20%2F%20KSU%20Next%20Spoofed-blue" alt="Recommended">
  <img src="https://img.shields.io/badge/Warning-Remove%20conflicting%20modules-orange" alt="Warning">
  <img src="https://img.shields.io/badge/Critical-Do%20not%20mix%20attestation%20backends-red" alt="Critical">
</p>

## 📌 Overview
**IntegrityBox Ultimate** is a practical toolkit for keeping an Android device with root access clean, certified, and easier to manage. It brings keybox handling, Play Integrity helpers, app-hiding templates, Google services cleanup, and device status checks into one Material You WebUI.

### ✨ Highlights
* ️ **Play Integrity in one place:** manage PIF, Keybox, Target List, Boot Hash, and Security Patch without stacking multiple conflicting modules.
* 🔑 **Keybox Hub:** cloud Keybox catalog, cached fallback, local XML import from `/sdcard/Download` and `/sdcard/Documents`, active Keybox selection, and state checks in **Integrity Checker**. The freshest published **non-revoked** cloud Keybox always takes priority and supersedes a manual pin; the manual pin only applies when the cloud is unreachable.
* 🚫 **Keybox revocation checks:** keyboxes are matched against Google's certificate revocation list, and each one is tagged **ACTUAL / REVOKED / UNCHECKED**. A revoked key is no longer treated as usable: the engine switches to the next actual keybox on its own, and if *all* candidates are revoked it keeps the current one applied (a revoked key still passes for a few days until GMS syncs the ban) and switches as soon as an actual keybox appears. Only the public bulk list is used — downloaded, matched locally, discarded — so your specific key is never sent anywhere and cannot accelerate its own ban.
* 🧬 **Fingerprint Selector:** built-in profile pool, scheduled PIF updates/application, manual Action refresh, and visible source information for the active profile.
* 🎯 **Target Box:** automatic Target List generation, protected Manual mode, import and backup, plus per-app Default/AOSP/Private profiles and AUTO/GENERATE/LEAF modes.
* 🗓️ **Security Patch:** automatic patch-date detection from PIF plus manual override through a date picker.
* 🔓 **Boot Hash Spoofer:** extract the real device Boot Hash and apply a manual override for apps that check bootloader or VBMeta state.
* 🛡️ **TEE / Widevine tools:** TEE state diagnostics, hardware-attestation backend support, and Widevine L1 repair attempts on supported devices.
* 🧹 **Spoof ROM Props:** automatic and manual cleanup for 30+ Custom ROM traces: LineageOS, crDroid, Evolution X, DerpFest, RisingOS, AOSPA, PixelExperience, Pixel, GrapheneOS, CalyxOS, PixelOS, ArrowOS, StatiX, and more (Generic covers the rest).
* 🎭 **Spoof Apps:** targeted device-fingerprint and property spoofing for individual third-party apps — beyond GMS and Play Store. Useful for apps that check `Build` fields locally, bypassing the Play Integrity verdict.
* 🕶️ **Root-environment hiding:** HideMyApplist template, suspicious-file hiding, TWRP/Fox/Magisk trace cleanup, and Anti-Detection Nuke.
* 🧰 **GMS Tools:** soft-restart Google Play Services/Play Store together with a GMS cache drop, including the DroidGuard verdict cache — without it a restart replays the old result and a spoofing change looks like it never applied. Plus Google Wallet data reset and a deep GMS/Play Store/GSF wipe to re-prepare the device for a fresh Play Store check.
* 🧼 **App Data Cleaner:** clear data and cache for individually selected apps, system packages included — handy after changing HMA, a Target profile, props, or Boot Hash, so the app re-checks the environment. Packages whose full wipe could reset every system setting, kill mobile connectivity or bootloop the device are locked out of accidental erasure; clearing *cache* stays available for every package.
* 🤖 **AutoPilot:** scheduled background maintenance for Keybox, PIF, Target Box, and security patches. The schedule is anchored to a fixed grid, so the cycle no longer creeps into the night, and a refresh that lands while offline is retried every hour until connectivity returns, instead of being burned for a full day.
* 📦 **Integrity Downloader:** selectable APK/ZIP/JSON downloads directly from WebUI.
* 📊 **Integrity Checker and Help Center:** active Keybox, PIF, TEE, Target Box, companion modules, network state, and diagnostic report export.
* 🎨 **Modern WebUI:** Material You interface, Quick Access, localization, built-in module guides, and AI Assistant.

### 🧭 Quick Navigation
* **Installing for the first time?** Start with [Installation & Ultimate Setup Guide](#-installation--ultimate-setup-guide).
* **Not sure what must be removed first?** See [Conflicting modules](#️-conflicting-modules-to-remove-or-disable).
* **Need Tricky Store, Zygisk, HMA, or other tools?** See [Core Requirements](#-core-requirements), [Optional, but highly recommended](#-optional-but-highly-recommended), and [Integrity Downloader](#-useful-tools-from-integrity-downloader).
* **Want manual control over Target List?** See [Automatic and Manual Target List Control](#-automatic-and-manual-target-list-control).
* **Something fails or an app detects root?** See [Quick Troubleshooting](#️-quick-troubleshooting).

---

## 🧩 Core Requirements
For comfortable use and maximum stealth, make sure the environment is built without conflicting root engines, attestation backends, or unnecessary traces in the user profile.

1. **Root solution.** Recommended order for devices where banking and government apps matter:

   - **Preferred:** [**SukiSU Ultra**](https://github.com/SukiSU-Ultra/SukiSU-Ultra/releases/latest) or [**KernelSU Next**](https://github.com/KernelSU-Next/KernelSU-Next/releases/latest). This is enough for the module to work; anti-detection can optionally be strengthened further with **SUSFS** (optional, see "Optional, but highly recommended").

   - **Alternative:** [**APatch**](https://github.com/bmax121/APatch/releases/latest), if there is no stable KernelSU / SukiSU kernel for your device or you specifically need kernel-based root without a Magisk-like setup.

   - **Fallback:** [**Kitsune Magisk / Kitsune Ufork**](https://t.me/KitsuneUfork). Use it only when KernelSU Next, SukiSU Ultra, or APatch are unavailable for your device. Stable behavior with strict banking apps is not guaranteed on Kitsune, because Magisk-like environments more often leave visible traces.

   Use only **one** root solution at a time. Do not mix Magisk / Kitsune, KernelSU Next, SukiSU Ultra, and APatch in the same system unless you fully understand the consequences.

2. **Hardware attestation backend.** Install [**Tricky Store**](https://github.com/5ec1cff/TrickyStore/releases/latest) or [**TEE Simulator**](https://github.com/JingMatrix/TEESimulator/releases/latest) if apps check hardware-backed keys and TEE / KeyMint attestation.

   Use only **one** attestation backend. Tricky Store, TrickyStoreOSS, TEE Simulator, and their forks must not run at the same time.

3. **WebUI.** To open the control panel, install [**MMRL**](https://github.com/MMRLApp/MMRL/releases/latest), [**WebUI X Portable**](https://github.com/MMRLApp/WebUI-X-Portable/releases/latest), or use built-in WebUI support in your Root Manager.

   If WebUI does not open directly, install [**KsuWebUIStandalone**](https://github.com/KOWX712/KsuWebUIStandalone/releases/latest) manually as a fallback first. After you can open WebUI, its APK can also be downloaded through Integrity Downloader.

4. **Mount metamodules.** IntegrityBox Ultimate itself does not require a separate metamodule just to work. A metamodule is only needed if you use other systemless modules that actually mount files into `/system`, `/vendor`, `/product`, `/system_ext`, `/odm`, or similar partitions.

   If you use KernelSU/APatch with a metamodule, keep only one mounting solution active at a time. IntegrityBox Ultimate sets `skip_mount` and `skip_mountify` markers so such solutions do not process it as a module with a system mount payload. Metamodule state is included in diagnostic reports.

## ⚠️ Conflicting modules to remove or disable

> [!IMPORTANT]
> Before installing, remove other Integrity / Play Integrity certification fix modules and tools so they do not conflict with IntegrityBox Ultimate.

> [!CAUTION]
> Do not mix multiple modules that change Keybox, fingerprint, GMS state, DroidGuard, VBMeta, or Play Integrity verdicts at the same time. These conflicts often cause unstable certification and false positives in banking apps.

<ul>
  <li><b>🧬 Play Integrity / DroidGuard spoofers:</b> PlayIntegrityFix / PIF, Play Integrity Fix Next / PlayIntegrityFix-NEXT, Play Integrity Fork / PIFork, PlayIntegritySuperFork, Play Integrity Fix Advanced, Strong Integrity Fix, SafetyNet Fix, Universal SafetyNet Fix, Displax SafetyNet Fix, and other PIF/SafetyNet forks.</li>
  <li><b>🧰 All-in-one Integrity / Keybox solutions:</b> Integrity-Box, Integrity Box forks, old IntegrityBox builds, Tricky Addon, Tricky Addon Enhanced / Update Target List, and similar modules if they manage keybox, target.txt, security patch, VBHash, or GMS state on their own.</li>
  <li><b>🔑 Key attestation / Keybox backend:</b> do not keep Tricky Store, TrickyStoreOSS, TEE Simulator, TEESimulator-RS, and their forks enabled at the same time. Leave only the backend selected for IntegrityBox Ultimate.</li>
  <li><b>🧾 Build props / fingerprint / security patch spoofers:</b> MagiskHide Props Config, Sensitive Props, Pixel Props / build.prop, Build-Prop-BETA, PixelFlasher PIF helper, XiaomiEU Injected PIF, and any ROM-bundled PIF / Pixel props spoof.</li>
  <li><b>📱 Pixel spoofing modules:</b> Pixelify, Pixelify Next, Pix3lify, Pixel Features, Google Photos Unlimited Backup, and similar modules if they change <code>ro.product*</code>, fingerprint, model, brand, security patch, or GMS properties.</li>
  <li><b>🧱 VBMeta / boot hash spoofers:</b> Android VBMeta Fixer, VBMeta Disguiser, and similar tools if IntegrityBox Ultimate Boot Hash / verifiedBootHash / VBMeta features are used at the same time.</li>
  <li>Any other module that changes fingerprint, build props, GMS state, DroidGuard, Keybox, attestation, security patch, VBMeta, boot hash, or Play Integrity verdicts.</li>
</ul>

## 🚨 Important for classic Magisk Stable users

> [!WARNING]
> Classic [**Magisk Stable**](https://github.com/topjohnwu/Magisk/releases/latest) is not the recommended option for IntegrityBox Ultimate if your goal is stable banking-app behavior and stricter environment checks. It is strongly recommended to move to [**Kitsune Magisk / Kitsune Ufork**](https://t.me/KitsuneUfork) or [**KSU Next Spoofed**](https://github.com/KernelSU-Next/KernelSU-Next/releases/latest) *(choose the APK with `-spoofed_...-release.apk` in the release assets)*; otherwise stable banking-app behavior is not guaranteed.

## ➕ Optional, but highly recommended

1. [**Zygisk Next**](https://github.com/Dr-TSNG/ZygiskNext/releases/latest) or [**ReZygisk**](https://github.com/PerformanC/ReZygisk/releases/latest) *(needed for Zygisk-based features unless you use the standalone Zygiskless Pixel Mode)*.
2. [**LSPosed / Vector**](https://github.com/JingMatrix/Vector/releases/latest) and [**HideMyApplist / HMA-OSS**](https://github.com/frknkrc44/HMA-OSS/releases/latest) *(recommended when banking or government apps react to app lists, root traces, or installed modules)*. Alternative HMA branch: [**Hide-My-Applist**](https://github.com/Dr-TSNG/Hide-My-Applist/releases/latest).
3. **SUSFS** *(optional, for maximum stealth).* An extra kernel-level hiding layer on top of SukiSU Ultra or KernelSU Next. Not required for the module to work, but it strengthens anti-detection against the strictest checks. It needs more than the Manager APK: a kernel / AnyKernel3 / boot image with SUSFS patches already built in for your exact device, Android version, kernel branch, and firmware; after flashing such a kernel, install the userspace module [**susfs4ksu / SUSFS-FOR-KERNELSU**](https://github.com/sidex15/susfs4ksu-module/releases/latest). Simply patching `boot.img`/`init_boot.img` through a Manager gives root but does not guarantee SUSFS: without patches in the kernel, the `susfs4ksu` module cannot enable kernel-level hiding.

## 📦 Useful tools from Integrity Downloader

> [!TIP]
> Integrity Downloader no longer downloads the whole bundle blindly. Enable the **Integrity Downloader** tile, tap **Apply Changes**, select only the tools you need in the checkbox dialog, and press **Download Selected**. After confirmation, a terminal window will show the download progress.

Current list from `assets/tools.list`:

* **ZygiskNext.zip** — current Zygisk Next for Root Managers without built-in Zygisk.
* **TrickyStore.zip** — hardware attestation backend (original, 5ec1cff).
* **TrickyStore_OSS.zip** — the same backend, an actively maintained FOSS fork (beakthoven). Installed instead of the original, not alongside it (same `tricky_store` module id).
* **KeyAttestation.apk** — app for manual certificate and verdict checks.
* **UpdateLocker.apk** — LSPosed module for blocking unwanted app updates.
* **HMA_Config.json** — ready-made IntegrityBox profile for HideMyApplist.
* **HMA_OSS.zip** — current HideMyApplist OSS build (flashed in the Root Manager).
* **PixelMask.apk** — LSPosed module for Pixel/GMS scenarios.
* **KSU_WebUI.apk** — standalone WebUI app for devices where the Root Manager does not open WebUI directly.
* **Core_Patch.apk** — LSPosed module for system install/signature restrictions.
* **Thor_Installer.apk** — manager/installer for additional Android tools.
* **Android_Faker.apk** — utility for manual Android identifier checks and setup.
* **LSPosedVector.zip** — LSPosed / Vector for HMA and other LSPosed modules.
* **Duck_Detector.apk** — root/Xposed/Magisk detector with native checks; useful for diagnosing what the root environment exposes.
* **Native_Root_Detector.apk** — native root detector showing exactly what apps can see during low-level environment scanning.

## 📁 Where to find files downloaded by Integrity Downloader

> [!NOTE]
> All selected APK/ZIP/JSON files downloaded by Integrity Downloader are saved to `/sdcard/IntegrityBox/Downloads`.

| Tool | File name | Full path |
| --- | --- | --- |
| Zygisk Next | `ZygiskNext.zip` | `/sdcard/IntegrityBox/Downloads/ZygiskNext.zip` |
| Tricky Store | `TrickyStore.zip` | `/sdcard/IntegrityBox/Downloads/TrickyStore.zip` |
| Tricky Store OSS | `TrickyStore_OSS.zip` | `/sdcard/IntegrityBox/Downloads/TrickyStore_OSS.zip` |
| Key Attestation | `KeyAttestation.apk` | `/sdcard/IntegrityBox/Downloads/KeyAttestation.apk` |
| Duck Detector | `Duck_Detector.apk` | `/sdcard/IntegrityBox/Downloads/Duck_Detector.apk` |
| Native Root Detector | `Native_Root_Detector.apk` | `/sdcard/IntegrityBox/Downloads/Native_Root_Detector.apk` |
| Update Locker | `UpdateLocker.apk` | `/sdcard/IntegrityBox/Downloads/UpdateLocker.apk` |
| HMA Config | `HMA_Config.json` | `/sdcard/IntegrityBox/Downloads/HMA_Config.json` |
| HideMyApplist OSS | `HMA_OSS.zip` | `/sdcard/IntegrityBox/Downloads/HMA_OSS.zip` |
| PixelMask | `PixelMask.apk` | `/sdcard/IntegrityBox/Downloads/PixelMask.apk` |
| KSU WebUI | `KSU_WebUI.apk` | `/sdcard/IntegrityBox/Downloads/KSU_WebUI.apk` |
| Core Patch | `Core_Patch.apk` | `/sdcard/IntegrityBox/Downloads/Core_Patch.apk` |
| Thor Installer | `Thor_Installer.apk` | `/sdcard/IntegrityBox/Downloads/Thor_Installer.apk` |
| Android Faker | `Android_Faker.apk` | `/sdcard/IntegrityBox/Downloads/Android_Faker.apk` |
| LSPosed / Vector | `LSPosedVector.zip` | `/sdcard/IntegrityBox/Downloads/LSPosedVector.zip` |

---

## 🚀 Installation & Ultimate Setup Guide
For a clean setup and the best chance of restoring Play Store certification, follow these steps:

> [!TIP]
> If this is your first setup, follow the steps in order and do not enable extra tools until you complete the first Play Store certification check.

1. ✅ **Install Dependencies:** Make sure your Root Manager is installed and only one hardware attestation backend is selected.
2. 📲 **Flash IntegrityBox Ultimate:** Install the module zip. During installation, the module prepares metamodule compatibility and stops old background workers from the previous version.
3. 🧭 **Choose setup behavior:** If prompted **"Erase previous installation data?"** — press **Vol Up** to reset or **Vol Down** to keep existing settings. On a clean install, you will also see the first-run setup mode prompt: **Vol Up** selects Manual mode with confirmation for advanced stages, **Vol Down** or timeout selects Auto mode with default values.
4. 🔄 **Reboot your device.** On first boot, the module performs initial setup: baseline safe defaults, Target Box, PIF, Keybox, Boot Hash, Security Patch, AutoPilot, and service profiles. In Manual mode, additional stages are confirmed separately to avoid overwriting user settings without permission.
5. ▶️ **Run the main action:** Open your Root Manager's module list and tap **Action** on the IntegrityBox Ultimate card. Right after boot the module may still be running first-boot background tasks, so **Action** can fail to start right away and show a message that another operation is already in progress — if that happens, wait a couple of minutes and try again. Wait until it finishes: it will refresh Keybox data, check PIF/Target schedules, and run only the stages that are actually needed.
6. 🔎 **Check the result:** Open WebUI → **Toolkit** → **Integrity Checker**. For the active Keybox check **both axes**: availability — the status should be `ONLINE`; and actuality — the **Actuality** line should read `ACTUAL`. A Keybox can be `ONLINE` and `REVOKED` at once (synced from the cloud but banned by Google) — if so, refresh it via **Keybox Hub** → **Keybox Loader** → **Sync & Apply Keybox**. Also confirm the page correctly shows PIF source, Target Box, TEE, and companion modules.
7. 🧼 **Deep Clean GMS:** Go to **Play Integrity Fix** → **GMS Tools** and run **Deep GMS Wipe**. This removes old Google certification states and Google Services Framework data. Reboot when prompted. *(You will be logged out of your Google Account.)*
8. ✅ **Re-login & Verify:** After rebooting, open the Play Store, log back into your Google account, and check your Play Protect certification status.
9. 🤖 **Check AutoPilot:** Go to **Auto Pilot** → **AutoPilot Manager** — the daemon should already be running in Xtreme mode. Switch to **Keybox Only** if you prefer minimal system impact.
10. 🧹 **Custom ROM Props if needed:** If your device runs a custom ROM, open **Detection** → **Spoof ROM Props** and enable **Auto Mode** — the module will scan for ROM-family traces and enable the matching cleanup cards automatically.
11. 📦 **Export a diagnostic package if needed:** If certification or apps are still unstable, open **Help Center** → **Export Report**. The archive will be saved to `/sdcard/IntegrityBox/Reports`.

> [!NOTE]
> Boot Hash and Widevine L1 can be configured automatically during first setup or confirmed manually in Manual mode. To adjust Boot Hash later — open **Detection** → **Boot Hash Spoofer**. To re-run Widevine L1 repair — use **Keybox Hub** → **Fix Widevine L1**.

> [!IMPORTANT]
> Do not tap **Action** repeatedly. Critical operations are protected from duplicate execution, but it is still better to wait for the current cycle to finish: a repeated run can be skipped as an already active operation.

### 🕶️ Advanced Stealth Setup
For users who need banking or government apps to see a cleaner device, we recommend setting up HideMyApplist (HMA) with the built-in helpers:

> [!IMPORTANT]
> If you already have your own HMA filter configured, do not run **Inject HMA Template** unless you actually need it: the IntegrityBox template can overwrite your current profile. During first setup, the module asks for confirmation for this stage separately.

> [!NOTE]
> In most cases, you do not need to open HMA manually before injecting the profile: IntegrityBox Ultimate attempts to create the required data directories and write the configuration automatically. If automatic injection fails, open HMA once, close it, and run **Inject HMA Template** again.

1. 📥 **Download Tools:** Open WebUI -> **Miscellaneous** -> **Module Settings**. Toggle **Integrity Downloader** ON and tap **Apply Changes**. In the dialog, select the tools you need with checkboxes. For the HMA scenario, you usually need **LSPosedVector.zip**, **HMA_OSS.zip**, and **HMA_Config.json**. Downloaded files will be saved to `/sdcard/IntegrityBox/Downloads`.
2. 🧩 **Install LSPosed / Vector:** Flash `/sdcard/IntegrityBox/Downloads/LSPosedVector.zip` in your Root Manager and reboot your device.
3. 🕵️ **Install HMA:** Flash `/sdcard/IntegrityBox/Downloads/HMA_OSS.zip` in your Root Manager, reboot, and enable HideMyApplist in LSPosed.
4. 🛡️ **Inject the HMA profile:** Open WebUI -> **Hide My Stuff** -> **Inject HMA Template**. This applies ready-made hiding rules for many banking and government apps directly into HMA.
5. 🧰 **Manual HMA import fallback:** If automatic injection fails, open the HMA app and manually import `/sdcard/IntegrityBox/Downloads/HMA_Config.json` through HMA's import/restore configuration flow.
6. 📱 **Install PixelMask for Google Photos features:** PixelMask can unlock selected Pixel features for Google Photos on your phone. Depending on the selected profile, this may include unlimited Original-quality photo and video backup or Pixel-only features such as Video Boost, Night Sight Video, Add Me, Reimagine, and Magic Editor.

#### For maximum stealth and anti-detection, use these tools according to the symptoms

1. 🏦 **Banking Mode:** Open **Toolkit** -> **Utility Box** and enable **Banking Mode**. It hides ADB/debug state and sets `sys.oem_unlock_allowed=0`.
2. 🛡️ **SELinux Enforcing:** On the home page, open **Cleanup & SELinux** -> **Enforce SELinux** and make sure the module status shows SELinux as `Enforcing`.
3. 🧩 **Strict Zygisk/Shamiko isolation:** Use **Zygisk Providers & Shamiko** -> **Enable Whitelist Mode** to enable strict isolation. Configure the app list separately in your Root Manager's DenyList or in the ZygiskNext/Shamiko settings.
4. ⚙️ **Optimize Zygisk provider:** Tap **Zygisk Providers & Shamiko** -> **Optimize Zygisk Provider** if you use Zygisk Next or a supported fork. The module applies recommended controller stealth settings.
5. 🧰 **Anti-detection flags:** In **Miscellaneous** → **Module Settings**, enable only what you actually need: **Debug Fingerprint**, **Debug Build**, **Build Tag**, **Clear LSposed**, **Spoof Encryption**, **Hide Recovery**, **Clear Gapps Logs**, and **Archive Manager Logs**. For ROM trace cleanup, use **Detection** → **Spoof ROM Props**.
6. 🗂️ **Hide suspicious files:** Use **Hide My Stuff** -> **Hide Suspicious Files** if an app sees `TWRP`, `Fox`, `Magisk`, root managers, or old traces on `/sdcard`. Do not add random system paths.
7. 🧹 **Reset the app state:** After changing HMA, Target, props, or Boot Hash, open **Cleanup & SELinux** -> **App Data Cleaner** and clear data/cache for the problematic app so it re-checks the environment.
8. 🔥 **Anti-Detection Nuke:** Use **Detection** -> **Anti-Detection Nuke** only for clear leftover traces. Start with **Soft Cleanup**, then **Standard Nuke**. Keep **Aggressive Nuke** as the last resort.
9. 🧪 **System Prop Spoofer:** Use **Detection** -> **System Prop Spoofer** only if you understand which `getprop` traces need to be reset or removed.

<details>
<summary><strong>Detailed System Prop Spoofer table</strong></summary>

#### Purpose summary

| Section | What it affects | When to use |
| --- | --- | --- |
| Reset Props | ADB, debug/dev state, secure mode, bootloader/Verified Boot, build type, OEM unlock, emulator flags. | When dangerous props need to be overwritten with safer user/locked/release-like values. |
| Duck Detector Props | A small set of props often checked by strict native detectors. | When the keys themselves should be removed from the current property space instead of changing their values. |
| Delete Props | ROM-branded properties, Pixel/EliteProps/PIF leftovers, and other traces from old spoofing modules. | When the property itself should disappear from the current property space. |
| Nuke Trash | `*.odex`, `*.vdex`, `base.odex` under `/data/app`. | When a detector reacts to stale odex/vdex artifacts after modules or app updates. |

#### Reset Props

| UI item | Props / action | Purpose |
| --- | --- | --- |
| USB Debug Block | `sys.usb.adb.disabled=1` | Applies a signal that USB ADB is disabled. |
| MTP Only Mode | `persist.sys.usb.config=mtp`, `sys.usb.config=mtp`, `sys.usb.state=mtp` | Removes ADB from USB configuration and leaves MTP mode. |
| ADB Root Off | `service.adb.root=0`, `service.adb.tcp.port=-1` | Disables root shell over ADB and ADB over TCP. |
| Secure Mode | `ro.secure=1`, `ro.adb.secure=1` | Restores secure-build and secure-ADB signals. |
| Debug Off | `ro.debuggable=0`, `persist.sys.debuggable=0` | Hides debug/userdebug state. |
| Dev Options Off | `persist.sys.developer_options=0`, `persist.sys.dev_mode=0` | Resets persistent Developer Options signals. |
| Global Settings | `development_settings_enabled=0`, `adb_enabled=0`, `oem_unlock_allowed=0` | Resets Android global settings for Developer Options, ADB, and OEM unlock. |
| Verified Boot | `ro.boot.verifiedbootstate=green`, `vendor.boot.verifiedbootstate=green` | Applies green Verified Boot state. |
| Flash Locked | `ro.boot.flash.locked=1` | Applies the signal that flash partitions are locked. |
| VBMeta Locked | `ro.boot.vbmeta.device_state=locked`, `vendor.boot.vbmeta.device_state=locked` | Applies locked VBMeta/device state. |
| SecureBoot | `ro.secureboot.lockstate=locked` | Applies locked Secure Boot state. |
| Warranty Valid | `ro.boot.warranty_bit=0` | Resets the warranty bit to a non-triggered value. |
| User Build | `ro.build.type=user`, `ro.build.tags=release-keys` | Presents the build as a normal release build. |
| OEM Lock | `ro.oem_unlock_supported=0`, `sys.oem_unlock_allowed=0` | Hides OEM unlock support/permission signals. |
| No Emulator | `ro.kernel.qemu=0`, `ro.boot.qemu=0`, `ro.hardware.virtual_device=0` | Removes basic emulator/virtual-device indicators. |
| Nuke Trash | Delete `*.odex`, `*.vdex`, `base.odex` under `/data/app` | Cleans stale app optimization artifacts; this is not a `getprop` change. |

#### Duck Detector Props

| UI item | Prop | Action |
| --- | --- | --- |
| ADB Secure | `ro.adb.secure` | Deletes the prop with `resetprop --delete` if it exists in the current property space. |
| Verified Boot State | `ro.boot.verifiedbootstate` | Deletes the prop with `resetprop --delete` if it exists in the current property space. |
| Verity Mode | `ro.boot.veritymode` | Deletes the prop with `resetprop --delete` if it exists in the current property space. |
| Build Tags | `ro.build.tags` | Deletes the prop with `resetprop --delete` if it exists in the current property space. |
| Build Type | `ro.build.type` | Deletes the prop with `resetprop --delete` if it exists in the current property space. |

#### Delete Props

Every item below **deletes the selected prop** from the current Android property space with `resetprop --delete`. The value is not spoofed or replaced. If the prop is absent, the module skips it.

| UI item | Prop |
| --- | --- |
| Lineage Device | `ro.lineage.device` |
| crDroid Device | `ro.crdroid.device` |
| Build Flavor | `ro.build.flavor` |
| Custom Device | `ro.custom.device` |
| Elite Version | `ro.build.elitever` |
| Xiaomi Dev ID | `ro.xiaomi.developerid` |
| Mod Version | `ro.modversion` |
| Elite Time | `ro.elite.version.code_time` |
| Elite Keybox | `sys.eliteprops.keybox` |
| Elite PIF | `sys.eliteprops.pif` |
| Elite Play Store | `sys.eliteprops.vending` |
| Elite Pixel | `sys.eliteprops.pixelprops` |
| Elite Photos | `sys.eliteprops.photos` |
| Elite Games | `sys.eliteprops.games` |
| Elite Snapchat | `sys.eliteprops.snapchat` |
| Elite Recent | `sys.eliteprops.recent` |
| Elite Recent All | `sys.eliteprops.recent.all` |
| Elite Spoof | `sys.eliteprops.spoofastab` |
| Pixel Device | `ro.pixel.device` |
| Pixel Version | `ro.pixel.version` |
| Pixel Build | `ro.pixel.build.version` |
| Pixel Release | `ro.pixel.releasetype` |
| Pixel Legal | `ro.pixellegal.url` |
| Evolution Device | `ro.evolution.device` |
| Evolution Build | `ro.evolution.build.version` |
| Evolution Display | `ro.evolution.display.version` |
| Evolution Version | `ro.evolution.version` |
| Evolution Legal | `ro.evolutionlegal.url` |
| Lineage SDK | `ro.lineage.build.version.plat.sdk` |
| Lineage Rev | `ro.lineage.build.version.plat.rev` |
| Rising Popup | `ro.rising.feature.pop_up_view` |
| Rising Chipset | `ro.rising.chipset` |
| Rising Maintainer | `ro.rising.maintainer` |
| Rising Code | `ro.rising.code` |
| Rising Package | `ro.rising.packagetype` |
| Rising Release | `ro.rising.releasetype` |
| Rising Version | `ro.rising.version` |
| Rising Build | `ro.rising.build.version` |
| Rising Display | `ro.rising.display.version` |
| Rising Codename | `ro.rising.platform_release_codename` |
| Rising Device | `ro.rising.device` |
| Rising Storage | `ro.rising.storage` |
| Rising RAM | `ro.rising.ram` |
| Rising Battery | `ro.rising.battery` |
| Rising Resolution | `ro.rising.display_resolution` |
| Lineage Version | `ro.lineage.version` |
| Lineage Display | `ro.lineage.display.version` |
| Lineage Build | `ro.lineage.build.version` |
| Lineage Release | `ro.lineage.releasetype` |
| Lineage Legal | `ro.lineagelegal.url` |
| Infinity Device | `ro.infinity.device` |
| Infinity SoC | `ro.infinity.soc` |
| Infinity Battery | `ro.infinity.battery` |
| Infinity Display | `ro.infinity.display` |
| Infinity Camera | `ro.infinity.camera` |
| Infinity Android | `ro.infinity.android.version` |
| Infinity Build | `ro.infinity.build.version` |
| Infinity Status | `ro.infinity.build.status` |
| Infinity Date | `ro.infinity.build.date` |
| Infinity Type | `ro.infinity.buildtype` |
| Infinity Fingerprint | `ro.infinity.fingerprint` |
| Infinity Version | `ro.infinity.version` |
| Infinity Maintainer | `ro.infinity.maintainer` |
| Havoc Variant | `ro.havoc.build.variant` |
| Havoc Device | `ro.havoc.device` |
| Havoc Date | `ro.havoc.build.date` |
| Havoc Build | `ro.havoc.build.version` |
| Havoc Fingerprint | `ro.havoc.fingerprint` |
| Havoc Release | `ro.havoc.releasetype` |
| Havoc Version | `ro.havoc.version` |
| Havoc Security | `ro.havoc.build.version.security_patch` |
| Derpfest Device | `ro.derpfest.device` |
| Derpfest Date | `ro.derpfest.build.date` |
| Derpfest Build | `ro.derpfest.build.version` |
| Derpfest Variant | `ro.derpfest.build.variant` |
| Derpfest Display | `ro.derpfest.display.version` |
| Derpfest Release | `ro.derpfest.releasetype` |
| Derpfest Version | `ro.derpfest.version` |
| Derpfest Legal | `ro.derpfestlegal.url` |
| Axion Device | `ro.axion.device` |
| Axion Version | `ro.axion.version` |
| Axion Display | `ro.axion.display.version` |
| Axion Build | `ro.axion.build.version` |
| Axion Release | `ro.axion.releasetype` |
| Axion Maintainer | `persist.sys.axion_maintainer` |
| Axion Processor | `persist.sys.axion_processor_info` |

</details>

### 🎯 Automatic and Manual Target List Control
In automatic mode, **Target Box** builds `/data/adb/Box-Brain/target.json` from a managed app list. You can view the current template in the repository:

[targetList/target.json](https://github.com/flexgav/IntegrityBox-Ultimate/blob/main/targetList/target.json)

The module first tries to fetch the latest list from the repository. If the network is unavailable, it uses the last cached list; if no cache exists yet, it falls back to the bundled list from the module ZIP. The working snapshot is stored as `/data/adb/Box-Brain/target.json`; native backend files are handled only by the backend API.

The automatic list includes:

* **Core Google packages:** Google Play Services, Play Store, Google Services Framework, and Google Wallet. These are always written.
* **Optional Google components:** SafetyCore, Google Contact Keys, Google Pay India, and similar packages. These are added only when installed on the device.
* **Checker apps:** Play Integrity, Key Attestation, Keybox Checker, and similar verification tools.
* **Root/Xposed/native detectors:** apps that check root, Zygisk, Xposed, Magisk, app lists, and low-level environment signals.
* **Additional checkers:** bootloader, NFC, DRM, and APK signature verification apps.

If the TEE/backend is marked as broken or its state cannot be determined, the module applies forced attestation through the active Keybox for Tricky Store compatibility.

If you want to use your own app list instead of the automatic template:

1. Open WebUI -> **Customize Tricky Store** -> **Target Box**.
2. Disable **Auto Update Target List** if you want full manual control over Target List.
3. Use **Import Target** to select an IntegrityBox Target List JSON file through the built-in file picker.
4. For per-app tuning, enable the app in Target Box, then select the **Default**, **AOSP**, or **Private** profile and the **AUTO**, **GENERATE**, or **LEAF** mode. You can also select several apps and apply one configuration in bulk.
5. If you enable **Auto Update Target List** again, the module will apply its own rules and refresh the displayed app list from the current Target List.

> [!NOTE]
> In automatic mode, Target Box updates by schedule through AutoPilot and when Action runs. Manual mode protects your own Target List from being overwritten. Before changing the file, the module creates a backup, and Target Box state is included in the diagnostic report.

---

## 📚 Built-in Knowledge Base
**IntegrityBox Ultimate includes an interactive Knowledge Base directly inside the WebUI.**

* 💡 **Module Guides:** Tap the lightbulb icon at the top of any module page.
* ℹ️ **Feature Details:** Tap the `( i )` buttons next to individual sections.
* 🤖 **AI Assistant:** An offline built-in assistant is available in the WebUI for common questions and troubleshooting.

---

## 🛠️ Quick Troubleshooting
> [!NOTE]
> Start with the simple checks first: Keybox status, **Restart Services**, then **Deep GMS Wipe** if needed, reboot, and Play Store certification check. Use advanced Target Box modes, Boot Hash Spoofer, and Nuke only when you know which exact check is failing.

### 🧪 Play Integrity fails
1. Open WebUI -> **Toolkit** -> **Integrity Checker**.
2. Check **Active Keybox**. These are **two independent axes** and both matter: availability — the status should be `ONLINE`; and actuality — the **Actuality** line should read `ACTUAL`. A Keybox can be `ONLINE` and `REVOKED` at the same time: synced from the cloud, but banned by Google and about to stop passing.
3. If the Keybox is missing, the status is not `ONLINE`, or it is flagged `REVOKED`, open WebUI -> **Keybox Hub** -> **Keybox Loader**. Select **Slot 1** at the top *(the primary slot)*, tap **Sync & Apply Keybox**, wait for the list to refresh, then tap a fresh Keybox carrying both the `PROVIDER` badge and the `ACTUAL` tag, and confirm loading it into **Slot 1**. If the cloud list does not appear, check your internet connection and GitHub access. If sync is still unavailable, place your own XML Keybox in `/sdcard/Download` or `/sdcard/Documents`, tap the refresh button, and load the detected `LOCAL` Keybox into **Slot 1**.
4. After updating the Keybox, run WebUI -> **Play Integrity Fix** -> **GMS Tools** -> **Deep GMS Wipe**, reboot, and check the Play Store again.

### 🛒 Play Store says "Device not certified"
1. Make sure Play Integrity already passes in **Integrity Checker**.
2. Enable hidden Play Store developer options: open **Play Store** -> tap your profile avatar in the top-right corner -> **Settings** -> **About** -> tap **Play Store version** 7 times until developer mode is enabled.
3. Check the integrity verdict: go back to Play Store **Settings** -> **General** -> **Developer options** -> in the **Play Integrity** section, tap **Check integrity**.
4. The result should show a passing integrity verdict. For normal Play Store certification, **Device integrity** / `MEETS_DEVICE_INTEGRITY` should pass; for Google Wallet and strict banking apps, **Strong integrity** / `MEETS_STRONG_INTEGRITY` is usually required.
5. Also check the certification status: **Play Store** -> profile avatar -> **Settings** -> **About** -> **Play Protect certification**. The expected status is **Device is certified**.
6. If the integrity verdict passes but the Play Store still says **Device is not certified**, open WebUI -> **Play Integrity Fix** -> **GMS Tools** -> **Deep GMS Wipe**.
7. Reboot, open the Play Store, sign in again, and wait a few minutes. Then repeat the integrity and certification checks above.

### 💳 Google Wallet / GPay fails
1. First run the built-in Play Integrity check through the Play Store: **Play Store** -> profile avatar -> **Settings** -> **General** -> **Developer options** -> **Play Integrity** -> **Check integrity**. If **Developer options** is not visible, enable it using the previous scenario.
2. Google Wallet usually requires **Strong integrity** / `MEETS_STRONG_INTEGRITY`. If only **Device integrity** passes, the Play Store may be certified, but Wallet can still reject payments.
3. **If you have just changed spoofing settings** (profile, fingerprint, **Advanced Spoofing** toggles), start with the light step: WebUI -> **Play Integrity Fix** -> **GMS Tools** -> the **Restart Services** button. It drops the DroidGuard verdict cache that keeps Google Play Services handing Wallet the **old** check result even though the settings already changed. Your cards and account are untouched. This is often enough — try adding a card right after.
4. If **Strong integrity** does not pass, go back to **Play Integrity fails**: check **Active Keybox** (both the `ONLINE` status and the `ACTUAL` tag), refresh the Keybox through **Keybox Loader**, then run **Deep GMS Wipe** and reboot.
5. If **Strong integrity** passes but Wallet still fails, open WebUI -> **Play Integrity Fix** -> **GMS Tools** -> the **Clear Wallet Data** button. It wipes Wallet's local state that cached a previous security failure and drops the services cache as well. You will **not** be signed out of your Google account, but you will most likely need to add your cards again.
6. Reboot, open Google Wallet, add your cards again if needed, and test contactless payment.

### 🏦 A banking app detects root, Xposed, or suspicious apps
1. Make sure **HideMyApplist / HMA-OSS** is installed, the HMA module is enabled in LSPosed, and the banking app is selected in HMA's LSPosed scope.
2. If the profile has not been applied yet, open WebUI -> **Hide My Stuff** -> **Inject HMA Template**. This imports the ready-made IntegrityBox configuration into HMA.
3. Open HMA and select the banking app in the list of apps that should see a hidden environment.
4. Apply the **FlexGAV 5.5** templates and presets to it: the blacklist/template for hidden apps, detector/root/sus-app presets, and settings presets such as accessibility/dev options/input method when available in your HMA build.
5. Save the HMA configuration, force stop the banking app or reboot, then test the app again.

### 🔐 An app fails hardware attestation / TEE
> [!IMPORTANT]
> Advanced Target Box profiles are configured per app. Do not apply them to all apps at once.

1. First make sure **Active Keybox** in **Integrity Checker** is `ONLINE`.
2. Open WebUI -> **Customize Tricky Store** -> **Target Box**.
3. Find the target app by name or package name.
4. Enable Target for the app, then select **Default** profile and **AUTO** mode for the first attempt. Changes are saved automatically.
5. Force stop the problematic app or reboot, then test it again.
6. If **AUTO** does not help, try **LEAF** for the same app. Use **GENERATE** only as a last resort when it is clear that the app does not accept the normal attestation chain.

### 🔓 An app complains about bootloader, VBMeta, or boot hash
1. Open WebUI -> **Detection** -> **Boot Hash Spoofer**.
2. Tap **Get Real Boot Hash**. The module first checks hardware boot sources: `/proc/cmdline`, device-tree, and `/proc/bootconfig`.
3. If boot sources do not return a valid hash, the module automatically tries the local Java Key Attestation fallback. You do not need to run it separately.
4. If the field is filled with a 64-character hash, tap **Apply**, then **Reboot**, and test the app again.
5. If you see **Extraction Failed**, open **Help Center** -> **Export Report** and save the diagnostic archive: it will include `boot_hash_extract.log` and `boot_hash_attestation.log`.
6. If the real hash cannot be obtained, use **Magic Wand** only as a fallback: it generates a valid 64-character hash, but it is not the real device value.
7. To roll the change back, return to **Boot Hash Spoofer**, tap **Reset**, and reboot.

### 📌 Need faster access to common tools
Long-press a WebUI tile for about 300 ms to pin it to **Quick Access** on the home page.

### 🆘 Nothing helped
If you still cannot restore Google certification or get the required apps working after all steps, open the Telegram group from the links at the top of this README and post in the help/support topic. Include your device model, ROM, Root Manager, attestation backend, Play Integrity result from the Play Store, and what you have already tried.

---

## 🧩 Binary Component Sources
The module ZIP already contains readable JS, HTML, CSS, shell scripts, and config files. Separate source folders are provided only for components shipped as compiled binaries:

- `sources/zygisk/` - source code and build notes for native Zygisk libraries from `IntegrityBox-Ultimate-Clnt/zygisk/*.so`.
- `sources/dex/` - Java source code the bundled `classes.dex` is built from (PIF entry point, provider, and keystore hooks).
- `sources/boot-hash-attestation/` - source code and build notes for the `boot_attest.jar` helper.
- `sources/licenses/` - licenses and notices for compiled components.

---

## 🙏 Credits & Acknowledgements
This project uses concepts and code from the following open-source work:
- @ez-me for ezme-nodebug.
- @osm0sis for PlayIntegrityFork.
- **LSPosed Team** for Shamiko's late start service script.
- **MeowDump** for the original Integrity-Box foundation.
- **You**, for using this module.
