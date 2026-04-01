RECORD="/data/adb/Box-Brain/Integrity-Box-Logs"
OUT="/storage/emulated/0/Download/IntegrityModules"
BOX="/data/adb/Box-Brain"
LOGZ="/data/adb/Box-Brain/Integrity-Box-Logs/integrity_downloader.log"
LOG_FILE="/data/adb/Box-Brain/Integrity-Box-Logs/action.log"
WIDTH=55

# Logger function
pif() {
    echo "$1" | tee -a "$RECORD/PlayIntegrityScript.log"
}

recommended_settings() {
    touch "$BOX/migrate_force"
    touch "$BOX/run_migrate"
    touch "$BOX/nodebug"
    touch "$BOX/encrypt"
    touch "$BOX/build"
    touch "$BOX/twrp"
    touch "$BOX/tag"
}

# Logger function
denylog() {
    echo "$1" | tee -a "$RECORD/denylist.log"
}

center() { printf "%*s\n" $(((${#1}+$WIDTH)/2)) "$1"; }

banner() {
  printf "%${WIDTH}s\n" | tr ' ' '='
  center "INTEGRITY BOX DOWNLOADER"
  printf "%${WIDTH}s\n" | tr ' ' '='
}

print_row() {
  printf "%-22s %-12s %-20s\n" "$1" "$2" "$3"
}

sha_ok() {
  if [ ! -f "$1" ]; then return 1; fi
  echo "$2  $1" | sha256sum -c - >/dev/null 2>&1
}

get_size() {
  if [ -f "$1" ]; then du -h "$1" 2>/dev/null | awk '{print $1}'; else echo "-"; fi
}

# determine downloader binary
detect_downloader() {
  # curl
  if command -v curl >/dev/null 2>&1; then
    DOWNLOADER=$(command -v curl)
    DL_MODE="curl"
    return
  fi

  # wget
  if command -v wget >/dev/null 2>&1; then
    DOWNLOADER=$(command -v wget)
    DL_MODE="wget"
    return
  fi

  # Magisk BusyBox
  if [ -x /data/adb/magisk/busybox ]; then
    DOWNLOADER="/data/adb/magisk/busybox"
    DL_MODE="busybox"
    return
  fi

  # KSU BusyBox
  if [ -x /data/adb/ksu/bin/busybox ]; then
    DOWNLOADER="/data/adb/ksu/bin/busybox"
    DL_MODE="busybox"
    return
  fi

  # 5. Built-in toybox wget
  if toybox wget --help >/dev/null 2>&1; then
    DOWNLOADER="toybox"
    DL_MODE="toybox"
    return
  fi

  # nothing available
  DOWNLOADER=""
  DL_MODE=""
}

wait_for_network() {
  max_wait=${1:-30} # seconds
  step=2
  waited=0

  while [ $waited -lt $max_wait ]; do
    if command -v ping >/dev/null 2>&1; then
      ping -c 1 1.1.1.1 >/dev/null 2>&1 && return 0
    fi

    if [ -n "$DOWNLOADER" ]; then
      if [ "$DL_MODE" = "curl" ]; then
        /system/bin/curl -s --head --connect-timeout 5 https://raw.githubusercontent.com >/dev/null 2>&1 && return 0
      elif [ "$DL_MODE" = "wget" ]; then
        /system/bin/wget --spider --timeout=5 --tries=1 https://raw.githubusercontent.com >/dev/null 2>&1 && return 0
      elif [ "$DL_MODE" = "busybox" ]; then
        /data/adb/magisk/busybox wget --spider --timeout=5 --tries=1 https://raw.githubusercontent.com >/dev/null 2>&1 && return 0
      fi
    fi

    sleep $step
    waited=$((waited+step))
  done

  return 1
}

download() {
  url="$1"
  file="$2"
  sum="$3"

  tmp="$OUT/$file.tmp"
  final="$OUT/$file"
  rm -f "$tmp" "$final"

  detect_downloader
  if [ -z "$DOWNLOADER" ]; then
    echo "ERROR: No downloader binary found" >>"$LOGZ"
    return 1
  fi

  att=1
  while [ $att -le 3 ]; do
    echo "$(date +%F' '%T) Download attempt $att for $file using $DL_MODE" >>"$LOGZ"

    if [ "$DL_MODE" = "curl" ]; then
        "$DOWNLOADER" -L --fail --connect-timeout 10 --max-time 120 -o "$tmp" "$url" 2>>"$LOGZ"
        rc=$?
    elif [ "$DL_MODE" = "wget" ]; then
        "$DOWNLOADER" --no-check-certificate -O "$tmp" "$url" 2>>"$LOGZ"
        rc=$?
    elif [ "$DL_MODE" = "busybox" ]; then
        "$DOWNLOADER" wget --no-check-certificate -O "$tmp" "$url" 2>>"$LOGZ"
        rc=$?
    elif [ "$DL_MODE" = "toybox" ]; then
        toybox wget -O "$tmp" "$url" 2>>"$LOGZ"
        rc=$?
    fi

    if [ $rc -ne 0 ]; then
      echo "WARN: downloader failed rc=$rc for $file" >>"$LOGZ"
      rm -f "$tmp"
      att=$((att+1))
      sleep 1
      continue
    fi

    # verify sha
    if sha_ok "$tmp" "$sum"; then
      mv "$tmp" "$final"
      echo "$(date +%F' '%T) OK: $file saved to $final" >>"$LOGZ"
      return 0
    else
      echo "WARN: SHA mismatch for $file" >>"$LOGZ"
      rm -f "$tmp"
      att=$((att+1))
      sleep 1
      continue
    fi
  done

  echo "ERROR: Failed to download $file after retries" >>"$LOGZ"
  rm -f "$tmp"
  return 1
}

safe_mv() {
  src="$1"
  dst="$2"
  mkdir -p "$(dirname "$dst")"
  mv "$src" "$dst" 2>>"$LOGZ" || cp -f "$src" "$dst" 2>>"$LOGZ" && rm -f "$src"
  return $?
}

# Configure DenyList
add_if_missing() {
    pkg="$1"; proc="$2"
    entry="$pkg|${proc:-$pkg}"
    if ! magisk --denylist ls | grep -q "$entry"; then
        magisk --denylist add "$pkg" $proc
        denylog "[AutoDeny] Added $entry"
    fi
}

setval() { grep -q "^$2=" "$1" && sed -i "s/^$2=.*/$2=$3/" "$1" && log "$2 > $3" || log "$2 not found"; }

lineage() {
    echo "$(date '+%Y-%m-%d %H:%M:%S') $*" | tee -a "$RECORD/lineage.log"
#    echo "$(date '+%Y-%m-%d %H:%M:%S') $*"
}

chup() {
echo "$(date '+%Y-%m-%d %H:%M:%S') - $1" >> "$RECORD/pixel.log"
}

set_resetprop() {
    local PROP="$1"
    local VALUE="$2"

    if prop_exists "$PROP"; then
        if resetprop -n -p "$PROP" "$VALUE" 2>/dev/null; then
            chup "Disabled spoof: $PROP > $VALUE"
        else
            chup "Failed to modify $PROP"
        fi
    else
        chup "Skipped $PROP (not defined)"
    fi
}

set_simpleprop() {
    PROP="$1"
    VALUE="$2"
    CURRENT=$(su -c getprop "$PROP")
    
    if [ -n "$CURRENT" ]; then
        su -c setprop "$PROP" "$VALUE" > /dev/null 2>&1
        chup "Set $PROP to $VALUE"
    else
        chup "Skipping $PROP, property does not exist"
    fi
}

# Helper to add packages
add_pkg() {
  pkg="$1"
  if [ "$teeBroken" = "true" ]; then
    echo "${pkg}!" >> "$TMP"
  else
    echo "$pkg" >> "$TMP"
  fi
}

# Connectivity check
megatron() {
  max_attempts=5
  attempt=1
  delay=1
  hosts="1.1.1.1 8.8.8.8 9.9.9.9 223.5.5.5 114.114.114.114"

  while [ $attempt -le $max_attempts ]; do
    echo " "
    echo "🌐 Attempt $attempt of $max_attempts..."

    for host in $hosts; do
      if ping -c1 -W2 "$host" >/dev/null 2>&1; then
        return 0
      fi
    done

    echo "❌ No internet detected"
    sleep $delay
    attempt=$((attempt + 1))
    [ $delay -lt 5 ] && delay=$((delay + 1))
  done

  echo "🚫 No internet detected after $max_attempts attempts."
  return 1
}

# Print header
print_header() {
  echo "
    ____      __                  _ __       
   /  _/___  / /____  ____ ______(_) /___  __
   / // __ \/ __/ _ \/ __ / ___/ / __/  / / /
 _/ // / / / /_/  __/ /_/ / /  / / /_/ /_/ / 
/___/_/ /_/\__/\___/\__, /_/  /_/\__/\__, /  
                   /____/           /____/           
             ____            
            / __ )____  _  __
           / __  / __ \| |/_/
          / /_/ / /_/ />  <  
         /_____/\____/_/|_|  
                    
"
}

# Track results
log_step() {
  local status="$1"
  local task="$2"
  local timestamp
  
  timestamp=$(date '+%Y-%m-%d %H:%M:%S')
  
  printf -- " ✦ %-10s %s\n" "$status" "$task"
  printf "[%s] %-10s %s\n" "$timestamp" "$status" "$task" >> "$LOG_FILE"
}

# Exit delay
handle_delay() {
  if [ "$KSU" = "true" ] || [ "$APATCH" = "true" ] && [ "$KSU_NEXT" != "true" ] && [ "$MMRL" != "true" ]; then
    echo
    echo " Closing in 7 seconds..."
    sleep 7
  fi
}

log_patch() {
    echo "$(date '+%Y-%m-%d %H:%M:%S') - $*" >> "$RECORD/patch.log"
}

# Kill GMS / Vending Processes
kill_process() {
  TARGET="$1"
  PID=$(pidof "$TARGET")
  if [ -n "$PID" ]; then
    kill -9 $PID
    log "- Killed $TARGET"
  else
    log "- $TARGET not running"
  fi
}
  
hide_recovery_folders() {
    [ -f /data/adb/Box-Brain/twrp ] || return

    SRC="/sdcard"
    DEST="/data/adb/recovery_backups"
    mkdir -p "$DEST"

    FOLDERS="TWRP OrangeFox FOX PBRP PitchBlack Recovery"

    random_str() { head /dev/urandom | tr -dc A-Za-z0-9 | head -c 12; }

    for F in $FOLDERS; do
        DIR="$SRC/$F"
        [ -d "$DIR" ] || continue

        if [ -f "$DIR/.twrps" ]; then
            rm -f "$DIR/.twrps" 2>/dev/null
            if [ -f "$DIR/.twrps" ]; then
                NEWF=".$(random_str)_$(date +%s)"
                mv "$DIR" "$SRC/$NEWF" 2>/dev/null
                DIR="$SRC/$NEWF"
                rm -f "$DIR/.twrps" 2>/dev/null
            fi
        fi

        SUB=$(find "$DIR" -mindepth 1 -maxdepth 1 -type d ! -name ".*" 2>/dev/null | wc -l)

        if [ "$SUB" -gt 0 ]; then
            mv "$DIR" "$DEST/$(random_str)" 2>/dev/null
        else
            rm -rf "$DIR" 2>/dev/null
        fi
    done
}

run_temp_exec() {
    local script="$1"

    if [ ! -r "$script" ]; then
        echo "Script $script not readable ❌"
        return 1
    fi

    local orig_mode
    orig_mode=$(stat -c "%a" "$script")
    echo "Original permission: $orig_mode"

    chmod +x "$script"
    echo "Temporary +x granted, executing..."
    "$script"

    echo "Execution finished, reverting permission"
    chmod "$orig_mode" "$script"
}

delete_if_exist() {
    path="$1"
    if [ -e "$path" ]; then
        rm -rf "$path"
        log "Deleted: $path"
    fi
}

P() {
  for Q in /data/adb/modules/busybox-ndk/system/*/busybox \
           /data/adb/ksu/bin/busybox \
           /data/adb/ap/bin/busybox \
           /data/adb/magisk/busybox; do
    [ -x "$Q" ] && echo "$Q" && return
  done
}

Z() {
  b=0; s=0
  while IFS= read -r -n1 c; do
    case "$c" in
      [A-Z]) v=$(printf '%d' "'$c"); v=$((v - 65));;
      [a-z]) v=$(printf '%d' "'$c"); v=$((v - 71));;
      [0-9]) v=$(printf '%d' "'$c"); v=$((v + 4));;
      '+') v=62;;
      '/') v=63;;
      '=') break;;
      *) continue;;
    esac
    b=$((b << 6 | v)); s=$((s + 6))
    if [ "$s" -ge 8 ]; then
      s=$((s - 8)); o=$(((b >> s) & 0xFF))
      printf \\$(printf '%03o' "$o")
    fi
  done
}

y() {
  p=$1
  f="$p"
  if echo "$p" | grep -q "/modules/"; then
    alt_f=$(echo "$p" | sed 's/\/modules\//\/modules_update\//')
  else
    alt_f=""
  fi

  # Check first path
  if [ -r "$f" ] && [ -s "$f" ]; then
    return 0
  fi

  # Check alternate path if set
  if [ -n "$alt_f" ] && [ -r "$alt_f" ] && [ -s "$alt_f" ]; then
    return 0
  fi

  log " ✦ Missing file: $p (tried: $f ${alt_f}) "
  reboot recovery
  exit 100
}

P() {
  for Q in /data/adb/modules/busybox-ndk/system/*/busybox \
           /data/adb/ksu/bin/busybox \
           /data/adb/ap/bin/busybox \
           /data/adb/magisk/busybox; do
    [ -x "$Q" ] && echo "$Q" && return
  done
}

Z() {
  b=0; s=0
  while IFS= read -r -n1 c; do
    case "$c" in
      [A-Z]) v=$(printf '%d' "'$c"); v=$((v - 65));;
      [a-z]) v=$(printf '%d' "'$c"); v=$((v - 71));;
      [0-9]) v=$(printf '%d' "'$c"); v=$((v + 4));;
      '+') v=62;;
      '/') v=63;;
      '=') break;;
      *) continue;;
    esac
    b=$((b << 6 | v)); s=$((s + 6))
    if [ "$s" -ge 8 ]; then
      s=$((s - 8)); o=$(((b >> s) & 0xFF))
      printf \\$(printf '%03o' "$o")
    fi
  done
}

