<div align="center">
  <img src="ibu.png" alt="IntegrityBox Ultimate" width="100%">
</div>

<br>

<div align="center">
  <a href="./README.md"><img src="./assets/readme_ru_icon.png" alt="Русский" height="72"></a>
  &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
  <a href="./readme_local/README.en.md"><img src="./assets/readme_en_icon.png" alt="English" height="72"></a>
  &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
  <a href="https://github.com/flexgav/IntegrityBox-Ultimate/releases/latest"><img src="./assets/download.png" alt="Скачать" height="72"></a>
  &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
  <a href="https://t.me/IntegrityBoxUltimateChatRU"><img src="./assets/tgru_icon.png" alt="Telegram RU" height="72"></a>
  &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
  <a href="https://t.me/IntegrityBoxUltimateChatEN"><img src="./assets/tgen_icon.png" alt="Telegram EN" height="72"></a>
</div>

---

<h1 align="center">IntegrityBox Ultimate</h1>

<p align="center">
  <b>Набор инструментов для сертификации, Keybox и приватности Android</b>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Recommended-Kitsune%20Magisk%20%2F%20KSU%20Next%20Spoofed-blue.svg" alt="Recommended">
  <img src="https://img.shields.io/badge/Warning-Remove%20conflicting%20modules-orange.svg" alt="Warning">
  <img src="https://img.shields.io/badge/Critical-Do%20not%20mix%20attestation%20backends-red.svg" alt="Critical">
</p>

## 📌 Обзор
**IntegrityBox Ultimate** — это удобный набор инструментов для Android с root-доступом: он помогает вернуть сертификацию Play Store, управлять Keybox, очищать следы Google-сервисов, настраивать скрытие приложений и проверять состояние устройства через понятный интерфейс в стиле Material You.

### ✨ Основные возможности
* ️ **Play Integrity в одном месте:** управление PIF, Keybox, Target List, Boot Hash и Security Patch без установки нескольких конфликтующих модулей.
* 🔑 **Центр Keybox:** облачный каталог Keybox, кэширование, локальный импорт XML из `/sdcard/Download` и `/sdcard/Documents`, выбор активного Keybox и проверка его состояния в **Проверке целостности**. Самый свежий опубликованный **неотозванный** облачный Keybox всегда в приоритете и вытесняет ручной выбор; ручной выбор действует только при недоступности облака.
* 🚫 **Проверка Keybox на отзыв:** ключи сверяются с бан-листом сертификатов Google, и каждый Keybox помечается **АКТУАЛЕН / ОТОЗВАН / НЕ ПРОВЕРЕН**. Отозванный больше не считается рабочим: при появлении отзыва движок сам переключается на следующий актуальный ключ, а если отозваны все — удерживает текущий (отозванный ещё несколько дней проходит, пока GMS не синхронизирует бан) и сменит его, как только появится актуальный. Используется только публичный общий список: он скачивается, сверяется локально и удаляется — ваш конкретный ключ никуда не отправляется и потому не может ускорить собственный бан.
* 🧬 **Fingerprint Selector:** встроенный пул профилей, обновление и применение PIF по расписанию, ручное обновление через Action и понятное отображение источника активного профиля.
* 🎯 **Target Box:** автоматическая сборка Target List, защищенный ручной режим, импорт и резервное копирование, а также точечное назначение приложениям профилей Default/AOSP/Private и режимов AUTO/GENERATE/LEAF.
* 🗓️ **Security Patch:** автоматическое определение даты патча из PIF и ручной Override через удобный выбор даты.
* 🔓 **Boot Hash Spoofer:** извлечение реального Boot Hash устройства и ручная подмена для приложений, которые проверяют состояние загрузчика или VBMeta.
* 🛡️ **TEE / Widevine инструменты:** диагностика состояния TEE, работа с backend’ом аппаратной аттестации и попытка восстановления Widevine L1 на поддерживаемых устройствах.
* 🧹 **Спуф ROM Props:** автоматическая и ручная очистка следов 30+ кастомных прошивок: LineageOS, crDroid, Evolution X, DerpFest, RisingOS, AOSPA, PixelExperience, Pixel, GrapheneOS, CalyxOS, PixelOS, ArrowOS, StatiX и другие (Generic — для остальных случаев).
* 🎭 **Spoof Apps:** точечный спуфинг отпечатка и свойств устройства для отдельных сторонних приложений — помимо GMS и Play Store. Полезно для приложений, которые проверяют `Build`-поля локально в обход вердикта Play Integrity.
* 🕶️ **Скрытие root-окружения:** готовый профиль HideMyApplist, скрытие подозрительных файлов и папок, очистка следов TWRP/Fox/Magisk и Anti-Detection Nuke.
* 🧰 **GMS Tools:** мягкий перезапуск Google Play Services/Play Store со сбросом кэша GMS, включая кэш вердикта DroidGuard — без него перезапуск переигрывает старый результат, и смена настроек спуфинга выглядит как неработающая. Плюс сброс данных Google Wallet и глубокая очистка GMS/Play Store/GSF для повторной подготовки устройства к проверке Play Маркета.
* 🧼 **App Data Cleaner:** очистка данных и кэша отдельно выбранных приложений, включая системные — удобно после смены HMA, Target-профиля, props или Boot Hash, чтобы приложение заново проверило окружение. Пакеты, полный сброс которых способен обнулить системные настройки, оборвать мобильную сеть или отправить устройство в bootloop, заблокированы от случайного стирания; очистка кэша при этом доступна для любого пакета.
* 🤖 **AutoPilot:** фоновое обслуживание Keybox, PIF, Target Box и патчей безопасности по расписанию без ручной рутины. Расписание привязано к фиксированной сетке, поэтому цикл не сползает со временем в ночь, а обновление, выпавшее на офлайн, не пропадает: попытка повторяется каждый час, пока связь не появится, вместо ожидания целых суток.
* 📦 **Integrity Downloader:** выборочная загрузка полезных APK/ZIP/JSON-инструментов прямо из WebUI.
* 📊 **Проверка целостности и Центр помощи:** проверка активного Keybox, PIF, TEE, Target Box, сопутствующих модулей, сетевого доступа и экспорт диагностического архива для поддержки.
* 🎨 **Удобный WebUI:** Material You интерфейс, быстрый доступ к избранным плиткам, локализация, справка внутри модулей и встроенный AI Assistant.

