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

echo 0 >/data/encore/skip_setpriority

set_perm_recursive $MODPATH 0 0 0777 0777
ui_print "- Reboot is needed after installation"
