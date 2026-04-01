#!/system/bin/sh
MODPATH="${0%/*}"
. $MODPATH/common_func.sh

# Module path and file references
SCRIPT="$MODPATH/webroot/common_scripts/autopilot.sh"
LOG_DIR="/data/adb/Box-Brain/Integrity-Box-Logs"
PROP="/data/adb/modules/playintegrityfix/system.prop"
PROP1="ro.crypto.state=encrypted"
PROP2="ro.build.tags=release-keys"
PROP3="ro.build.type=user"
PIF="/data/adb/modules/playintegrityfix"
LOG="$LOG_DIR/service.log"
LOG2="$LOG_DIR/encrypt.log"
LOG3="$LOG_DIR/autopif.log"
LOG4="$LOG_DIR/twrp.log"
LOG5="$LOG_DIR/tag.log"
LOG6="$LOG_DIR/build.log"

# Log folder
mkdir -p "$LOG_DIR"

# Logger function
log() {
    echo "$(date '+%Y-%m-%d %H:%M:%S') | $1" | tee -a "$LOG"
}

# Spoof Encryption 
{
  echo "ENCRYPT CHECK ($(date))"

  if [ -f /data/adb/Box-Brain/encrypt ]; then
    if grep -qxF "$PROP1" "$PROP"; then
      echo "Prop already exists, no action needed"
    else
      echo "$PROP1" >> "$PROP"
      echo "Spoofed prop: $PROP1"
    fi
  else
    if grep -qxF "$PROP1" "$PROP"; then
      sed -i "\|^$LINE\$|d" "$PROP"
      echo "Removed line: $PROP1"
    else
      echo "Prop not present, no action needed"
    fi
  fi

  echo
} >> "$LOG2" 2>&1

# Spoof Tag 
{
  echo "TAG CHECK ($(date))"

  if [ -f /data/adb/Box-Brain/tag ]; then
    if grep -qxF "$PROP2" "$PROP"; then
      echo "Prop already exists, no action needed"
    else
      echo "$PROP2" >> "$PROP"
      echo "Spoofed prop: $PROP1"
    fi
  else
    if grep -qxF "$PROP2" "$PROP"; then
      sed -i "\|^$PROP2\$|d" "$PROP"
      echo "Removed line: $PROP2"
    else
      echo "Prop not present, no action needed"
    fi
  fi

  echo
} >> "$LOG5" 2>&1

# Spoof Build 
{
  echo "BUILD CHECK ($(date))"

  if [ -f /data/adb/Box-Brain/build ]; then
    if grep -qxF "$PROP3" "$PROP"; then
      echo "Prop already exists, no action needed"
    else
      echo "$PROP3" >> "$PROP"
      echo "Spoofed prop: $PROP3"
    fi
  else
    if grep -qxF "$PROP3" "$PROP"; then
      sed -i "\|^$PROP3\$|d" "$PROP"
      echo "Removed line: $PROP3"
    else
      echo "Prop not present, no action needed"
    fi
  fi

  echo
} >> "$LOG6" 2>&1

# Rename twrp folder to avoid root detection
{
  echo "TWRP/FOX RENAME ($(date))"
  echo
  [ -f /data/adb/Box-Brain/twrp ] && hide_recovery_folders
} >> "$LOG4" 2>&1

# Reset system properties if mismatch 
resetprop_if_diff sys.usb.adb.disabled 1
resetprop_if_diff service.adb.root 0
resetprop_if_diff persist.sys.developer_options 0
resetprop_if_diff persist.sys.dev_mode 0
resetprop_if_diff persist.sys.debuggable 0
resetprop_if_diff ro.oem_unlock_supported 0
resetprop_if_diff ro.hardware.virtual_device 0

##########################################
# adapted from Play Integrity Fork by @osm0sis
# source: https://github.com/osm0sis/PlayIntegrityFork
# license: GPL-3.0
##########################################

# Conditional sensitive properties
# Magisk Recovery Mode
resetprop_if_match ro.boot.mode recovery unknown
resetprop_if_match ro.bootmode recovery unknown
resetprop_if_match vendor.boot.mode recovery unknown

# SELinux
resetprop_if_diff ro.boot.selinux enforcing
# use delete since it can be 0 or 1 for enforcing depending on OEM
if ! $SKIPDELPROP; then
    delprop_if_exist ro.build.selinux
fi
# use toybox to protect stat access time reading
if [ "$(toybox cat /sys/fs/selinux/enforce)" = "0" ]; then
    chmod 640 /sys/fs/selinux/enforce
    chmod 440 /sys/fs/selinux/policy
fi

# Conditional late sensitive properties

# must be set after boot_completed for various OEMs
{
until [ "$(getprop sys.boot_completed)" = "1" ]; do
    sleep 1
done

# SafetyNet/Play Integrity + OEM
# avoid bootloop on some Xiaomi devices
resetprop_if_diff ro.secureboot.lockstate locked
# avoid breaking Realme fingerprint scanners
resetprop_if_diff ro.boot.flash.locked 1
resetprop_if_diff ro.boot.realme.lockstate 1
# avoid breaking Oppo fingerprint scanners
resetprop_if_diff ro.boot.vbmeta.device_state locked
# avoid breaking OnePlus display modes/fingerprint scanners
resetprop_if_diff vendor.boot.verifiedbootstate green
# avoid breaking OnePlus/Oppo fingerprint scanners on OOS/ColorOS 12+
resetprop_if_diff ro.boot.verifiedbootstate green
resetprop_if_diff ro.boot.veritymode enforcing
resetprop_if_diff vendor.boot.vbmeta.device_state locked

# Other
resetprop_if_diff sys.oem_unlock_allowed 0

}&

# Stop when needed
if [ -f "/data/adb/Box-Brain/rukja" ]; then
    log "Daemon disabled by user"
    exit 1
fi

# Restarts daemon if dead
while true; do
    if [ -f "/data/adb/Box-Brain/autopilot" ]; then
        # Check heartbeat
        last=$(cat /data/adb/Box-Brain/daemon_heartbeat 2>/dev/null || echo "0")
        now=$(date +%s)
        
        # Dead if no heartbeat for 3+ minutes
        if [ $((now - last)) -gt 180 ]; then
            # Clean stale locks
            rm -rf /data/adb/Box-Brain/autorun.lockdir \
                   /data/adb/Box-Brain/.executing 2>/dev/null
            
            # Restart
            sh "$SCRIPT" >/dev/null 2>&1 &
        fi
    fi
    
    sleep 60
done
