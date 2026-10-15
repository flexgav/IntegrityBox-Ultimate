> Release Date: 11/10/2026

#### 🛠️ IntegrityBox Ultimate v42.7

- **Stopped planting foreign system properties**: the boot scripts and Prop Spoofer used to *create* lock-state and OEM properties even on devices that never had them (e.g. Samsung/MIUI/Realme props on a Motorola, or `sys.oem_unlock_allowed` on Android 16+, which the system itself no longer sets). A property that does not exist on stock is itself a detection signal, so the module now only normalises properties the device already defines and never creates new ones. This clears the "System Properties" red flag some detectors raised after an update.
- **New — Hide Extra Root Managers** (Module Settings, reboot): reversibly hides the data folders of inactive root managers (for example a leftover Magisk sitting beside an active KernelSU) to avoid multiple-root detection. It only acts when exactly one manager is actually running, and everything is restored when you turn it off or uninstall.
- **Switching keybox backend now carries your settings over**: changing the active backend (e.g. from Tricky Store to TEESimulator) used to leave the new backend running with its default app list until the next scheduled refresh, so apps you target (Wallet, Play Services, …) were not actually covered. The module now migrates your existing target list, active keybox and patch level to the new backend right after the switch — fully offline, without resetting any of your update schedules. Removing a backend without installing another is now handled cleanly as an "undefined" state (shown as such in status and the report) with no action taken, instead of silently falling back to a default.
- **OhMyKeymint device identity kept in sync**: when OhMyKeymint is the active keybox backend, its attested device identity (brand/device/product/manufacturer) now follows the active spoof profile, so it can no longer drift from the device props the engine sets.
- **Cleaner diagnostics & cleanup**: Anti-Detection Nuke now also clears leftover root-manager and GApps installer logs from Download, and the report lists any loose keybox `.xml` files found in Download (a common accidental leak).
- **Engine updated to PlayIntegrityFork v18**: the native hooking layer moved from Dobby to ShadowHook and gained Android 17 field-write support, for better compatibility on current Android builds.

#### 🛠️ IntegrityBox Ultimate v42.5

