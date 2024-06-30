#!/sbin/sh
umask 022
ui_print() { echo "$1"; }
require_new_magisk() {
  ui_print "*******************************"
  ui_print " Please install Magisk v20.4+! "
  ui_print "*******************************"
  exit 1
}
OUTFD=$2
ZIPFILE=$3
mount /data 2>/dev/null
if ! [ -f /data/adb/magisk/util_functions.sh ]; then
  require_new_magisk
fi
. /data/adb/magisk/util_functions.sh
if [ $MAGISK_VER_CODE -lt 20400 ]; then
  require_new_magisk
fi
install_module
exit 0