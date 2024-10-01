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
extract "$ZIPFILE" 'module.prop' $MODPATH
extract "$ZIPFILE" 'service.sh' $MODPATH
extract "$ZIPFILE" 'libs/arm64-v8a/encore-service' $TMPDIR
extract "$ZIPFILE" 'libs/arm64-v8a/vmtouch' $TMPDIR
extract "$ZIPFILE" 'libs/armeabi-v7a/encore-service' $TMPDIR
extract "$ZIPFILE" 'libs/armeabi-v7a/vmtouch' $TMPDIR
extract "$ZIPFILE" 'system/bin/encore-utils' $MODPATH
extract "$ZIPFILE" 'system/bin/encore-perfcommon' $MODPATH
extract "$ZIPFILE" 'system/bin/encore-normal' $MODPATH
extract "$ZIPFILE" 'system/bin/encore-powersave' $MODPATH
extract "$ZIPFILE" 'system/bin/encore-performance' $MODPATH
extract "$ZIPFILE" 'system/bin/encore-mempreload' $MODPATH
extract "$ZIPFILE" 'system/bin/encore-setpriority' $MODPATH

# Extrace executable
if [ $ARCH = "arm64" ]; then
	ui_print "- Copying arm64 libs"
	cp $TMPDIR/libs/arm64-v8a/* $MODPATH/system/bin
elif [ $ARCH = "arm" ]; then
	ui_print "- Copying arm libs"
	cp $TMPDIR/libs/armeabi-v7a/* $MODPATH/system/bin
else
	abort "- Unsupported ARCH: $ARCH"
fi

# Extract webroot
ui_print "- Extracting webroot"
unzip -o "$ZIPFILE" "webroot/*" -d "$MODPATH" >&2

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
	ui_print "- Installing bellavita toast..."
	extract "$ZIPFILE" 'toast.apk' $TMPDIR
	pm install $TMPDIR/toast.apk >&2
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