- **Keybox revocation awareness (Google ban list)**: keyboxes are now checked against Google's certificate revocation list, so a banned key is no longer treated as usable. The check uses only the public bulk list (downloaded, matched locally, discarded) — your specific key is never sent anywhere, so it cannot accelerate a ban. Each keybox in the catalog is tagged **ACTUAL / REVOKED / UNCHECKED**.
- **Smarter keybox selection**: the engine now prefers the freshest **non-revoked** keybox. If the active one gets revoked it automatically switches to the next actual keybox; if *all* candidates are revoked it keeps the current one applied (a revoked key can still pass for a few days until GMS syncs the ban) and auto-switches as soon as an actual keybox appears — the device is never left without a keybox.
- **Accurate keybox publication date**: the "Publication Date" no longer silently falls back to the sync day when the date lookup fails — a keybox no longer looks freshly-published when it isn't, and a failed date lookup can no longer let an old keybox win selection over a genuinely newer one.
- **Keybox date lookup no longer hits GitHub API limits**: the provider publication date is now read from GitHub's public Atom history feed (`github.com/.../<file>.atom`) instead of the REST API. This drops the 60-requests/hour-per-IP quota entirely (which failed often on carrier-grade NAT where many users share one IP) and needs no token — so the date is read reliably, and keybox freshness ordering stays correct.
- **An emptied provider now removes its keyboxes instead of resurrecting old ones**: when a cloud provider is reachable but serves no keys (empty or deleted index, or a key pulled from distribution), the sync now clears that provider's entries from the catalog. Previously "provider returned nothing" was indistinguishable from "provider unreachable", so it fell back to the cached catalog and brought the withdrawn (often revoked) keyboxes straight back. Genuine unreachability still falls back to cache, as before — the two cases are now told apart.
- **Download detection hardened against upstream text changes**: keybox providers, the HMA template, the target list and the tool list judged a download by scanning the response body for the literal string "404: Not Found". That breaks the moment the host rewords its 404 page or the CDN changes. All of them now key off the actual HTTP status code (200 = have it, 4xx/5xx = gone, no response = unreachable) through one shared fetch helper with a consistent timeout.
- **Keybox status in WebUI**: both Keybox Loader and Integrity Checker now show actuality on its own line (🟢 ACTUAL / 🔴 REVOKED / ⚪ UNCHECKED), while the card icon carries a different signal — origin: blue for a cloud keybox, amber for a local one. The card itself stays neutral so neither signal drowns the other. The in-app guides explain that availability and actuality are separate axes: a keybox can be **ONLINE yet REVOKED**.
- **AutoPilot no longer drifts into the night**: scheduled Keybox/PIF/Target refreshes are now anchored to a fixed grid instead of the completion time, so each cycle no longer creeps ~1.5h later day after day. The daily run stays at roughly the same time it started at.
- **AutoPilot defers instead of wasting a slot when offline**: if a scheduled network refresh lands while offline (e.g. airplane mode), the slot is retried on every following cycle (~1h) until connectivity returns, instead of being consumed and waiting a full day (or three for PIF). A long offline gap cleanly re-anchors the schedule to when connectivity returns.
- **GMS Props Spoof now off by default**: the `spoofProps` toggle (Advanced Spoofing) turned out to break Google Wallet on some setups, while the main build spoofing works fine without it — so it is now disabled by default for new installs and for a fresh Advanced/Supreme setup. Existing installs are migrated once on update; if you had deliberately switched the toggle **on** in the WebUI, your choice is preserved, and re-enabling it later is never overridden again.
- **Manual Action always refreshes**: pressing the **Action** button now always pulls the latest Keybox and PIF, ignoring the cooldown timers. Previously a manual Action within the cooldown window could silently skip the update and leave you on a stale (or expired) keybox — especially noticeable right after updating over an older build.
- **Post-install reminder**: the installer now ends with a clear highlighted note to press **Action** after reboot and follow the on-screen steps.
- **Anti-Detection Nuke — Aggressive no longer hangs the WebUI**: the aggressive cleanup used to wipe the whole `/data/local/tmp` from inside the very shell the WebUI runs through, which cut the completion signal and left the loader spinning forever. That blanket wipe was removed (targeted tmp traces are still cleaned); the operation now finishes and closes normally.
- **Stuck-task recovery**: the execution-lock system now treats its timeout as allowed *silence* rather than total duration, with a heartbeat inherited by child tasks — a genuinely long-but-alive job (e.g. a slow keybox download) keeps its lock, while a truly stalled IntegrityBox process is detected and cleared instead of blocking future runs indefinitely.
- **Spoofing changes now actually take effect — DroidGuard cache is dropped**: restarting Google services was force-stopping GMS but leaving its DroidGuard verdict cache in place, so GMS relaunched and replayed the *old* result. A spoofing change therefore looked like it had not applied — most visibly with **Google Wallet**, which kept rejecting cards ("device doesn't meet security requirements") until a full GMS data wipe was done, logging you out in the process. That cache is now cleared alongside the restart. This covers **every** toggle in Advanced Spoofing, not just the GMS Tools button, so a setting change applies immediately without a trip to GMS Tools. Cache only — your Google account and settings are never touched, and the Play Store cache is deliberately left alone so toggling doesn't trigger an icon re-download.
- **Wallet Reset now clears the services cache too**: the button wiped Google Wallet/GPay data but left GMS holding the same stale verdict to hand straight back to the freshly reset wallet. It now performs the cache drop as well, and the confirmation states plainly that you will **not** be signed out.
- **Main screen no longer breaks at large "Display size" settings**: the dashboard and the tile grid were sized in fixed units, so raising Android's display size shrank the available width while the text stayed put — labels overflowed their cells and overlapped the status values, and tile text simultaneously collapsed to its minimum on tablets. Both grids are now sized against their own cell, which makes the layout independent of the display-size setting by construction. Dashboard labels and tile headings are also driven by a single shared definition, so they can no longer drift apart, and the grids now break to a new column count at the same width instead of at two different ones.
- **App Data Cleaner now covers system apps**: the list is no longer limited to user-installed packages — system packages are included too (marked with a **SYSTEM** badge, sorted below your own apps) so GMS-adjacent and vendor components can be cleaned from the same place. The search field filters across both.
- **App Data Cleaner safety guard**: packages whose full wipe would reset every system setting, kill mobile network, wipe the root manager's module list or bootloop the device (framework, Settings storage, SystemUI, telephony providers, keystore, root managers, known Xiaomi triggers) are **locked out of Full Reset** and cannot be selected. A second tier is flagged **RISK** — selectable, but a wipe loses Bluetooth pairings, keyboard dictionaries or the media index, and the confirmation now says so. Both apply only to Full Reset: clearing *cache* stays available for every package, since it destroys nothing. GMS/Play/Wallet are deliberately not restricted — wiping them is a supported GMS Tools scenario.
- **App Data Cleaner — bulk-select buttons removed**: "Select All" / "Clear Selection" are gone, so a Full Reset is always an explicit, per-app choice rather than one tap away from wiping everything the current filter matched.
- **Two Russian strings were missing**: the "PIF pipeline busy" and "Failed to switch profile" messages in Profile Selector had no translation and silently fell back to English; both are localized now.
- **WebUI text rendering fixed**: metadata badges no longer switch fonts depending on the page language (a plain `monospace` was resolving to different system fonts for Cyrillic vs Latin, throwing off alignment).
- **Bundled tools updated**: ZygiskNext 1.5.0, HMA-OSS oss-166, Duck Detector and others refreshed. **TrickyStore is now offered in two variants** — the original (5ec1cff) and the actively-maintained FOSS fork **TrickyStoreOSS** (beakthoven) — selectable in the tool catalog (both share the same `tricky_store` module id, so only one is installed).

