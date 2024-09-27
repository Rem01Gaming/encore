[ ! -d /data/encore ] && mkdir /data/encore
unzip -o "$ZIPFILE" 'system/*' -d $MODPATH >&2
unzip -o "$ZIPFILE" 'libs/*' -d $TMPDIR >&2
unzip -o "$ZIPFILE" 'service.sh' -d "$MODPATH" >&2
unzip -o "$ZIPFILE" 'gamelist.txt' -d "/data/encore" >&2
unzip -o "$ZIPFILE" 'AppMonitoringUtil.sh' -d "/data/encore" >&2

if [ $ARCH = "arm64" ]; then
	ui_print "- Copying arm64 libs"
	cp $TMPDIR/libs/arm64-v8a/encore-service $MODPATH/system/bin/
elif [ $ARCH = "arm" ]; then
	ui_print "- Copying arm32 libs"
	cp $TMPDIR/libs/armeabi-v7a/encore-service $MODPATH/system/bin/
else
	rm -rf /data/encore
	abort "- Unsupported ARCH: $ARCH"
fi

[ ! -f /data/encore/preload_game ] && echo 0 >/data/encore/preload_game
[ ! -f /data/encore/preload_graphiclibs ] && echo 0 >/data/encore/preload_graphiclibs
[ ! -f /data/encore/kill_logd ] && echo 0 >/data/encore/kill_logd
[ ! -f /data/encore/perf_cpu_gov ] && echo performance >/data/encore/perf_cpu_gov

if pm list packages | grep -q bellavita.toast; then
	ui_print "- The Bellavita Toast app is already installed."
else
	ui_print "- Bellavita Toast isn't installed"
	ui_print "- Installing bellavita toast..."
	unzip -o "$ZIPFILE" 'toast.apk' -d $TMPDIR >&2
	pm install $TMPDIR/toast.apk
	rm -f $TMPDIR/toast.apk
	if ! pm list packages | grep -q bellavita.toast; then
		ui_print "- Can't install Bellavita Toast due to selinux restrictions"
		ui_print "  Please install it manually after installation."
	fi
fi

set_perm_recursive $MODPATH/system 0 0 0777 0777
ui_print "- Reboot is needed after installation"