y() {
  p=$1
  f="$p"
  if echo "$p" | grep -q "/modules/"; then
    alt_f=$(echo "$p" | sed 's/\/modules\//\/modules_update\//')
  else
    alt_f=""
  fi

  # Check first path
  if [ -r "$f" ] && [ -s "$f" ]; then
    return 0
  fi

  # Check alternate path if set
  if [ -n "$alt_f" ] && [ -r "$alt_f" ] && [ -s "$alt_f" ]; then
    return 0
  fi

  log " ✦ Missing file: $p (tried: $f ${alt_f}) "
  reboot recovery
  exit 100
}

writelog() {
    echo "$(date +'%Y-%m-%d %H:%M:%S') - $1" | tee -a "$LOG_FILE"
    /system/bin/log -t PATCH_OVERRIDE "$1"
}

# Function to check and set property if needed
check_and_set_prop() {
    local PROP=$1
    local VALUE=$2

    local CURRENT
    CURRENT=$(getprop "$PROP")

    if [ "$CURRENT" = "$VALUE" ]; then
        writelog " $PROP is already set to $VALUE no change needed"
    else
        if resetprop "$PROP" "$VALUE"; then
            writelog " Set $PROP to $VALUE (was: $CURRENT)"
        else
            writelog " Failed to set $PROP (current: $CURRENT)"
        fi
    fi
}