> Release Date: 24/06/2026

#### 🛠️ IntegrityBox Ultimate v42.4

- **Profile switching — false error fixed**: applying a profile could show a red "operation failed" message even when the switch actually completed successfully; success is now verified against the device's real active-profile state instead of the raw shell exit code.
- **Keybox Loader — sync now also applies the keybox**: the sync button no longer just rebuilds the visible list — it now selects and activates the current keybox into Slot 1 in the same step, and AutoPilot's schedule correctly recognizes a manual sync so the next automatic check isn't needlessly delayed. Renamed to **Sync & Apply Keybox** ("Обновить Кейбоксы") to match, with the in-app guide updated to describe the actual selection policy (freshest cloud keybox always wins over a manual pin).
- **App list loading hardened**: fixed cases where the installed-app list (Spoof Apps, App Data Cleaner, Applist Detector, Target Box) could fail to populate or load very slowly on some devices; app names are now generated directly from the package identifier instead of a bundled native tool.
- **WebUI visibility timing fixed**: the WebUI button no longer appears from a background AutoPilot cycle right after a fresh install — it now unlocks only after a user-initiated Action run, as intended.
- **Status labels clarified**: the Activity Log's "Last check" (Keybox) and "Last read" (Fingerprint) timestamps actually showed when the active file was last written, not when it was last checked; both are now labeled **Last applied** to reflect what they really measure.
- **Scheduling reliability**: Keybox, PIF, and Target refresh intervals are now defined in a single shared place instead of separately in AutoPilot and Action, removing a risk of the two silently drifting apart if a schedule is adjusted in the future.

> Release Date: 22/06/2026

#### 🚀 IntegrityBox Ultimate v42.3