### 🧭 Быстрая навигация
* **Ставите модуль впервые?** Начните с раздела [Установка и полное руководство по настройке](#-установка-и-полное-руководство-по-настройке).
* **Не знаете, что нужно удалить перед установкой?** Смотрите [Конфликтующие модули](#️-конфликтующие-модули-которые-нужно-удалить-или-отключить).
* **Нужны Tricky Store, Zygisk, HMA или другие инструменты?** Смотрите [Основные требования](#-основные-требования), [Дополнительные, но настоятельно рекомендуемые](#-дополнительные-но-настоятельно-рекомендуемые) и [Integrity Downloader](#-полезные-инструменты-из-integrity-downloader).
* **Хотите вручную контролировать Target List?** Смотрите [Ручное управление Target List](#-ручное-управление-target-list).
* **Что-то не проходит или приложение видит root?** Смотрите [Быстрое решение проблем](#️-быстрое-решение-проблем).

---

## 🧩 Основные требования

Для комфортной работы и максимальной скрытности убедитесь, что окружение собрано без конфликтующих root-движков, backend’ов аттестации и лишних следов в пользовательском профиле.

1. **Root-решение.** Рекомендуемый порядок для устройств, где важна работа банковских и государственных приложений:

   - **Предпочтительно:** [**SukiSU Ultra**](https://github.com/SukiSU-Ultra/SukiSU-Ultra/releases/latest) или [**KernelSU Next**](https://github.com/KernelSU-Next/KernelSU-Next/releases/latest). Этого достаточно для работы модуля; при необходимости антидетект дополнительно усиливается **SUSFS** (опционально, см. раздел «Дополнительные, но настоятельно рекомендуемые»).

   - **Альтернатива:** [**APatch**](https://github.com/bmax121/APatch/releases/latest), если для устройства нет стабильного KernelSU/SukiSU-ядра или требуется kernel-based root без Magisk-подобной схемы.

   - **Fallback-вариант:** [**Kitsune Magisk / Kitsune Ufork**](https://t.me/KitsuneUfork). Используйте его только если KernelSU Next, SukiSU Ultra или APatch недоступны для вашего устройства. Для строгих банковских приложений стабильная работа на Kitsune не гарантируется, так как Magisk-подобные окружения чаще оставляют видимые следы.

   Используйте только **одно** root-решение одновременно. Не смешивайте Magisk/Kitsune, KernelSU Next, SukiSU Ultra и APatch в одной системе без полного понимания последствий.

2. **Backend для аппаратной аттестации.** Установите [**Tricky Store**](https://github.com/5ec1cff/TrickyStore/releases/latest) или [**TEE Simulator**](https://github.com/JingMatrix/TEESimulator/releases/latest), если приложения проверяют hardware-backed ключи и TEE/KeyMint-аттестацию.

   Используйте только **один** backend аттестации. Tricky Store, TrickyStoreOSS, TEE Simulator и их форки не должны работать одновременно.

3. **WebUI.** Для открытия панели управления установите [**MMRL**](https://github.com/MMRLApp/MMRL/releases/latest), [**WebUI X Portable**](https://github.com/MMRLApp/WebUI-X-Portable/releases/latest) или используйте встроенную поддержку WebUI в root-менеджере.

   Если WebUI не открывается напрямую, заранее установите [**KsuWebUIStandalone**](https://github.com/KOWX712/KsuWebUIStandalone/releases/latest) вручную как fallback-вариант. После первого входа в WebUI его APK также можно скачать через Integrity Downloader.

4. **Метамодули монтирования.** IntegrityBox Ultimate сам по себе не требует отдельный metamodule ради своей работы. Он нужен только если у вас есть другие systemless-модули, которые реально монтируют файлы в `/system`, `/vendor`, `/product`, `/system_ext`, `/odm` и похожие разделы.

   Если используете KernelSU/APatch с metamodule, держите активным только одно решение монтирования одновременно. IntegrityBox Ultimate выставляет маркеры исключения `skip_mount` и `skip_mountify`, чтобы такие решения не пытались обрабатывать его как модуль с системным mount-пayload. Состояние metamodule-подсистемы попадает в диагностический отчет.

## ⚠️ Конфликтующие модули, которые нужно удалить или отключить

> [!IMPORTANT]
> Перед установкой удалите другие модули и решения для починки Integrity / Play Integrity сертификации, чтобы они не конфликтовали с IntegrityBox Ultimate.

> [!CAUTION]
> Не смешивайте несколько модулей, которые одновременно меняют Keybox, fingerprint, GMS-состояние, DroidGuard, VBMeta или Play Integrity verdicts. Такие конфликты часто дают нестабильную сертификацию и ложные срабатывания банковских приложений.

<ul>
  <li><b>🧬 Play Integrity / DroidGuard spoofers:</b> PlayIntegrityFix / PIF, Play Integrity Fix Next / PlayIntegrityFix-NEXT, Play Integrity Fork / PIFork, PlayIntegritySuperFork, Play Integrity Fix Advanced, Strong Integrity Fix, SafetyNet Fix, Universal SafetyNet Fix, Displax SafetyNet Fix и другие PIF/SafetyNet-форки.</li>
  <li><b>🧰 All-in-one решения для Integrity / Keybox:</b> Integrity-Box, Integrity Box forks, старые сборки IntegrityBox, Tricky Addon, Tricky Addon Enhanced / Update Target List и аналоги, если они самостоятельно управляют keybox, target.txt, security patch, VBHash или GMS-состоянием.</li>
  <li><b>🔑 Key attestation / Keybox backend:</b> не держите одновременно Tricky Store, TrickyStoreOSS, TEE Simulator, TEESimulator-RS и их форки. Оставьте только тот backend, который выбран для работы с IntegrityBox Ultimate.</li>
  <li><b>🧾 Build props / fingerprint / security patch spoofers:</b> MagiskHide Props Config, Sensitive Props, Pixel Props / build.prop, Build-Prop-BETA, PixelFlasher PIF helper, XiaomiEU Injected PIF и любые ROM-встроенные PIF / Pixel props spoof.</li>
  <li><b>📱 Pixel spoofing modules:</b> Pixelify, Pixelify Next, Pix3lify, Pixel Features, Google Photos Unlimited Backup и похожие модули, если они меняют <code>ro.product*</code>, fingerprint, model, brand, security patch или свойства GMS.</li>
  <li><b>🧱 VBMeta / boot hash spoofers:</b> Android VBMeta Fixer, VBMeta Disguiser и аналоги, если одновременно используются функции IntegrityBox Ultimate для Boot Hash / verifiedBootHash / VBMeta.</li>
  <li>Другие модули, которые меняют fingerprint, build props, GMS-состояние, DroidGuard, Keybox, attestation, security patch, VBMeta, boot hash или Play Integrity verdicts.</li>
</ul>

## 🚨 Важно для пользователей классического Magisk Stable

> [!WARNING]
> Классический [**Magisk Stable**](https://github.com/topjohnwu/Magisk/releases/latest) не является рекомендуемым вариантом для IntegrityBox Ultimate, если цель — стабильная работа банковских приложений и строгих проверок окружения. Настоятельно рекомендуется перейти на [**Kitsune Magisk / Kitsune Ufork**](https://t.me/KitsuneUfork) или на [**KSU Next Spoofed**](https://github.com/KernelSU-Next/KernelSU-Next/releases/latest) *(в релизах выбирайте APK с `-spoofed_...-release.apk`)*, иначе стабильная работа банковских приложений не гарантируется.

## ➕ Дополнительные, но настоятельно рекомендуемые

1. [**Zygisk Next**](https://github.com/Dr-TSNG/ZygiskNext/releases/latest) или [**ReZygisk**](https://github.com/PerformanC/ReZygisk/releases/latest) *(нужны для функций на базе Zygisk, если вы не используете автономный Zygiskless Pixel Mode)*.
2. [**LSPosed / Vector**](https://github.com/JingMatrix/Vector/releases/latest) и [**HideMyApplist / HMA-OSS**](https://github.com/frknkrc44/HMA-OSS/releases/latest) *(рекомендуются, если банковские или государственные приложения реагируют на список приложений, root-следы или установленные модули)*. Альтернативная ветка HMA: [**Hide-My-Applist**](https://github.com/Dr-TSNG/Hide-My-Applist/releases/latest).
3. **SUSFS** *(опционально, для максимальной скрытности).* Дополнительный инструмент kernel-level скрытия поверх SukiSU Ultra или KernelSU Next. Для базовой работы модуля не требуется, но усиливает антидетект против самых строгих проверок. Нужны не только Manager APK, а ядро/AnyKernel3/boot-образ со встроенными SUSFS-патчами под конкретное устройство, версию Android, ветку ядра и прошивку; после прошивки такого ядра установите userspace-модуль [**susfs4ksu / SUSFS-FOR-KERNELSU**](https://github.com/sidex15/susfs4ksu-module/releases/latest). Простое патчирование `boot.img`/`init_boot.img` через Manager даёт root, но не гарантирует SUSFS: без патчей в ядре модуль `susfs4ksu` не включит kernel-level скрытие.

## 📦 Полезные инструменты из Integrity Downloader

> [!TIP]
> Integrity Downloader больше не скачивает весь набор вслепую. После включения плитки **Integrity Downloader** и нажатия **Применить изменения** появится окно выбора: отметьте чекбоксами только нужные утилиты и нажмите **Скачать выбранное**. После подтверждения откроется терминальное окно с прогрессом загрузки.

Текущий список из `assets/tools.list`:

* **ZygiskNext.zip** — актуальный Zygisk Next для Root-менеджеров без встроенного Zygisk.
* **TrickyStore.zip** — backend для аппаратной аттестации (оригинал, 5ec1cff).
* **TrickyStore_OSS.zip** — тот же backend, активно поддерживаемый FOSS-форк (beakthoven). Ставится вместо оригинала, не вместе с ним (один и тот же id модуля `tricky_store`).
* **KeyAttestation.apk** — приложение для ручной проверки сертификатов и verdict-данных.
* **UpdateLocker.apk** — LSPosed-модуль для блокировки нежелательных обновлений приложений.
* **HMA_Config.json** — готовый профиль IntegrityBox для HideMyApplist.
* **HMA_OSS.zip** — актуальная сборка HideMyApplist OSS (прошивается в Root-менеджере).
* **PixelMask.apk** — LSPosed-модуль для Pixel/GMS-сценариев.
* **KSU_WebUI.apk** — отдельное приложение WebUI для устройств, где Root-менеджер не открывает WebUI напрямую.
* **Core_Patch.apk** — LSPosed-модуль для патча системных ограничений установки/подписи.
* **Thor_Installer.apk** — менеджер/инсталлер для дополнительных Android-инструментов.
* **Android_Faker.apk** — утилита для ручной проверки и настройки Android-идентификаторов.
* **LSPosedVector.zip** — LSPosed / Vector для работы HMA и других LSPosed-модулей.
* **Duck_Detector.apk** — детектор root/Xposed/Magisk с нативными проверками; полезен для диагностики обнаружения рут-среды.
* **Native_Root_Detector.apk** — нативный детектор рут, показывающий что именно приложения могут видеть при низкоуровневом сканировании.

## 📁 Где искать файлы, скачанные через Integrity Downloader

> [!NOTE]
> Все выбранные APK/ZIP/JSON, скачанные через Integrity Downloader, сохраняются в `/sdcard/IntegrityBox/Downloads`.

| Инструмент | Имя файла | Полный путь |
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

## 🚀 Установка и полное руководство по настройке
Для чистой настройки и лучшего шанса вернуть сертификацию Play Store выполните эти шаги:

> [!TIP]
> Если вы настраиваете модуль впервые, проходите шаги по порядку и не включайте дополнительные инструменты до первичной проверки сертификации Play Маркета.

1. ✅ **Установите зависимости:** Убедитесь, что у вас установлен Root-менеджер и выбран только один backend аппаратной аттестации.
2. 📲 **Прошейте IntegrityBox Ultimate:** Установите zip-архив модуля. Во время установки модуль готовит совместимость с metamodule-решениями и останавливает старые фоновые процессы предыдущей версии.
3. 🧭 **Выберите поведение установки:** Если отображается запрос **«Стереть данные предыдущей установки?»** — нажмите **Vol Up** для сброса или **Vol Down** для сохранения настроек. При чистой установке появится выбор режима первичной настройки: **Vol Up** — Ручной режим с подтверждением дополнительных этапов, **Vol Down** или ожидание таймера — Авто режим с применением стандартных значений.
4. 🔄 **Перезагрузите устройство.** При первой загрузке модуль выполняет начальную настройку: включает базовые безопасные параметры, подготавливает Target Box, PIF, Keybox, Boot Hash, Security Patch, AutoPilot и служебные профили. В Ручном режиме дополнительные этапы запрашиваются отдельно, чтобы не перезаписать пользовательские настройки без подтверждения.
5. ▶️ **Запустите основное действие:** Перейдите в список модулей Root-менеджера и нажмите **Action** на карточке IntegrityBox Ultimate. Сразу после загрузки системы модуль может ещё выполнять фоновые задачи первой настройки, поэтому **Action** способен сработать не сразу и показать сообщение о том, что уже выполняется другая операция — в этом случае подождите пару минут и повторите попытку. Дождитесь завершения: модуль обновит Keybox, проверит расписание PIF/Target и применит только те этапы, которые действительно необходимы.
6. 🔎 **Проверьте статус:** Откройте WebUI → **Инструментарий** → **Проверка целостности**. У активного Keybox смотрите **обе оси**: доступность — статус `ONLINE`, и актуальность — строка **Актуальность** должна показывать `АКТУАЛЕН`. Keybox может быть `ONLINE` и при этом `ОТОЗВАН` (синхронизирован из облака, но забанен Google) — тогда обновите его через **Центр Keybox** → **Keybox Loader** → **Обновить Кейбоксы**. Убедитесь также, что страница проверки корректно показывает источники PIF, Target Box, TEE и сопутствующие модули.
7. 🧼 **Глубокая очистка GMS:** Перейдите в **Play Integrity Fix** → **GMS Tools** и нажмите кнопку **Глубокая очистка GMS**. Это удалит старые состояния сертификации Google и данные Google Services Framework. Следуйте подсказке, чтобы **перезагрузить** устройство. *(Вы выйдете из аккаунта Google.)*
8. ✅ **Повторный вход и проверка:** После перезагрузки откройте Play Маркет, снова войдите в аккаунт Google и проверьте статус сертификации Play Защиты.
9. 🤖 **Проверьте AutoPilot:** Перейдите в **Автопилот** → **Управление AutoPilot** — демон уже должен быть включён в режиме Xtreme. При необходимости переключите режим на **Keybox Only** для минимального воздействия на систему.
10. 🧹 **Спуф ROM Props при необходимости:** Если устройство работает на кастомной прошивке, откройте **Обнаружение** → **Спуф ROM Props** и нажмите **Авторежим** — модуль сам определит, следы каких ROM-семейств есть на устройстве, и включит нужные карточки очистки.
11. 📦 **Соберите диагностический архив при проблемах:** Если сертификация или приложения всё ещё работают нестабильно, откройте **Центр помощи** → **Экспорт отчета**. Архив появится в `/sdcard/IntegrityBox/Reports`.

> [!NOTE]
> Boot Hash и Widevine L1 могут быть настроены автоматически при первой установке или подтверждены вручную в Ручном режиме. Если потребуется изменить Boot Hash вручную — откройте **Обнаружение** → **Boot Hash Spoofer**. Для повторного запуска Widevine L1 — **Центр Keybox** → **Фикс Widevine L1**.

> [!IMPORTANT]
> Не нажимайте **Action** несколько раз подряд. Критичные операции защищены от дублирующего запуска, но лучше дождаться завершения текущего цикла: повторный запуск может быть пропущен как уже активная операция.

### 🕶️ Продвинутая настройка скрытности
Если банковские или государственные приложения слишком внимательно проверяют устройство, рекомендуем настроить HideMyApplist (HMA) через встроенные инструменты:

> [!IMPORTANT]
> Если у вас уже настроен собственный фильтр HMA, не применяйте **Применить шаблон HMA** без необходимости: готовый шаблон IntegrityBox может перезаписать текущий профиль. При первой настройке модуль запрашивает подтверждение для этого этапа отдельно.

> [!NOTE]
> Обычно HMA не нужно открывать вручную перед применением профиля: IntegrityBox Ultimate пытается сам создать нужные data-директории и записать конфигурацию. Если применение не сработало, откройте HMA один раз, закройте его и повторите **Применить шаблон HMA**.

1. 📥 **Скачайте инструменты:** Откройте WebUI -> **Разное** -> **Настройки модуля**. Включите **Integrity Downloader** и нажмите **Применить изменения**. В появившемся окне отметьте чекбоксами нужные позиции. Для HMA-сценария обычно нужны **LSPosedVector.zip**, **HMA_OSS.zip** и **HMA_Config.json**. Скачанные файлы появятся в `/sdcard/IntegrityBox/Downloads`.
2. 🧩 **Установите LSPosed / Vector:** Прошейте `/sdcard/IntegrityBox/Downloads/LSPosedVector.zip` в вашем Root-менеджере и перезагрузите устройство.
3. 🕵️ **Установите HMA:** Прошейте `/sdcard/IntegrityBox/Downloads/HMA_OSS.zip` в Root-менеджере, перезагрузитесь и включите HideMyApplist в LSPosed.
4. 🛡️ **Примените готовый профиль HMA:** Откройте WebUI -> **Скрытие следов** -> **Применить шаблон HMA**. Это применит готовые правила скрытия для многих банковских и государственных приложений прямо в HMA.
5. 🧰 **Запасной ручной импорт HMA:** Если автоматическое применение не прошло, откройте приложение HMA и импортируйте скачанный файл `/sdcard/IntegrityBox/Downloads/HMA_Config.json` вручную через импорт/восстановление конфигурации HMA.
6. 📱 **Установите PixelMask для функций Google Photos:** PixelMask позволяет открыть на вашем смартфоне некоторые Pixel-функции Google Photos. В зависимости от выбранного профиля это может быть безлимитная загрузка фото и видео в исходном качестве или Pixel-only возможности вроде Video Boost, Night Sight Video, Add Me, Reimagine и Magic Editor.

### Для максимальной скрытности и антидетекта дополнительно используйте эти функции:

1. 🏦 **Banking Mode:** Откройте **Инструментарий** -> **Утилиты (Utility Box)** и включите **Banking Mode**. Он скрывает ADB/debug-состояние и выставляет `sys.oem_unlock_allowed=0`.
2. 🛡️ **SELinux Enforcing:** На главной откройте **Очистка и SELinux** -> **Enforce SELinux** и убедитесь, что в статусе модуля SELinux отображается как `Enforcing`.
3. 🧩 **Строгая изоляция Zygisk/Shamiko:** Используйте **Zygisk-провайдеры и Shamiko** -> **Whitelist Mode**, чтобы включить строгий режим изоляции. Список приложений для скрытия root/Zygisk настраивается отдельно в DenyList вашего Root-менеджера или в настройках ZygiskNext/Shamiko.
4. ⚙️ **Оптимизация ZygiskNext:** Нажмите **Zygisk-провайдеры и Shamiko** -> **Оптимизация Zygisk Provider**, если используете Zygisk Next. Модуль применит рекомендуемые stealth-настройки контроллера.
5. 🧰 **Антидетект-флаги:** В **Разное** → **Настройки модуля** включайте только нужные пункты: **Debug Fingerprint**, **Debug Build**, **Build Tag**, **Очистка LSposed**, **Подмена шифрования**, **Скрыть Recovery**, **Очистка логов Gapps** и **Архив логов менеджера**. Для очистки следов прошивки используйте **Обнаружение** → **Спуф ROM Props**.
6. 🗂️ **Скрытие подозрительных файлов:** Используйте **Скрытие следов** -> **Hide Suspicious Files**, если приложение видит папки `TWRP`, `Fox`, `Magisk`, менеджеры root или старые следы на `/sdcard`. Не добавляйте случайные системные пути.
7. 🧹 **Сброс состояния приложения:** После изменения HMA, Target, props или Boot Hash откройте **Очистка и SELinux** -> **App Data Cleaner** и очистите данные/кэш проблемного приложения, чтобы оно заново проверило окружение.
8. 🔥 **Anti-Detection Nuke:** Используйте **Обнаружение** -> **Anti-Detection Nuke** только при явных остаточных следах. Начинайте с **Мягкая очистка**, затем **Стандартная очистка**. **Агрессивная очистка** оставьте как крайний вариант.
9. 🧪 **System Prop Spoofer:** Используйте **Обнаружение** -> **System Prop Spoofer** только если понимаете, какие `getprop`-следы нужно сбросить или удалить.

<details>
<summary><strong>Подробная таблица System Prop Spoofer</strong></summary>

#### Сводка по назначению

| Раздел | Что затрагивает | Когда использовать |
| --- | --- | --- |
| Reset Props | ADB, debug/dev state, secure mode, bootloader/Verified Boot, build type, OEM unlock, emulator flags. | Когда нужно подставить безопасные user/locked/release-like значения вместо опасных текущих значений. |
| Duck Detector Props | Небольшой набор props, которые часто проверяют строгие native-детекторы. | Когда нужно удалить сами ключи из текущего property space, а не подменить их значения. |
| Delete Props | ROM-брендовые свойства, Pixel/EliteProps/PIF leftovers и другие следы старых spoof-модулей. | Когда нужно удалить сам факт наличия кастомного/спуфинг-свойства из текущего property space. |
| Nuke Trash | `*.odex`, `*.vdex`, `base.odex` в `/data/app`. | Когда детектор цепляется за старые odex/vdex-артефакты после модулей или обновлений приложений. |

#### Reset Props

| Пункт в UI | Props / действие | За что отвечает |
| --- | --- | --- |
| USB Debug Block | `sys.usb.adb.disabled=1` | Подставляет признак отключенного ADB по USB. |
| MTP Only Mode | `persist.sys.usb.config=mtp`, `sys.usb.config=mtp`, `sys.usb.state=mtp` | Убирает ADB из USB-конфигурации и оставляет режим MTP. |
| ADB Root Off | `service.adb.root=0`, `service.adb.tcp.port=-1` | Отключает root shell через ADB и ADB over TCP. |
| Secure Mode | `ro.secure=1`, `ro.adb.secure=1` | Возвращает признаки secure-сборки и защищенного ADB. |
| Debug Off | `ro.debuggable=0`, `persist.sys.debuggable=0` | Скрывает debug/userdebug-состояние. |
| Dev Options Off | `persist.sys.developer_options=0`, `persist.sys.dev_mode=0` | Сбрасывает persistent-признаки режима разработчика. |
| Global Settings | `development_settings_enabled=0`, `adb_enabled=0`, `oem_unlock_allowed=0` | Сбрасывает глобальные Android-настройки Developer Options, ADB и OEM unlock. |
| Verified Boot | `ro.boot.verifiedbootstate=green`, `vendor.boot.verifiedbootstate=green` | Подставляет green-состояние Verified Boot. |
| Flash Locked | `ro.boot.flash.locked=1` | Подставляет признак заблокированной прошивки flash-разделов. |
| VBMeta Locked | `ro.boot.vbmeta.device_state=locked`, `vendor.boot.vbmeta.device_state=locked` | Подставляет locked-состояние VBMeta/device state. |
| SecureBoot | `ro.secureboot.lockstate=locked` | Подставляет locked-состояние Secure Boot. |
| Warranty Valid | `ro.boot.warranty_bit=0` | Сбрасывает warranty bit к значению без срабатывания. |
| User Build | `ro.build.type=user`, `ro.build.tags=release-keys` | Подставляет признаки обычной release-сборки. |
| OEM Lock | `ro.oem_unlock_supported=0`, `sys.oem_unlock_allowed=0` | Скрывает поддержку/разрешение OEM unlock. |
| No Emulator | `ro.kernel.qemu=0`, `ro.boot.qemu=0`, `ro.hardware.virtual_device=0` | Убирает базовые признаки emulator/virtual device. |
| Nuke Trash | Удаление `*.odex`, `*.vdex`, `base.odex` в `/data/app` | Очищает старые артефакты оптимизации приложений; это не `getprop`. |

#### Duck Detector Props

| Пункт в UI | Prop | Действие |
| --- | --- | --- |
| ADB Secure | `ro.adb.secure` | Удаляет prop через `resetprop --delete`, если он есть в текущем property space. |
| Verified Boot State | `ro.boot.verifiedbootstate` | Удаляет prop через `resetprop --delete`, если он есть в текущем property space. |
| Verity Mode | `ro.boot.veritymode` | Удаляет prop через `resetprop --delete`, если он есть в текущем property space. |
| Build Tags | `ro.build.tags` | Удаляет prop через `resetprop --delete`, если он есть в текущем property space. |
| Build Type | `ro.build.type` | Удаляет prop через `resetprop --delete`, если он есть в текущем property space. |

#### Delete Props

Все пункты ниже **удаляют выбранный prop** из текущего Android property space через `resetprop --delete`. Значение не спуфится и не заменяется. Если prop отсутствует, модуль пропускает его.

| Пункт в UI | Prop |
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

### 🎯 Автоматическое и ручное управление Target List
В автоматическом режиме **Target Box** формирует `/data/adb/Box-Brain/target.json` из управляемого списка приложений. Актуальный шаблон можно посмотреть в репозитории:

[targetList/target.json](https://github.com/flexgav/IntegrityBox-Ultimate/blob/main/targetList/target.json)

Модуль сначала пытается получить свежий список из репозитория. Если сеть недоступна, используется последний кэшированный список, а если кэша еще нет — встроенный список из ZIP модуля. Рабочий снимок хранится в `/data/adb/Box-Brain/target.json`; файлы бэкендов читает и записывает только API бэкенда.

В автоматический список входят:

* **Базовые Google-пакеты:** Google Play Services, Play Store, Google Services Framework и Google Wallet. Они записываются всегда.
* **Дополнительные Google-компоненты:** SafetyCore, Google Contact Keys, Google Pay India и похожие пакеты. Они добавляются только если установлены на устройстве.
* **Проверочные приложения:** Play Integrity, Key Attestation, Keybox Checker и похожие утилиты проверки.
* **Root/Xposed/native-детекторы:** приложения, которые проверяют root, Zygisk, Xposed, Magisk, списки приложений и низкоуровневые признаки окружения.
* **Дополнительные проверки:** приложения для проверки загрузчика, NFC, DRM и подписи APK.

Если TEE/backend помечен как сломанный или его состояние не удалось определить, модуль применяет принудительную настройку аттестации через актуальный Keybox.

Если вы хотите использовать собственный список приложений вместо автоматического шаблона:

1. Откройте WebUI -> **Настройка Tricky Store** -> **Target Box**.
2. Отключите **Автообновление Target List**, если хотите полностью контролировать Target List вручную.
3. Используйте импорт Target, чтобы выбрать JSON-файл Target List из IntegrityBox через встроенный проводник.
4. Для точечной настройки приложения включите его в Target Box, затем выберите профиль **Default**, **AOSP** или **Private** и режим **AUTO**, **GENERATE** или **LEAF**. Можно также выделить несколько приложений и применить общие настройки сразу ко всем.
5. Если **Автообновление Target List** снова включить, модуль применит свои правила и обновит отображение списка из актуального Target List.

> [!NOTE]
> В автоматическом режиме Target Box обновляется по расписанию через AutoPilot и при запуске Action, но ручной режим защищает ваш собственный Target List от перезаписи. Перед изменением файла модуль делает резервную копию, а состояние Target Box попадает в диагностический отчет.

---

## 📚 Встроенная база знаний
Больше не нужно искать каждую мелочь в огромном README. **IntegrityBox Ultimate включает интерактивную Базу Знаний прямо внутри WebUI.**

* 💡 **Руководства по модулям:** Нажмите на иконку лампочки в верхней части любой страницы модуля, чтобы узнать, что делает конкретный инструмент.
* ℹ️ **Описание функций:** Нажимайте кнопки `( i )` рядом с отдельными разделами, чтобы получить понятное описание переключателей и цветных значков статуса.
* 🤖 **AI Ассистент:** Офлайн AI-ассистент, встроенный в WebUI, готов ответить на распространенные вопросы и помочь в решении проблем.

---

## 🛠️ Быстрое решение проблем
> [!NOTE]
> Начинайте с простых действий: проверка Keybox, **Перезапустить сервисы**, при необходимости **Глубокая очистка GMS**, перезагрузка и повторная проверка Play Маркета. Расширенные режимы Target Box, Boot Hash Spoofer и Nuke используйте только когда понятно, какая именно проверка не проходит.

### 🧪 Play Integrity не проходит
1. Откройте WebUI -> **Инструментарий** -> **Проверка целостности**.
2. Проверьте **Active Keybox**. Это **две независимые оси**, и смотреть нужно обе: доступность — статус должен быть `ONLINE`, и актуальность — строка **Актуальность** должна показывать `АКТУАЛЕН`. Keybox может быть `ONLINE` и при этом `ОТОЗВАН`: он синхронизирован из облака, но забанен Google и вскоре перестанет проходить.
3. Если Keybox отсутствует, статус не `ONLINE` или он помечен как `ОТОЗВАН`, откройте WebUI -> **Центр Keybox** -> **Keybox Loader**. Сверху выберите **Слот 1** *(основной слот)*, нажмите **Обновить Кейбоксы**, дождитесь обновления списка, затем нажмите на свежий Keybox с бейджем `ПРОВАЙДЕР` и меткой `АКТУАЛЕН` и подтвердите загрузку в **Слот 1**. Если cloud-список не появился, проверьте интернет и доступ к GitHub. Если синхронизация все равно недоступна, положите свой XML Keybox в `/sdcard/Download` или `/sdcard/Documents`, нажмите кнопку обновления списка и загрузите найденный `ЛОКАЛЬНЫЙ` Keybox в **Слот 1**.
4. После обновления Keybox выполните WebUI -> **Play Integrity Fix** -> **GMS Tools** -> кнопка **Глубокая очистка GMS**, перезагрузите устройство и снова проверьте Play Маркет.

### 🛒 Play Маркет пишет "Устройство не сертифицировано"
1. Убедитесь, что Play Integrity уже проходит в **Проверке целостности**.
2. Включите скрытые параметры разработчика Play Маркета: откройте **Play Маркет** -> нажмите на аватар профиля справа сверху -> **Настройки** -> **О приложении** -> нажмите **Версия Play Маркета** 7 раз, пока не появится сообщение о включении режима разработчика.
3. Проверьте verdict целостности: вернитесь в **Настройки** Play Маркета -> **Общие** -> **Параметры разработчика** -> в блоке **Play Integrity** нажмите **Проверить целостность**.
4. В результате проверки устройство должно получать успешный verdict. Для обычной сертификации Play Маркета должен проходить **Device integrity** / `MEETS_DEVICE_INTEGRITY`; для Google Wallet и строгих банковских приложений обычно нужен успешный **Strong integrity** / `MEETS_STRONG_INTEGRITY`.
5. Отдельно проверьте статус сертификации: **Play Маркет** -> аватар профиля -> **Настройки** -> **О приложении** -> **Сертификация Play Защиты**. В норме там должно быть написано **Устройство сертифицировано**.
6. Если verdict целостности проходит, но Play Маркет все еще пишет **Устройство не сертифицировано**, откройте WebUI -> **Play Integrity Fix** -> **GMS Tools** -> кнопка **Глубокая очистка GMS**.
7. Перезагрузите устройство, откройте Play Маркет, войдите в аккаунт заново и подождите несколько минут. Затем повторите проверку целостности и сертификации по шагам выше.

### 💳 Google Wallet / GPay не работает
1. Сначала выполните встроенную проверку Play Integrity через Play Маркет: **Play Маркет** -> аватар профиля -> **Настройки** -> **Общие** -> **Параметры разработчика** -> **Play Integrity** -> **Проверить целостность**. Если раздела **Параметры разработчика** нет, включите его по инструкции из сценария выше.
2. Для Google Wallet обычно должен проходить **Strong integrity** / `MEETS_STRONG_INTEGRITY`. Если проходит только **Device integrity**, Play Маркет может быть сертифицирован, но Wallet все равно может отклонять оплату.
3. **Если вы только что меняли настройки спуфинга** (профиль, отпечаток, тумблеры **Advanced Spoofing**), начните с лёгкого шага: WebUI -> **Play Integrity Fix** -> **GMS Tools** -> кнопка **Перезапустить сервисы**. Она сбрасывает кэш вердикта DroidGuard, из-за которого Google Play Services продолжают отдавать Wallet **старый** результат проверки, хотя настройки уже изменились. Карты и аккаунт при этом не затрагиваются. Часто этого достаточно — попробуйте добавить карту сразу после.
4. Если **Strong integrity** не проходит, вернитесь к сценарию **Play Integrity не проходит**: проверьте **Active Keybox** (и статус `ONLINE`, и метку `АКТУАЛЕН`), обновите Keybox через **Keybox Loader**, затем выполните **Глубокая очистка GMS** и перезагрузитесь.
5. Если **Strong integrity** проходит, но Wallet все равно не работает, откройте WebUI -> **Play Integrity Fix** -> **GMS Tools** -> кнопка **Очистить данные Wallet**. Она стирает локальное состояние Wallet, запомнившее прошлую ошибку безопасности, и заодно сбрасывает кэш сервисов. Из аккаунта Google вас **не выкинет**, но карты, скорее всего, придётся добавить заново.
6. Перезагрузите устройство, откройте Google Wallet, снова добавьте карты при необходимости и проверьте бесконтактную оплату.

### 🏦 Банковское приложение видит root, Xposed или подозрительные приложения
1. Убедитесь, что установлен **HideMyApplist / HMA-OSS**, модуль HMA включен в LSPosed, а банковское приложение отмечено в области действия LSPosed для HMA.
2. Если профиль еще не применялся, откройте WebUI -> **Скрытие следов** -> **Применить шаблон HMA**. Это загрузит готовую конфигурацию IntegrityBox в HMA.
3. Откройте HMA и выберите нужное банковское приложение в разделе приложений, от которых нужно скрывать окружение.
4. Примените к нему шаблоны и пресеты **FlexGAV 5.5**: blacklist/template для скрываемых приложений, detector/root/sus-apps пресеты, а также пресеты настроек вроде accessibility/dev options/input method, если они доступны в вашем HMA.
5. Сохраните конфигурацию HMA, принудительно остановите банковское приложение или перезагрузите устройство, затем проверьте приложение снова.

### 🔐 Приложение не проходит аппаратную аттестацию / TEE
> [!IMPORTANT]
> Расширенные профили Target Box настраиваются для конкретного приложения. Не применяйте их ко всем приложениям сразу.

1. Сначала убедитесь, что **Active Keybox** в **Проверке целостности** имеет статус `ONLINE`.
2. Откройте WebUI -> **Настройка Tricky Store** -> **Target Box**.
3. Найдите нужное приложение по названию или package name.
4. Включите Target для приложения, затем для первой попытки выберите профиль **Default** и режим **AUTO**. Изменения сохраняются автоматически.
5. Принудительно остановите проблемное приложение или перезагрузите устройство, затем проверьте приложение снова.
6. Если **AUTO** не помог, попробуйте **LEAF** для этого же приложения. **GENERATE** используйте только как последний вариант, если понятно, что приложению не подходит обычная цепочка аттестации.

### 🔓 Приложение ругается на загрузчик, VBMeta или boot hash
1. Откройте WebUI -> **Обнаружение** -> **Boot Hash Spoofer**.
2. Нажмите **Получить реальный хэш**. Модуль сначала проверит аппаратные boot-источники: `/proc/cmdline`, device-tree и `/proc/bootconfig`.
3. Если boot-источники не вернули валидный hash, модуль автоматически попробует fallback через локальную Java Key Attestation операцию. Отдельно запускать ее не нужно.
4. Если поле заполнилось 64-символьным hash, нажмите **Apply**, затем **Reboot** и проверьте приложение снова.
5. Если появилось **Extraction Failed**, откройте **Центр помощи** -> **Экспорт отчета** и сохраните диагностический архив: в нем будут логи `boot_hash_extract.log` и `boot_hash_attestation.log`.
6. Если реальный hash получить не удалось, используйте **Magic Wand** только как запасной вариант: она генерирует валидный 64-символьный hash, но это не реальное значение устройства.
7. Если нужно откатить изменение, вернитесь в **Boot Hash Spoofer**, нажмите **Сбросить** и перезагрузите устройство.

### 📌 Нужен быстрый доступ к часто используемым инструментам
Зажмите нужную плитку WebUI примерно на 300 мс, чтобы закрепить ее в **Быстром доступе** на главной странице.

### 🆘 Ничего не помогло
Если после всех шагов не удалось добиться сертификации Google или стабильной работы нужных приложений, перейдите в Telegram-группу по ссылкам в шапке README и напишите в тему помощи и поддержки. Опишите модель устройства, прошивку, Root-менеджер, используемый backend аттестации, результат Play Integrity из Play Маркета и что уже пробовали сделать.

---

## 🧩 Исходники бинарных компонентов
JS, HTML, CSS, shell-скрипты и конфиги модуля уже лежат в ZIP в открытом виде. Отдельно публикуются только исходники компонентов, которые попадают в модуль как собранные бинарные файлы:

- `sources/zygisk/` — исходники и заметки по сборке native Zygisk-библиотек из `IntegrityBox-Ultimate-Clnt/zygisk/*.so`.
- `sources/dex/` — исходники Java-классов, из которых собирается `classes.dex` (PIF entry point, provider и keystore-хуки).
- `sources/boot-hash-attestation/` — исходники и заметки по сборке helper-файла `boot_attest.jar`.
- `sources/licenses/` — лицензии и notices для собранных компонентов.

---

## 🙏 Благодарности
Этот проект использует концепции и код из следующих open-source проектов:
- @ez-me за ezme-nodebug.
- @osm0sis за PlayIntegrityFork.
- **Команде LSPosed** за скрипт отложенного запуска сервиса Shamiko.
- **MeowDump** за оригинальную основу Integrity-Box.
- **Вам**, за использование этого модуля.