ensure_blacklist_entries() {
    BLACKLIST="/data/adb/Box-Brain/blacklist.txt"

    # Ensure directory & file exist
    mkdir -p "$(dirname "$BLACKLIST")"
    [ -f "$BLACKLIST" ] || touch "$BLACKLIST"

    # Required blacklist entries
    REQUIRED_ENTRIES="
io.github.vvb2060.mahoshojo
com.reveny.nativecheck
icu.nullptr.nativetest
com.android.nativetest
io.liankong.riskdetector
me.garfieldhan.holmes
luna.safe.luna
com.zhenxi.hunter
io.github.a13e300.ksuwebui
com.topjohnwu.magisk
com.dergoogler.mmrl.wx
com.dergoogler.mmrl
org.frknkrc44.hma_oss
"

    for entry in $REQUIRED_ENTRIES; do
        # Exact match only
        if ! grep -qxF "$entry" "$BLACKLIST"; then
            echo "$entry" >> "$BLACKLIST"
        fi
    done
}

ensure_exec_permissions() {
  local DIR="/data/adb/modules/playintegrityfix"

  [ -d "$DIR" ] || return 0

  for file in "$DIR"/*.sh; do
    [ -f "$file" ] || continue

    if [ ! -x "$file" ]; then
      chmod +x "$file"
    fi
  done
}

##########################################
# adapted from Play Integrity Fork by @osm0sis
# source: https://github.com/osm0sis/PlayIntegrityFork
# license: GPL-3.0
##########################################

SKIPDELPROP=false
[ -f "$MODPATH/skipdelprop" ] && SKIPDELPROP=true

# delprop_if_exist <prop name>
delprop_if_exist() {
    local NAME="$1"

    [ -n "$(resetprop "$NAME")" ] && resetprop --delete "$NAME"
}

SKIPPERSISTPROP=false
[ -f "$MODPATH/skippersistprop" ] && SKIPPERSISTPROP=true

# persistprop <prop name> <new value>
persistprop() {
    local NAME="$1"
    local NEWVALUE="$2"
    local CURVALUE="$(resetprop "$NAME")"

    if ! grep -q "$NAME" $MODPATH/uninstall.sh 2>/dev/null; then
        if [ "$CURVALUE" ]; then
            [ "$NEWVALUE" = "$CURVALUE" ] || echo "resetprop -n -p \"$NAME\" \"$CURVALUE\"" >> $MODPATH/uninstall.sh
        else
            echo "resetprop -p --delete \"$NAME\"" >> $MODPATH/uninstall.sh
        fi
    fi
    resetprop -n -p "$NAME" "$NEWVALUE"
}

RESETPROP="resetprop -n"
[ -f /data/adb/magisk/util_functions.sh ] && [ "$(grep MAGISK_VER_CODE /data/adb/magisk/util_functions.sh | cut -d= -f2)" -lt 27003 ] && RESETPROP=resetprop_hexpatch

# resetprop_hexpatch [-f|--force] <prop name> <new value>
resetprop_hexpatch() {
    case "$1" in
        -f|--force) local FORCE=1; shift;;
    esac 

    local NAME="$1"
    local NEWVALUE="$2"
    local CURVALUE="$(resetprop "$NAME")"

    [ ! "$NEWVALUE" -o ! "$CURVALUE" ] && return 1
    [ "$NEWVALUE" = "$CURVALUE" -a ! "$FORCE" ] && return 2

    local NEWLEN=${#NEWVALUE}
    if [ -f /dev/__properties__ ]; then
        local PROPFILE=/dev/__properties__
    else
        local PROPFILE="/dev/__properties__/$(resetprop -Z "$NAME")"
    fi
    [ ! -f "$PROPFILE" ] && return 3
    local NAMEOFFSET=$(echo $(strings -t d "$PROPFILE" | grep "$NAME") | cut -d\  -f1)

    #<hex 2-byte change counter><flags byte><hex length of prop value><prop value + nul padding to 92 bytes><prop name>
    local NEWHEX="$(printf '%02x' "$NEWLEN")$(printf "$NEWVALUE" | od -A n -t x1 -v | tr -d ' \n')$(printf "%$((92-NEWLEN))s" | sed 's/ /00/g')"

    printf "Patch '$NAME' to '$NEWVALUE' in '$PROPFILE' @ 0x%08x -> \n[0000??$NEWHEX]\n" $((NAMEOFFSET-96))

    echo -ne "\x00\x00" \
        | dd obs=1 count=2 seek=$((NAMEOFFSET-96)) conv=notrunc of="$PROPFILE"
    echo -ne "$(printf "$NEWHEX" | sed -e 's/.\{2\}/&\\x/g' -e 's/^/\\x/' -e 's/\\x$//')" \
        | dd obs=1 count=93 seek=$((NAMEOFFSET-93)) conv=notrunc of="$PROPFILE"
}

# resetprop_if_diff <prop name> <expected value>
resetprop_if_diff() {
    local NAME="$1"
    local EXPECTED="$2"
    local CURRENT="$(resetprop "$NAME")"

    [ -z "$CURRENT" ] || [ "$CURRENT" = "$EXPECTED" ] || $RESETPROP "$NAME" "$EXPECTED"
}

# resetprop_if_match <prop name> <value match string> <new value>
resetprop_if_match() {
    local NAME="$1"
    local CONTAINS="$2"
    local VALUE="$3"

    [[ "$(resetprop "$NAME")" = *"$CONTAINS"* ]] && $RESETPROP "$NAME" "$VALUE"
}

# stub for boot-time
if [ "$(getprop sys.boot_completed)" != "1" ]; then
    ui_print() { return; }
fi