- **First-run setup reworked**: the initial setup flow is now split into safe automatic steps and optional advanced steps. In Manual mode, sensitive stages are confirmed with Vol+/Vol- prompts; in Auto mode, defaults are applied without extra interaction.
- **Metamodule and mount compatibility**: improved handling for KernelSU/APatch mount setups. The module now sets protective mount markers and the diagnostic report checks Mountify plus other metamodule-style solutions.
- **Execution locks for critical tasks**: Action, AutoPilot, PIF handling, Target updates, Keybox synchronization, and tool downloads are protected from duplicate parallel execution. Repeated starts now detect active work, stale locks, and stuck processes instead of running a second cycle over the first one.
- **Safer updates**: before installing an update, the module stops old runtime workers so code from the previous version does not continue running alongside the new installation.
- **Target Box pipeline**: target list handling is now centralized. `target.txt` generation supports scheduled refresh, cached/local fallback, current-file backup, and Manual mode protection. Reports now include Target Box status, `target.txt` contents, and the file itself without duplicating the same data in JSON.
- **PIF Processing 2.0**: PIF handling is now a single managed flow. Pool update and profile application timestamps are separated (`last_pif_check`, `last_pif_pool_update`, `last_pif_applied`), so AutoPilot can schedule by the actual applied profile while Status still reflects the active local file.
- **Keybox synchronization rebuilt**: provider sync now has clearer diagnostics, provider-level logs, cache preservation, local XML import, deduplication, cleanup of stale entries, and protection against parallel sync runs.
- **Tool downloader rebuilt**: tool downloads are now a separate controlled stage with list validation, checksum verification, explicit return codes, and a dedicated diagnostic log.
- **Structured diagnostics**: runtime and service scripts now use structured file logs with level, stage, event, return code, and message fields. Terminal output for installer and Action has been unified and cleaned up.
- **Boot and runtime reliability**: post-boot, background, uninstall, and verification stages now expose clearer return codes and fewer silent failures. AutoPilot watchdog handling, timeout checks, stale process cleanup, and runtime marker cleanup were improved.
- **Expanded report package**: reports now include metamodule state, Target Box state, PIF/security patch state, companion module status, raw/effective TEE state, and diagnostic log copies.
- **Status and WebUI updates**: Status now shows PIF source, metadata state, TEE state, companion modules, CDN/network diagnostics, and active-file state more clearly. UI cards and shared components were polished, including automatic-height behavior for profile/mode blocks.
- **WebUI reorganized**: modules were renamed to clear, purpose-based names and regrouped for easier navigation (e.g. Certified → GMS Tools, Risky → Applist Detector, Control → Zygiskless Mode). **Target Box and Target Simulator were merged** into a single Target Box module. **Spoof Apps** was added as a new module for per-app attestation spoofing.
- **Spoof ROM Props — 20+ new ROM families**: coverage expanded well beyond the original set with AlphaDroid, ArrowOS, Avium, Bliss, CalyxOS, Clover, GrapheneOS, Halcyon, LMODroid, Lumine, Lunaris, MatrixX, Mica, MistOS, PixelOS, Sakura, StatiX, Superior, WitaQua, YAAP, and more.
- **Per-app spoofing engine**: the bundled dex and Zygisk companion libraries were rebuilt to drive the new Spoof Apps flow, and a cached app-label helper makes the app picker (Spoof Apps / HMA) load faster.
- **Keybox policy — freshest cloud always wins**: keybox selection now always prefers the newest published cloud keybox and supersedes a manual pin (important after an upstream revocation); the manual pin applies only when no cloud keybox is reachable (offline / sync failure).
- **Advanced Spoofing — A10 mode fix**: Play Store SDK Downgrade (A10) is now mutually exclusive with Pixel / Vending-fingerprint / Provider / Signature spoofing, and its state is persisted to both `cust.spf.prop` and `custom_toggles.env` so it survives later toggles.
- **Default profile — Pixelify**: when no profile control file is present, the module now applies the Pixelify profile (provider spoof off) instead of an advanced preset, to avoid breaking STRONG attestation.
- **Maintenance & hardening**: bundled tools updated (ZygiskNext 1.4.0 and others), execution-lock and SKIP-path handling tightened, PIF pool streamlined, and a broad WebUI polish and localization pass across all modules.

> Release Date: 31/05/2026

#### 🚀 IntegrityBox Ultimate v41.5

- **Zero-config first install**: on fresh install the module now runs a full automated setup sequence — disables ROM native GMS/Vending spoofing to prevent conflicts, applies Banking Mode defaults (ADB, Wi-Fi ADB, developer options, OEM unlock all turned off), auto-extracts and saves the real Boot Hash, runs Standard Nuke cleanup, applies ZygiskNext stealth optimizations, attempts Widevine L1 repair if TEE is marked broken, syncs the Keybox catalog from cloud, and enables AutoPilot Xtreme with recommended flags. On update, **Vol Down** retains existing config; **Vol Up** wipes Box-Brain and re-runs the full setup as if fresh.
- **New module — Spoof ROM Props**: dedicated manager for Custom ROM property trace cleanup, moved from Module Settings where it existed as two toggles. **Auto** mode scans the device and enables matching cleanup cards; **Manual** mode lets you target specific ROM families: Generic, LineageOS, crDroid, Evolution X, DerpFest, RisingOS, AOSPA, PixelExperience, Pixel, Infinity, Havoc, Axion.
- **Keybox priority and persistence**: rebuilt keybox selection logic. New order: **CLOUD → CACHED → LOCAL**. A preferred keybox can be pinned — cloud takes priority when available, selection persists as fallback when cloud is unreachable. Sync now supports multiple keyboxes per provider, detects duplicates, and preserves the previous catalog on network failure.
- **Security Patch — manual date override**: **Auto / Manual** toggle added to the Security Patch module. Manual mode uses a visual month/day picker stored in `manual_security_patch.cfg` with priority over PIF metadata auto-detection.
- **Status — CACHED keybox indicator**: Integrity Checker now distinguishes a cloud Keybox whose provider is temporarily unavailable (**CACHED**) from a live online source. State badges (Online, Offline, Local, Cached, Missing) are fully localized.
- **Help Center — richer diagnostics**: Report.sh rewritten with +270 lines of new logic: JSON-structured output, ROM family trace detection across all supported families, full Keybox state (hash, preferred, source, SHA256), and service script status.
- **KeyboxLoader**: **CACHED** badge and provider name now shown per keybox entry in the list; sync label updated to reflect catalog rebuild flow.
- **Module Settings — UI cleanup**: `Spoof Lineage Props` and `Lineage v2` flags removed; both are now part of the dedicated Spoof ROM Props module.
- **Dynamic UI scaling**: WebUI now measures the host system text scale via a hidden probe element and adjusts the root font size (`--ibu-root-font-size`) accordingly. Scales down on compact screens (< 360 px wide or < 700 px tall) and responds live to resize and orientation changes.
- **AutoPilot mode documentation**: Xtreme and Keybox Only mode descriptions rewritten from daemon source. **Xtreme**: hourly — targets + security patches; every 24 h — Keybox catalog rebuild and injection; every 3 days — PIF rotation (pool of 10 profiles) + GMS cache wipe. **Keybox Only**: hourly — target scope only; every 24 h — Keybox only.

