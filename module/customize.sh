# shellcheck disable=SC2034
SKIPUNZIP=1

# Flashable integrity checkup
ui_print "- Extracting verify.sh"
unzip -o "$ZIPFILE" 'verify.sh' -d "$TMPDIR" >&2
if [ ! -f "$TMPDIR/verify.sh" ]; then
	ui_print "*********************************************************"
	ui_print "! Unable to extract verify.sh!"
	ui_print "! This zip may be corrupted, please try downloading again"
	abort "*********************************************************"
fi

source "$TMPDIR/verify.sh"

mkdir $MODPATH/system/bin
extract "$ZIPFILE" 'service.sh' $MODPATH
extract "$ZIPFILE" 'system/bin/encore-utils' $MODPATH/system/bin
extract "$ZIPFILE" 'system/bin/encore-perfcommon' $MODPATH/system/bin
extract "$ZIPFILE" 'system/bin/encore-normal' $MODPATH/system/bin
extract "$ZIPFILE" 'system/bin/encore-powersave' $MODPATH/system/bin
extract "$ZIPFILE" 'system/bin/encore-performance' $MODPATH/system/bin
extract "$ZIPFILE" 'system/bin/encore-mempreload' $MODPATH/system/bin
extract "$ZIPFILE" 'system/bin/encore-setpriority' $MODPATH/system/bin
if [ $ARCH = "arm64" ]; then
	extract "$ZIPFILE" 'libs/arm64-v8a/encore-service' $MODPATH/system/bin
	extract "$ZIPFILE" 'libs/arm64-v8a/vmtouch' $MODPATH/system/bin
elif [ $ARCH = "arm" ]; then
	extract "$ZIPFILE" 'libs/armeabi-v7a/encore-service' $MODPATH/system/bin
	extract "$ZIPFILE" 'libs/armeabi-v7a/vmtouch' $MODPATH/system/bin
else
	abort "- Unsupported ARCH: $ARCH"
fi

# Set configs
[ ! -d /data/encore ] && mkdir /data/encore
unzip -o "$ZIPFILE" 'gamelist.txt' -d "/data/encore" >&2
unzip -o "$ZIPFILE" 'AppMonitoringUtil.sh' -d "/data/encore" >&2
[ ! -f /data/encore/preload_game ] && echo 0 >/data/encore/preload_game
[ ! -f /data/encore/preload_graphiclibs ] && echo 0 >/data/encore/preload_graphiclibs
[ ! -f /data/encore/kill_logd ] && echo 0 >/data/encore/kill_logd
[ ! -f /data/encore/perf_cpu_gov ] && echo performance >/data/encore/perf_cpu_gov

if pm list packages | grep -q bellavita.toast; then
	ui_print "- Bellavita Toast app is already installed"
else
	ui_print "- Bellavita Toast isn't installed"
	ui_print "- Installing bellavita toast..."
	extract "$ZIPFILE" 'toast.apk' $TMPDIR
	pm install $TMPDIR/toast.apk
	rm -f $TMPDIR/toast.apk
	if ! pm list packages | grep -q bellavita.toast; then
		ui_print "- Can't install Bellavita Toast due to selinux restrictions"
		ui_print "- Please install Bellavita Toast manually after installation."
	fi
fi

ui_print "- Permission setup"
set_perm_recursive "$MODPATH/system/bin" 0 0 0755 0755

ui_print ""
ui_print "- A long, long time ago... Cosmos told Encore about a fun spot!"
ui_print "- Come on! Let's go check it out ~"
