#!/system/bin/sh
MODPATH="${0%/*}"
. $MODPATH/common_func.sh

boot="/data/adb/service.d"
placeholder="/data/adb/modules/playintegrityfix/webroot/common_scripts"
mkdir -p "/data/adb/Box-Brain/Integrity-Box-Logs"
mkdir -p "$boot"

# Grant perms 
if [ -f "$placeholder/autopilot.sh" ]; then
    chmod 755 "$placeholder/autopilot.sh"
fi

# Remove installation script if exists 
if [ -f "/data/adb/modules/playintegrityfix/customize.sh" ]; then
  rm -rf "/data/adb/modules/playintegrityfix/customize.sh"
fi

# Handle Vending-specific prop
if [ -f "/data/adb/Box-Brain/enablevending" ]; then
    set_simpleprop persist.sys.pixelprops.vending true
fi

if [ -f "/data/adb/Box-Brain/disablevending" ]; then
    set_simpleprop persist.sys.pixelprops.vending false
fi

# Handle GMS-specific props
if [ -f "/data/adb/Box-Brain/enablegms" ]; then
    set_resetprop persist.sys.pihooks.disable.gms_key_attestation_block false
    set_resetprop persist.sys.pihooks.disable.gms_props false
    set_simpleprop persist.sys.pihooks.disable 0
    set_simpleprop persist.sys.kihooks.disable 0
fi

if [ -f "/data/adb/Box-Brain/disablegms" ]; then
    set_resetprop persist.sys.pihooks.disable.gms_key_attestation_block true
    set_resetprop persist.sys.pihooks.disable.gms_props true
    set_simpleprop persist.sys.pihooks.disable 1
    set_simpleprop persist.sys.kihooks.disable 1
fi

if [ ! -f "$placeholder/target.sh" ]; then
  cat <<'EOF' > "$placeholder/target.sh"
#!/system/bin/sh
MODPATH="/data/adb/modules/playintegrityfix"
. $MODPATH/common_func.sh

TARGET_DIR="/data/adb/tricky_store"
TARGET="$TARGET_DIR/target.txt"
BACKUP="$TARGET.bak"
TMP="${TARGET}.new.$$"
success=0
made_backup=0
orig_selinux="$(getenforce 2>/dev/null || echo Permissive)"

mkdir -p "$TARGET_DIR" 2>/dev/null
if [ ! -f "$SKIP_FILE" ] && [ "$orig_selinux" = "Enforcing" ]; then
    setenforce 0
fi

[ -f "$TARGET" ] && mv -f "$TARGET" "$BACKUP" && made_backup=1 && log_step "BACKUP" "$BACKUP"

teeBroken="false"
TEE_STATUS="$TARGET_DIR/tee_status"
[ -f "$TEE_STATUS" ] && [ "$(grep -E '^teeBroken=' "$TEE_STATUS" | cut -d '=' -f2)" = "true" ] && teeBroken="true"

for pkg in com.android.vending com.google.android.gms com.google.android.gsf; do
    echo "$pkg" >> "$TMP"
done

cmd package list packages -3 2>/dev/null | cut -d ":" -f2 | while read -r pkg; do
    [ -z "$pkg" ] && continue
    grep -Fxq "$pkg" "$TMP" || echo "$pkg" >> "$TMP"
done

sed -i 's/^[[:space:]]*//;s/[[:space:]]*$//' "$TMP"
sort -u "$TMP" -o "$TMP"

BLACKLIST="/data/adb/Box-Brain/blacklist.txt"
if [ -s "$BLACKLIST" ]; then
    sed -i 's/^[[:space:]]*//;s/[[:space:]]*$//' "$BLACKLIST"
    grep -Fvxf "$BLACKLIST" "$TMP" > "${TMP}.filtered" || true
    mv -f "${TMP}.filtered" "$TMP"
    log_step "CLEANED" "Blacklisted Apps removed"
else
    log_step "SKIPPED" "Blacklist not configured"
fi

[ "$teeBroken" = "true" ] && sed -i 's/$/!/' "$TMP" && log_step "SUPPORT" "TEE Broken detected"

mv -f "$TMP" "$TARGET" && success=1 && log_step "UPDATED" "Target Packages updated"

if [ ! -f "$SKIP_FILE" ] && [ "$orig_selinux" = "Enforcing" ]; then
    setenforce 1
fi
exit 0
EOF
fi

chmod 755 "$placeholder/target.sh"

if [ ! -f "$placeholder/gms.sh" ]; then
  cat <<'EOF' > "$placeholder/gms.sh"
#!/system/bin/sh
MODPATH="/data/adb/modules/playintegrityfix"
. $MODPATH/common_func.sh

for proc in com.google.android.gms.unstable com.google.android.gms com.android.vending; do
  kill_process "$proc"
done

exit 0
EOF
fi

chmod 755 "$placeholder/gms.sh"

if [ ! -f "$placeholder/webui.sh" ]; then
  cat <<'EOF' > "$placeholder/webui.sh"
#!/system/bin/sh

if pm list packages | grep -q "io.github.a13e300.ksuwebui"; then
   am start -n "io.github.a13e300.ksuwebui/.WebUIActivity" -e id "playintegrityfix"
   exit 0
fi

if pm list packages | grep -q "com.dergoogler.mmrl.webuix"; then
   am start -n "com.dergoogler.mmrl.webuix/.ui.activity.webui.WebUIActivity" -e MOD_ID "playintegrityfix"
   exit 0
fi

am start -a android.intent.action.VIEW -d "https://github.com/5ec1cff/KsuWebUIStandalone/releases"
exit 0
EOF
fi

chmod 755 "$placeholder/webui.sh"

if [ ! -f "$placeholder/run_scan.sh" ]; then
  cat <<'EOF' > "$placeholder/run_scan.sh"