> Release Date: 21/05/2026

#### 🛠️ IntegrityBox Ultimate v41.0

- **Russian WebUI localization**: added full Russian interface translation, automatic system-language detection, and manual language switching in Module Settings.
- **HMA Profile**: expanded the hiding profile with more banking, system, and sensitive apps. After updating, apply **Inject HMA Template** to refresh the profile.
- **Inject HMA Template**: improved config injection on firmware where direct `/data` access is isolated, and removed unnecessary HMA app launches during the operation.
- **App Data Cleaner**: fixed stale selection counting. The cleanup button no longer shows a phantom app count when the saved selection contains apps that are no longer installed.
- **Fingerprint Selector**: added a dedicated PIF Pool update button for users running AutoPilot in **Keybox Only** mode.
- **Interface polish**: adjusted help dialogs, cards, status labels, buttons, and dynamic action messages for clearer Russian and English wording.
- **Help Center**: improved diagnostic report generation and log output formatting.

> Release Date: 15/05/2026

#### 🚀 IntegrityBox Ultimate v40.7

- **Boot Hash Spoofer**: unified real boot hash extraction through boot sources with KeyAttestation fallback. Magic Wand remains available as a backup option.
- **Get Real Boot Hash**: one-tap extraction of the original bootloader hash for stricter banking and security checks.
- **Anti-Detection Nuke**: deep root-trace cleanup with Soft, Standard, and Aggressive modes for logs, LSPosed cache, detector leftovers, and tool traces.
- **Fix Widevine L1**: hardware TEE/Widevine repair flow for supported BBK devices such as OnePlus, Oppo, Realme, plus Motorola.
- **Smart Security Patch**: safer `system=prop` mapping, HyperOS/ColorOS fixes, and generated patch dates for Google ban workarounds.
- **Target Manager**: Auto/Manual modes, `target.txt` import/backup, and a cleaner layout with Target Box and Target Simulator placed together.
- **Target Simulator**: per-app hardware-backed attestation tuning, automatic save on changes, import/backup flow, and keybox availability checks.
- **Quick Access**: long-press any WebUI tile to pin it to the home screen.
- **Integrity Downloader**: selectable downloads with checkboxes instead of downloading the whole tool bundle at once.
- **PixelMask download**: PixelMask was added to the downloadable tools list for Google Photos Pixel-feature scenarios.
- **Stealth Guide**: refreshed HMA, Banking Mode, SELinux, Zygisk/Shamiko, file hiding, Nuke, and app cleanup guidance.
- **HMA Profile**: updated hiding profile. After updating, apply **Inject HMA Template**, force stop the target app, and launch it again.
- **Help Center**: diagnostic archive now includes system data, installed apps, modules, and relevant logs.
- **Hide Files**: hidden directories are shown again as Vault/Masked and can be restored from the WebUI.
- **Keybox Pool**: improved XML cleanup before publishing and synchronization.
- **Documentation**: refreshed Russian and English README sections for setup, HMA, Play Store, Wallet, Boot Hash, System Prop Spoofer, and anti-detection flow.

> Release Date: 17/04/2026

#### 🚀 INTEGRITYBOX ULTIMATE - REBORN

- **Advanced Resource Delivery**: Core components are now securely and dynamically fetched.
- **Enhanced Stealth Mechanisms**: Upgraded structural integrity to prevent automated analysis.
- **Strict POSIX Shell**: Re-written core scripts for maximum Android compatibility.
- **Local Fallback System**: UI and configs work seamlessly even offline.
