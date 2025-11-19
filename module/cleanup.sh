#!/system/bin/sh
# shellcheck disable=SC2016

MODULE_DIR="/data/adb/modules/encore"
THIS_SCRIPT="/data/adb/service.d/.encore_cleanup.sh"

if [ ! -d "$MODULE_DIR/disable" ]; then
  cat "$MODULE_DIR/module.prop.orig" >"$MODULE_DIR/module.prop"
  rm -f "$THIS_SCRIPT"
fi