#!/system/bin/sh

SCRIPT="/data/adb/modules/playintegrityfix/webroot/common_scripts/scan_keybox.sh"

# Run detached
sh "$SCRIPT" > /data/adb/Box-Brain/Integrity-Box-Logs/keybox_runner.log 2>&1 &
EOF
fi

chmod 755 "$placeholder/run_scan.sh"

if [ ! -f "$placeholder/scan_keybox.sh" ]; then
  cat <<'EOF' > "$placeholder/scan_keybox.sh"
#!/system/bin/sh

OUT="/data/adb/Box-Brain/Integrity-Box-Logs/keybox_scan.log"
TARGET="/sdcard/Download"

rm -f "$OUT"

# epoch|size_bytes|full_path
find "$TARGET" -type f -iname "*.xml" -printf "%T@|%s|%p\n" 2>/dev/null \
  | sort -nr >> "$OUT"
EOF
fi

chmod 755 "$placeholder/scan_keybox.sh"

rm -rf "$placeholder/resetprop.sh"
if [ ! -f "$placeholder/resetprop.sh" ]; then
  cat <<'EOF' > "$placeholder/resetprop.sh"
#!/system/bin/sh
PKG="com.reveny.nativecheck"

su -c 'getprop | grep -E "pphooks|pihook|pixelprops|gms|pi" | sed -E "s/^\[(.*)\]:.*/\1/" | while IFS= read -r prop; do resetprop -p -d "$prop"; done'

# Check if package exists
if pm list packages | grep -q "$PKG"; then
    echo "Package $PKG found. Force stopping..."
    am force-stop "$PKG"
else
    echo "$PKG not installed."
fi
EOF
fi

chmod 755 "$placeholder/resetprop.sh"

cat <<'EOF' > "$placeholder/Report.sh"
#!/system/bin/sh

OUT_DIR="/sdcard"
OUT_FILE="$OUT_DIR/report.json"

# helpers
jescape() {
  echo "$1" | sed 's/\\/\\\\/g; s/"/\\"/g'
}

json_array() {
  awk '{printf "\"%s\",", $0}' | sed 's/,$//'
}

mask_fingerprint() {
    local FP

    # Get fingerprint from getprop first
    FP="$(getprop ro.build.fingerprint 2>/dev/null)"

    # Fallback to build.prop
    [ -z "$FP" ] && FP="$(grep -m1 '^ro.build.fingerprint=' /system/build.prop /vendor/build.prop 2>/dev/null | cut -d= -f2)"

    # Fallback to pseudo fingerprint
    [ -z "$FP" ] && FP="$(getprop ro.product.brand 2>/dev/null)/$(getprop ro.product.device 2>/dev/null)/$(getprop ro.build.version.release 2>/dev/null)"

    # Default if empty
    [ -z "$FP" ] && FP="unknown/unknown/unknown"

    # Remove leading/trailing slashes
    FP="${FP#/}"
    FP="${FP%/}"

    # Use parameter expansion instead of IFS read to avoid byte splitting
    local PREFIX="${FP%%/*}"      # first part
    local REST="${FP#*/}"
    PREFIX="${PREFIX}/"
    local SECOND="${REST%%/*}"    # second part
    PREFIX="${PREFIX}${SECOND}"

    # Last colon-separated part as tag
    local TAGS
    if [[ "$FP" == *:* ]]; then
        TAGS="${FP##*:}"
    else
        TAGS="unknown"
    fi

    echo "${PREFIX}/***MASKED***/${TAGS}"
}

# root implementation
ROOT_IMPL="none"
[ -d /data/adb/ksu/bin ] && ROOT_IMPL="kernelsu"
[ -d /data/adb/ap/bin ] && ROOT_IMPL="apatch"
[ -d /data/adb/magisk ] && ROOT_IMPL="magisk"

# fingerprint
FP_RAW="$(getprop ro.build.fingerprint)"
FP_MASKED="$(mask_fingerprint "$FP_RAW")"

# kernel
KERNEL_NAME="$(uname -s)"
KERNEL_RELEASE="$(uname -r)"
KERNEL_VERSION="$(uname -v)"
KERNEL_FULL="$(uname -a)"
PROC_VERSION="$(cat /proc/version 2>/dev/null)"

# system state
SELINUX="$(getenforce 2>/dev/null)"
VB_STATE="$(getprop ro.boot.verifiedbootstate)"
VBMETA_STATE="$(getprop ro.boot.vbmeta.device_state)"
FLASH_LOCKED="$(getprop ro.boot.flash.locked)"
SECURE="$(getprop ro.secure)"
DEBUGGABLE="$(getprop ro.debuggable)"
QEMU="$(getprop ro.kernel.qemu)"

# play services / store
GMS_DUMP="$(dumpsys package com.google.android.gms 2>/dev/null)"
GMS_VNAME="$(echo "$GMS_DUMP" | grep versionName | head -n1 | cut -d= -f2)"
GMS_VCODE="$(echo "$GMS_DUMP" | grep versionCode | head -n1 | cut -d= -f2 | cut -d' ' -f1)"

PLAY_DUMP="$(dumpsys package com.android.vending 2>/dev/null)"
PLAY_VNAME="$(echo "$PLAY_DUMP" | grep versionName | head -n1 | cut -d= -f2)"
PLAY_VCODE="$(echo "$PLAY_DUMP" | grep versionCode | head -n1 | cut -d= -f2 | cut -d' ' -f1)"

# user apps
pm list packages -3 | cut -d: -f2 > "$OUT_DIR/user_apps.tmp"

# PIF
PIF_FILE="/data/adb/modules/playintegrityfix/custom.pif.prop"

# default: empty object, formatted
PIF_JSON="{
    }"

if [ -f "$PIF_FILE" ]; then
  PIF_JSON="$(
    awk -F= '
      BEGIN {
        print "{"
        first=1
      }

      $1=="spoofBuild" ||
      $1=="spoofProps" ||
      $1=="spoofProvider" ||
      $1=="spoofSignature" ||
      $1=="spoofVendingFinger" ||
      $1=="spoofPixel" ||
      $1=="spoofVendingSdk" {

        if (!first) printf ",\n"
        first=0
        printf "        \"%s\": \"%s\"", $1, $2
      }

      END {
        if (!first) print ""
        print "    }"
      }
    ' "$PIF_FILE"
  )"
fi

# magisk modules
MODULES_JSON=""
for m in /data/adb/modules/*; do
  PROP="$m/module.prop"
  [ -f "$PROP" ] || continue

  ID="$(grep '^id=' "$PROP" | cut -d= -f2)"
  NAME="$(grep '^name=' "$PROP" | cut -d= -f2)"
  VERSION="$(grep '^version=' "$PROP" | cut -d= -f2)"
  AUTHOR="$(grep '^author=' "$PROP" | cut -d= -f2)"

  MODULES_JSON="${MODULES_JSON}{
    \"id\":\"$(jescape "$ID")\",
    \"name\":\"$(jescape "$NAME")\",
    \"version\":\"$(jescape "$VERSION")\",
    \"author\":\"$(jescape "$AUTHOR")\"
  },"
done

MODULES_JSON="[${MODULES_JSON%,}]"

# JSON
{
echo "{"
echo "  \"timestamp\": \"$(date -Iseconds)\","

echo "  \"root\": {"
echo "    \"implementation\": \"$(jescape "$ROOT_IMPL")\""
echo "  },"

echo "  \"build\": {"
echo "    \"fingerprint\": \"$(jescape "$FP_MASKED")\","
echo "    \"tags\": \"$(jescape "$(getprop ro.build.tags)")\","
echo "    \"type\": \"$(jescape "$(getprop ro.build.type)")\""
echo "  },"

echo "  \"device\": {"
echo "    \"brand\": \"$(jescape "$(getprop ro.product.brand)")\","
echo "    \"manufacturer\": \"$(jescape "$(getprop ro.product.manufacturer)")\","
echo "    \"model\": \"$(jescape "$(getprop ro.product.model)")\","
echo "    \"device\": \"$(jescape "$(getprop ro.product.device)")\""
echo "  },"

echo "  \"android\": {"
echo "    \"version\": \"$(jescape "$(getprop ro.build.version.release)")\","
echo "    \"sdk\": \"$(jescape "$(getprop ro.build.version.sdk)")\","
echo "    \"security_patch\": \"$(jescape "$(getprop ro.build.version.security_patch)")\""
echo "  },"

echo "  \"kernel\": {"
echo "    \"name\": \"$(jescape "$KERNEL_NAME")\","
echo "    \"release\": \"$(jescape "$KERNEL_RELEASE")\","
echo "    \"version\": \"$(jescape "$KERNEL_VERSION")\","
echo "    \"full\": \"$(jescape "$KERNEL_FULL")\","
echo "    \"proc_version\": \"$(jescape "$PROC_VERSION")\""
echo "  },"

echo "  \"system_state\": {"
echo "    \"selinux\": \"$(jescape "$SELINUX")\","
echo "    \"verified_boot\": \"$(jescape "$VB_STATE")\","
echo "    \"vbmeta_state\": \"$(jescape "$VBMETA_STATE")\","
echo "    \"flash_locked\": \"$(jescape "$FLASH_LOCKED")\","
echo "    \"secure\": \"$(jescape "$SECURE")\","
echo "    \"debuggable\": \"$(jescape "$DEBUGGABLE")\","
echo "    \"kernel_qemu\": \"$(jescape "$QEMU")\""
echo "  },"

echo "  \"play\": {"
echo "    \"services\": {"
echo "      \"version_name\": \"$(jescape "$GMS_VNAME")\","
echo "      \"version_code\": \"$(jescape "$GMS_VCODE")\""
echo "    },"
echo "    \"store\": {"
echo "      \"version_name\": \"$(jescape "$PLAY_VNAME")\","
echo "      \"version_code\": \"$(jescape "$PLAY_VCODE")\""
echo "    }"
echo "  },"

echo "  \"playintegrityfix\": $PIF_JSON,"

echo "  \"modules\": $MODULES_JSON,"

echo "  \"user_apps\": [$(cat "$OUT_DIR/user_apps.tmp" | json_array)]"

echo "}"
} > "$OUT_FILE"

rm -f "$OUT_DIR/user_apps.tmp"

echo
echo "======================================"
echo " Report generated successfully"
echo " $OUT_FILE"
echo "======================================"
EOF

chmod 755 "$placeholder/Report.sh"

cat <<'EOF' > "$boot/.box_cleanup.sh"
#!/system/bin/sh

# This script cleans up leftover files after module ID change.
#
# IntegrityBox and PIF now replace each other to avoid conflicts.
# If a user flashes PIF over IntegrityBox, leftover IntegrityBox files may remain.
# This script deletes those leftover files and folders, and then deletes itself. 
# It only runs if IntegrityBox is not installed

PROP_FILE="/data/adb/modules/playintegrityfix/module.prop"
REQUIRED_LINE="support=https://t.me/MeowDump"
LOG_DIR="/data/adb/Box-Brain"

SERVICE_FILES="
/data/adb/service.d/shamiko.sh
/data/adb/service.d/prop.sh
/data/adb/service.d/hash.sh
/data/adb/service.d/lineage.sh
/data/adb/service.d/package.sh
"

# Check if the prop file exists and contains the required line
if [ ! -f "$PROP_FILE" ] || ! grep -Fq "$REQUIRED_LINE" "$PROP_FILE"; then
    # Delete leftover files if they exist
    for file in $SERVICE_FILES; do
        [ -e "$file" ] && rm -rf "$file"
    done

    # Delete Box-Brain folder if it exists
    [ -d "$LOG_DIR" ] && rm -rf "$LOG_DIR"

    # Delete this script itself
    rm -f "$0"
fi
EOF

chmod 755 "$boot/.box_cleanup.sh"

cat <<'EOF' > "$placeholder/force_override.sh"
#!/system/bin/sh
L=/data/adb/Box-Brain/Integrity-Box-Logs/ForceSpoof.log
mkdir -p ${L%/*}
getprop | grep -i lineage | while read l; do
p=${l#*[}; p=${p%%]*}
echo "$(date '+%F %T') DEL $p" >> $L
resetprop --delete "$p"
done
EOF

chmod 755 "$placeholder/force_override.sh"

cat <<'EOF' > "$placeholder/override_lineage.sh"
#!/system/bin/sh

OVERRIDE="/data/adb/modules/playintegrityfix/webroot/common_scripts/force_override.sh"

# Stop when safe mode is enabled 
if [ -f "/data/adb/Box-Brain/safemode" ]; then
    echo " Permission denied by Safe Mode"
    exit 1
fi

# check prop
echo " Checking for Lineage Props"
getprop | grep -i lineage
echo " "

# config
PROP_FILE="/data/adb/modules/playintegrityfix/system.prop"
LOG_FILE="/data/adb/Box-Brain/Integrity-Box-Logs/prop_debug.log"

# init logging
echo "[prop spoof debug log]" > "$LOG_FILE"
echo "[INFO] Script started at $(date)" >> "$LOG_FILE"

# check file
if [ ! -f "$PROP_FILE" ]; then
    echo "[ERROR] Prop file not found: $PROP_FILE" >> "$LOG_FILE"
    exit 1
fi

if [ ! -r "$PROP_FILE" ]; then
    echo "[ERROR] Cannot read prop file: $PROP_FILE" >> "$LOG_FILE"
    exit 1
fi

# process lines
while IFS= read -r line || [ -n "$line" ]; do
    # Strip [brackets] if present
    clean_line=$(echo "$line" | sed -E 's/^\[(.*)\]=\[(.*)\]$/\1=\2/')

    # Skip empty or comment lines
    if [ -z "$clean_line" ] || echo "$clean_line" | grep -qE '^#'; then
        echo "[SKIP] Empty or comment: $line" >> "$LOG_FILE"
        continue
    fi

    key=$(echo "$clean_line" | cut -d '=' -f1)
    value=$(echo "$clean_line" | cut -d '=' -f2-)

    # Sanity check
    if [ -z "$key" ] || [ -z "$value" ]; then
        echo "[SKIP] Malformed line: $line" >> "$LOG_FILE"
        continue
    fi

    case "$key" in
        init.svc.*|ro.boottime.*)
            echo "[SKIP] Dynamic prop (not changeable): $key" >> "$LOG_FILE"
            continue
            ;;
        ro.crypto.state)
            echo "[SKIP] Encryption state spoof skipped: $key" >> "$LOG_FILE"
            continue
            ;;
        *)
            # Attempt to override using resetprop
            resetprop "$key" "$value"
            # Check if the change was successful
            actual_value=$(getprop "$key")
            if [ "$actual_value" = "$value" ]; then
                echo "[OK] Overridden: $key=$value" >> "$LOG_FILE"
            else
                echo "[WARN] Failed to override: $key. Current value: $actual_value" >> "$LOG_FILE"
            fi
            ;;
    esac
done < "$PROP_FILE"

if [ -f "$OVERRIDE" ]; then
    sh "$OVERRIDE"
fi

echo "[INFO] Script completed at $(date)" >> "$LOG_FILE"
echo "•••••••••••••••••••••=" >> "$LOG_FILE"
echo " "
echo " "
exit 0
EOF

chmod 755 "$placeholder/override_lineage.sh"

touch "$placeholder/kill"
touch "$placeholder/aosp"
touch "$placeholder/patch"
touch "$placeholder/xml"
touch "$placeholder/tee"
touch "$placeholder/user"
touch "$placeholder/hma"
touch "$placeholder/ulock"
touch "$placeholder/stop"
touch "$placeholder/start"
touch "$placeholder/nogms"
touch "$placeholder/lineage"
touch "$placeholder/selinux"
touch "$placeholder/hide"
touch "$placeholder/resetprop"
touch "$placeholder/faq"
touch "$placeholder/nuke"
touch "$placeholder/zygisknext"
touch "$placeholder/yesgms"

cat <<'EOF' > "$placeholder/hma.sh"
#!/system/bin/sh

SRC_CONFIG="/data/adb/modules/playintegrityfix/hidemyapplist/config.json"

APP_PATHS="
/data/user/0/org.frknkrc44.hma_oss
/data/user/0/com.google.android.hmal
/data/user/0/com.tsng.hidemyapplist
"

LOG_DIR="/data/adb/Box-Brain/Integrity-Box-Logs"
LOG_FILE="$LOG_DIR/hma.log"

BACKUP_DIR="/data/adb/HMA"
DATE_TAG="$(date '+%Y-%m-%d_%H-%M-%S')"
ANTISELINUX="/data/adb/Box-Brain/antiselinux"

ORIG_SELINUX=""
SELINUX_CHANGED=0
PKG_NAME=""
ACTIVITY=""

mkdir -p "$LOG_DIR"
mkdir -p "$BACKUP_DIR"

log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" >> "$LOG_FILE"
}

get_selinux_mode() {
    if command -v getenforce >/dev/null 2>&1; then
        getenforce
    else
        echo "Unknown"
    fi
}

set_selinux_permissive() {
    if command -v setenforce >/dev/null 2>&1; then
        setenforce 0
    fi
}

restore_selinux() {
    if [ "$SELINUX_CHANGED" -eq 1 ] && [ "$ORIG_SELINUX" = "Enforcing" ]; then
        log "Restoring SELinux to Enforcing"
        setenforce 1
    fi
}

log "•••••••••••••= HMA config sync started •••••••••••••"

if [ ! -f "$SRC_CONFIG" ]; then
    log "ERROR: Source config not found: $SRC_CONFIG"
    exit 1
fi

log "Source config found: $SRC_CONFIG"

HMA_INSTALLED=0
if pm list packages | grep -q "^package:org.frknkrc44.hma_oss$"; then
    HMA_INSTALLED=1
    PKG_NAME="org.frknkrc44.hma_oss"
    ACTIVITY="org.frknkrc44.hma_oss/.ui.activity.MainActivity"
elif pm list packages | grep -q "^package:com.google.android.hmal$"; then
    HMA_INSTALLED=1
    PKG_NAME="com.google.android.hmal"
    ACTIVITY="com.google.android.hmal/icu.nullptr.hidemyapplist.ui.activity.MainActivity"
elif pm list packages | grep -q "^package:com.tsng.hidemyapplist$"; then
    HMA_INSTALLED=1
    PKG_NAME="com.tsng.hidemyapplist"
    ACTIVITY="com.tsng.hidemyapplist/icu.nullptr.hidemyapplist.ui.activity.MainActivity"
fi

if [ "$HMA_INSTALLED" -eq 0 ]; then
    log "No supported HMA app installed"
    log "Opening download page..."
    nohup am start -a android.intent.action.VIEW -d "https://github.com/frknkrc44/HMA-OSS/releases" > /dev/null 2>&1 &
    exit 1
fi

log "Resolved package name: $PKG_NAME"

TARGET_APP=""
for APP in $APP_PATHS; do
    if [ -d "$APP" ]; then
        TARGET_APP="$APP"
        log "Found installed app data path: $APP"
        break
    fi
done

if [ -z "$TARGET_APP" ]; then
    log "App installed but data directory not found"
    log "Launching app to create data directory..."
    log "Launching activity: $ACTIVITY"
    am start --user 0 -a android.intent.action.VIEW -n "$ACTIVITY" >>"$LOG_FILE" 2>&1
    sleep 5
    am force-stop "$PKG_NAME" >>"$LOG_FILE" 2>&1
    log "App launched and stopped, checking data directory..."
    
    for APP in $APP_PATHS; do
        if [ -d "$APP" ]; then
            TARGET_APP="$APP"
            log "Data directory now available: $APP"
            break
        fi
    done
    
    if [ -z "$TARGET_APP" ]; then
        log "ERROR: Data directory still not created after launch"
        exit 1
    fi
fi

TARGET_FILES="$TARGET_APP/files"
TARGET_CONFIG="$TARGET_FILES/config.json"

if [ ! -d "$TARGET_FILES" ]; then
    log "/files directory missing, creating: $TARGET_FILES"
    mkdir -p "$TARGET_FILES" || {
        log "ERROR: Failed to create $TARGET_FILES"
        exit 1
    }
fi

if [ -f "$TARGET_CONFIG" ]; then
    BACKUP_NAME="config_${DATE_TAG}.json"
    log "Existing config found, moving to $BACKUP_DIR/$BACKUP_NAME"
    mv "$TARGET_CONFIG" "$BACKUP_DIR/$BACKUP_NAME" || {
        log "ERROR: Failed to move existing config"
        exit 1
    }
fi

log "Copying new config to $TARGET_CONFIG"
cp "$SRC_CONFIG" "$TARGET_CONFIG" || {
    log "ERROR: Failed to copy new config"
    exit 1
}

chmod 666 "$TARGET_CONFIG"
chown system:system "$TARGET_CONFIG" 2>/dev/null

if [ ! -f "$ANTISELINUX" ]; then
    ORIG_SELINUX="$(get_selinux_mode)"
    log "Current SELinux mode: $ORIG_SELINUX"
    if [ "$ORIG_SELINUX" = "Enforcing" ]; then
        log "Switching SELinux to Permissive temporarily"
        set_selinux_permissive
        SELINUX_CHANGED=1
        sleep 0.5
    fi
else
    log "antiselinux flag found, skipping SELinux mode change"
fi

log "Force stopping app: $PKG_NAME"
am force-stop "$PKG_NAME" >>"$LOG_FILE" 2>&1
sleep 1
log "Launching activity: $ACTIVITY"
am start --user 0 -a android.intent.action.VIEW -n "$ACTIVITY" >>"$LOG_FILE" 2>&1
if [ $? -eq 0 ]; then
    log "App launched successfully"
else
    log "ERROR: Failed to launch app"
fi

restore_selinux
log "Config copy completed successfully"
log "••••••••••••• Finished •••••••••••••"
log
log
exit 0
EOF

chmod 777 "$placeholder/hma.sh"

cat <<'EOF' > "$boot/package.sh"
#!/system/bin/sh

# Check if required module folders exist
# These modules add system app package names to target.txt which ruins keybox & increases battery drain
MODULE1="/data/adb/modules/.TA_utl"
MODULE2="/data/adb/modules/tsupport-advance"
MODULE3="/data/adb/modules/Yurikey"
MODULE4="/data/adb/modules/tricky_store/webroot"

if [ ! -d "$MODULE1" ] && [ ! -d "$MODULE2" ] && [ ! -d "$MODULE3" ] && [ ! -d "$MODULE4" ]; then
    exit 0
fi

# Paths
IGNORE_FLAG="/data/adb/Box-Brain/ignore"
TARGET_FILE="/data/adb/tricky_store/target.txt"
SCRIPT="/data/adb/modules/playintegrityfix/webroot/common_scripts/target.sh"
LOG_FILE="/data/adb/Box-Brain/Integrity-Box-Logs/target.log"

# Create log directory if needed
mkdir -p "$(dirname "$LOG_FILE")"

# Log function
log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" >> "$LOG_FILE"
}

# Check ignore flag
if [ -f "$IGNORE_FLAG" ]; then
    log "Ignore flag found, exiting"
    exit 0
fi

# Function to check and execute
execute_if_needed() {
    if [ -f "$TARGET_FILE" ]; then
        line_count=$(wc -l < "$TARGET_FILE")
        log "Target.txt has $line_count packages"
        if [ "$line_count" -gt 50 ]; then
            log "Line count exceeds 50, executing cleanup script"
            if [ -f "$SCRIPT" ]; then
                sh "$SCRIPT"
                log "Script executed with exit code $?"
            else
                log "Script not found: $SCRIPT"
            fi
        fi
    else
        log "Target file not found: $TARGET_FILE"
    fi
}

# Initial check
log "••• Service started •••"
execute_if_needed

# Monitor in background
while true; do
    sleep 30
    if [ -f "$IGNORE_FLAG" ]; then
        log "Ignore flag detected during monitoring, stopping"
        exit 0
    fi
    execute_if_needed
done
EOF

chmod 777 "$boot/package.sh"

cat <<'EOF' > "$boot/lineage.sh"
#!/system/bin/sh

MODPATH="/data/adb/modules/playintegrityfix"
. $MODPATH/common_func.sh

# Module path and file references
LOG_DIR="/data/adb/Box-Brain/Integrity-Box-Logs"
PROP="/data/adb/modules/playintegrityfix/system.prop"

note() {
    TS="$(date '+%Y-%m-%d %H:%M:%S')"
    mkdir -p "$LOG_DIR" 2>/dev/null
    printf "%s | %s\n" "$TS" "$1" >> "$LOG_DIR/Lineage.log"
}

# Abort the script & delete flags web safe mode is active 
if [ -f "/data/adb/Box-Brain/safemode" ]; then
    note "$(date '+%Y-%m-%d %H:%M:%S') : Safemode active, script aborted." >> "/data/adb/Box-Brain/Integrity-Box-Logs/safemode.log"
    rm -rf "/data/adb/Box-Brain/NoLineageProp"
    rm -rf "/data/adb/Box-Brain/nodebug"
    rm -rf "/data/adb/Box-Brain/tag"
    exit 1
fi

if [ -f "/data/adb/modules/playintegrityfix/disable" ]; then
    rm -rf "/data/adb/modules/playintegrityfix/system.prop"
    exit 0
fi

# Module install path
export MODPATH="/data/adb/modules/playintegrityfix"

NO_LINEAGE_FLAG="/data/adb/Box-Brain/NoLineageProp"
NODEBUG_FLAG="/data/adb/Box-Brain/nodebug"
TAG_FLAG="/data/adb/Box-Brain/tag"

TMP_PROP="$MODPATH/tmp.prop"
SYSTEM_PROP="$MODPATH/system.prop"
> "$TMP_PROP" # clear old temp file

# Build summary of active flags
FLAGS_ACTIVE=""
[ -f "$NO_LINEAGE_FLAG" ] && FLAGS_ACTIVE="$FLAGS_ACTIVE NoLineageProp"
[ -f "$NODEBUG_FLAG" ] && FLAGS_ACTIVE="$FLAGS_ACTIVE nodebug"
[ -f "$TAG_FLAG" ] && FLAGS_ACTIVE="$FLAGS_ACTIVE tag"

if [ -n "$FLAGS_ACTIVE" ]; then
    note "Prop sanitization flags active: $FLAGS_ACTIVE"
    note "Preparing temporary prop file..."
    getprop | grep "userdebug" >> "$TMP_PROP"
    getprop | grep "test-keys" >> "$TMP_PROP"
    getprop | grep "lineage_" >> "$TMP_PROP"

    # Basic cleanup
    sed -i 's///g' "$TMP_PROP"
    sed -i 's/: /=/g' "$TMP_PROP"
else
    note "No prop sanitization flags found. Skipping."
fi

# LineageOS cleanup
if [ -f "$NO_LINEAGE_FLAG" ]; then
    note "NoLineageProp flag detected. Deleting LineageOS props..."
    for prop in \
        ro.lineage.build.version \
        ro.lineage.build.version.plat.rev \
        ro.lineage.build.version.plat.sdk \
        ro.lineage.device \
        ro.lineage.display.version \
        ro.lineage.releasetype \
        ro.lineage.version \
        ro.lineagelegal.url; do
        resetprop --delete "$prop"
    done
    sed -i 's/lineage_//g' "$TMP_PROP"
    note "LineageOS props sanitized."
fi

# userdebug to user
if [ -f "$NODEBUG_FLAG" ]; then
    if grep -q "userdebug" "$TMP_PROP"; then
        sed -i 's/userdebug/user/g' "$TMP_PROP"
    fi
    note "userdebug to user sanitization applied."
fi

# test-keys to release-keys
if [ -f "$TAG_FLAG" ]; then
    if grep -q "test-keys" "$TMP_PROP"; then
        sed -i 's/test-keys/release-keys/g' "$TMP_PROP"
    fi
    note "test-keys to release-keys sanitization applied."
fi

# Finalize system.prop
if [ -s "$TMP_PROP" ]; then
    note "Sorting and creating final system.prop..."
    sort -u "$TMP_PROP" > "$SYSTEM_PROP"
    rm -f "$TMP_PROP"
    note "system.prop created at $SYSTEM_PROP."

    note "Waiting 30 seconds before applying props..."
    sleep 30

    note "Applying props via resetprop..."
    resetprop -n --file "$SYSTEM_PROP"
    note "Prop sanitization applied from system.prop"
fi

# Explicit fingerprint sanitization
if [ -f "$NODEBUG_FLAG" ] || [ -f "$TAG_FLAG" ]; then
    fp=$(getprop ro.build.fingerprint)
    fp_clean="$fp"

    [ -f "$NODEBUG_FLAG" ] && fp_clean=${fp_clean/userdebug/user}
    [ -f "$TAG_FLAG" ] && {
        fp_clean=${fp_clean/test-keys/release-keys}
        fp_clean=${fp_clean/dev-keys/release-keys}
    }

    if [ "$fp" != "$fp_clean" ]; then
        resetprop ro.build.fingerprint "$fp_clean"
        [ -f "$NODEBUG_FLAG" ] && resetprop ro.build.type "user"
        [ -f "$TAG_FLAG" ] && resetprop ro.build.tags "release-keys"
        note "Fingerprint sanitized to $fp_clean"
    else
        note "Fingerprint already clean. No changes applied."
    fi
fi
EOF

chmod 777 "$boot/lineage.sh"

cat <<'EOF' > "$boot/hash.sh"
#!/system/bin/sh

HASH_FILE="/data/adb/Box-Brain/hash.txt"
LOG_DIR="/data/adb/Box-Brain/Integrity-Box-Logs"
LOG_FILE="$LOG_DIR/vbmeta.log"

mkdir -p "$LOG_DIR"

log() {
  echo "$(date '+%Y-%m-%d %H:%M:%S') | $1" >> "$LOG_FILE"
}

# Stop when safe mode is enabled 
if [ -f "/data/adb/Box-Brain/safemode" ]; then
    log " Permission denied by Safe Mode"
    exit 1
fi

log " "
log "Script started"

# Find resetprop
RESETPROP=""
for RP in \
  /sbin/resetprop \
  /system/bin/resetprop \
  /system/xbin/resetprop \
  /data/adb/magisk/resetprop \
  /data/adb/ksu/bin/resetprop \
  $(command -v resetprop 2>/dev/null)
do
  if [ -x "$RP" ]; then
    RESETPROP="$RP"
    break
  fi
done

if [ -z "$RESETPROP" ]; then
  log "ERROR: resetprop binary not found. Exiting."
  exit 0
fi

log "Using resetprop: $RESETPROP"

# Always set static default props
"$RESETPROP" ro.boot.vbmeta.size "4096"
"$RESETPROP" ro.boot.vbmeta.hash_alg "sha256"
"$RESETPROP" ro.boot.vbmeta.avb_version "2.0"
"$RESETPROP" ro.boot.vbmeta.device_state "locked"
log "Set static VBMeta props: size=4096, hash_alg=sha256, avb_version=2.0, device_state=locked"

# Handle hash
if [ ! -s "$HASH_FILE" ]; then
  log "Hash file missing or empty : clearing vbmeta.digest"
  "$RESETPROP" --delete ro.boot.vbmeta.digest
  exit 0
fi

# Extract hash
DIGEST=$(tr -cd '0-9a-fA-F' < "$HASH_FILE")

if [ -z "$DIGEST" ]; then
  log "Hash file contained no valid hex. Clearing vbmeta.digest."
  "$RESETPROP" --delete ro.boot.vbmeta.digest
  exit 0
fi

if [ "${#DIGEST}" -ne 64 ]; then
  log "Invalid hash length (${#DIGEST}). Expected 64 (SHA-256). Clearing vbmeta.digest."
  "$RESETPROP" --delete ro.boot.vbmeta.digest
  exit 0
fi

# Set digest if valid
"$RESETPROP" ro.boot.vbmeta.digest "$DIGEST"
log "Set ro.boot.vbmeta.digest = $DIGEST"
log " "

exit 0
EOF

chmod 777 "$boot/hash.sh"

#if [ ! -f "$boot/prop.sh" ]; then
cat <<'EOF' > "$boot/prop.sh"
#!/system/bin/sh

# CONFIG
PATCH_DATE="2026-04-01"
FILE_PATH="/data/adb/tricky_store/security_patch.txt"
SKIP_FILE="/data/adb/Box-Brain/skip"
LOG_DIR="/data/adb/Box-Brain/Integrity-Box-Logs"
LOG_FILE="$LOG_DIR/prop_patch.log"

writelog() {
    TS="$(date '+%Y-%m-%d %H:%M:%S')"
    mkdir -p "$LOG_DIR" 2>/dev/null
    printf "%s | %s\n" "$TS" "$1" >> "$LOG_FILE"
}

abort() {
    writelog "ERROR | $1"
    exit 1
}

# SAFE MODE CHECK
if [ -f "/data/adb/Box-Brain/safemode" ]; then
    echo "$(date '+%Y-%m-%d %H:%M:%S') : Safemode active, script aborted." \
        >> "/data/adb/Box-Brain/Integrity-Box-Logs/safemode.log"
    exit 1
fi

# RESETPROP CHECK
if ! command -v resetprop >/dev/null 2>&1; then
    abort "resetprop not found, cannot continue"
fi

# PROP SET FUNCTION
setprop_safe() {
    PROP=$1
    VALUE=$2
    CURRENT=$(getprop "$PROP")

    if [ "$CURRENT" = "$VALUE" ]; then
        writelog "✔ $PROP already set to $VALUE"
        return
    fi

    if resetprop "$PROP" "$VALUE"; then
        writelog "✔ Set $PROP to $VALUE (was: $CURRENT)"
    else
        writelog "❌ Failed to set $PROP (current: $CURRENT)"
    fi
}

# START LOG
writelog "•••••• Starting Security Patch Override ••••••"

# SAVE PATCH DATE
mkdir -p "/data/adb/tricky_store"
echo "all=$PATCH_DATE" > "$FILE_PATH" 2>>"$LOG_FILE"

# APPLY SYSTEM+VENDOR SECURITY PATCH
if [ -f "$SKIP_FILE" ]; then
    writelog "⚠ Sensitive device detected, skipping ro.vendor.build.security_patch"
else
    setprop_safe ro.vendor.build.security_patch "$PATCH_DATE"
    setprop_safe ro.build.version.security_patch "$PATCH_DATE"
fi

# FINAL VERIFICATION
BUILD_VAL=$(getprop ro.build.version.security_patch)
VENDOR_VAL=$(getprop ro.vendor.build.security_patch)

if [ -f "$SKIP_FILE" ]; then
    writelog "⚠ Sensitive device detected, Vendor patch override intentionally skipped"
else
    writelog "Vendor Patch Applied: $VENDOR_VAL"
    writelog "System Patch Applied: $BUILD_VAL"
fi

writelog "•••••• Script Finished Successfully ••••••"
exit 0
EOF
#fi

chmod 777 "$boot/prop.sh"

##########################################
# adapted from Shamiko (service.sh) by @LSPosed
# source: https://github.com/LSPosed/LSPosed.github.io/releases
##########################################

if [ ! -f "/data/adb/modules/zygisk_shamiko/module.prop" ]; then
   cat <<'EOF' > "$boot/shamiko.sh"
#!/system/bin/sh

# Stop when safe mode is enabled 
if [ -f "/data/adb/Box-Brain/safemode" ]; then
    echo " Permission denied by Safe Mode"
    exit 1
fi

check_reset_prop() {
  local NAME=$1
  local EXPECTED=$2
  local VALUE=$(resetprop $NAME)
  [ -z $VALUE ] || [ $VALUE = $EXPECTED ] || resetprop -n $NAME $EXPECTED
}

contains_reset_prop() {
  local NAME=$1
  local CONTAINS=$2
  local NEWVAL=$3
  [[ "$(resetprop $NAME)" = *"$CONTAINS"* ]] && resetprop -n $NAME $NEWVAL
}

resetprop -w sys.boot_completed 0

check_reset_prop "ro.boot.vbmeta.device_state" "locked"
check_reset_prop "ro.boot.verifiedbootstate" "green"
check_reset_prop "ro.boot.flash.locked" "1"
check_reset_prop "ro.boot.veritymode" "enforcing"
check_reset_prop "ro.boot.warranty_bit" "0"
check_reset_prop "ro.warranty_bit" "0"
check_reset_prop "ro.debuggable" "0"
check_reset_prop "ro.force.debuggable" "0"
check_reset_prop "ro.secure" "1"
check_reset_prop "ro.adb.secure" "1"
check_reset_prop "ro.build.type" "user"
check_reset_prop "ro.build.tags" "release-keys"
check_reset_prop "ro.vendor.boot.warranty_bit" "0"
check_reset_prop "ro.vendor.warranty_bit" "0"
check_reset_prop "vendor.boot.vbmeta.device_state" "locked"
check_reset_prop "vendor.boot.verifiedbootstate" "green"
check_reset_prop "sys.oem_unlock_allowed" "0"

# MIUI specific
check_reset_prop "ro.secureboot.lockstate" "locked"

# Realme specific
check_reset_prop "ro.boot.realmebootstate" "green"
check_reset_prop "ro.boot.realme.lockstate" "1"

# Hide that we booted from recovery when magisk is in recovery mode
contains_reset_prop "ro.bootmode" "recovery" "unknown"
contains_reset_prop "ro.boot.bootmode" "recovery" "unknown"
contains_reset_prop "vendor.boot.bootmode" "recovery" "unknown"
EOF
fi

chmod 777 "$boot/shamiko.sh"

##########################################
# adapted from Play Integrity Fork by @osm0sis
# source: https://github.com/osm0sis/PlayIntegrityFork
# license: GPL-3.0
##########################################

# First check if Magisk directory exists
if [ -d "/data/adb/magisk" ]; then
    echo "Magisk detected."

    if [ -d "$MODPATH/zygisk" ]; then
        # Remove Play Services and Play Store from Magisk DenyList when set to Enforce in normal mode
        if magisk --denylist status; then
            magisk --denylist rm com.google.android.gms
            magisk --denylist rm com.android.vending
        fi

        # Run common tasks for installation and boot-time
        . "$MODPATH/common_setup.sh"
    else
        # Add Play Services DroidGuard and Play Store processes to Magisk DenyList for better results in scripts-only mode
        magisk --denylist add com.google.android.gms com.google.android.gms.unstable
        magisk --denylist add com.android.vending
    fi

else
    echo "Skipped denylist, Bro's not using Magisk"
fi

# Conditional early sensitive properties

# Samsung
resetprop_if_diff ro.boot.warranty_bit 0
resetprop_if_diff ro.vendor.boot.warranty_bit 0
resetprop_if_diff ro.vendor.warranty_bit 0
resetprop_if_diff ro.warranty_bit 0

# Realme
resetprop_if_diff ro.boot.realmebootstate green

# OnePlus
resetprop_if_diff ro.is_ever_orange 0

# Microsoft
for PROP in $(resetprop | grep -oE 'ro.*.build.tags'); do
    resetprop_if_diff $PROP release-keys
done

# Other
for PROP in $(resetprop | grep -oE 'ro.*.build.type'); do
    resetprop_if_diff $PROP user
done
resetprop_if_diff ro.adb.secure 1
if ! $SKIPDELPROP; then
    delprop_if_exist ro.boot.verifiedbooterror
    delprop_if_exist ro.boot.verifyerrorpart
fi
resetprop_if_diff ro.boot.veritymode.managed yes
resetprop_if_diff ro.debuggable 0
resetprop_if_diff ro.force.debuggable 0
resetprop_if_diff ro.secure 1

exit 0