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

# Extract module files
ui_print "- Extracting module files"
extract "$ZIPFILE" 'module.prop' $MODPATH
extract "$ZIPFILE" 'service.sh' $MODPATH
extract "$ZIPFILE" 'system/bin/encore_profiler' $MODPATH
extract "$ZIPFILE" 'system/bin/encore_utility' $MODPATH

# Extract executables
if [ $ARCH = "arm64" ]; then
	extract "$ZIPFILE" 'libs/arm64-v8a/encored' $TMPDIR
	cp $TMPDIR/libs/arm64-v8a/* $MODPATH/system/bin
elif [ $ARCH = "arm" ]; then
	extract "$ZIPFILE" 'libs/armeabi-v7a/encored' $TMPDIR
	cp $TMPDIR/libs/armeabi-v7a/* $MODPATH/system/bin
else
	ui_print "*********************************************************"
	ui_print "! Unsupported ARCH: $ARCH"
	ui_print "! Encore Tweaks only supports ARM Chipsets"
	abort "*********************************************************"
fi
rm -rf $TMPDIR/libs

# Extract webroot
ui_print "- Extracting webroot"
unzip -o "$ZIPFILE" "webroot/*" -d "$MODPATH" >&2

# Set configs
ui_print "- Encore Tweaks configuration setup"
[ ! -d /data/encore ] && mkdir /data/encore
extract "$ZIPFILE" 'encore_logo.png' "/data/local/tmp"
unzip -o "$ZIPFILE" 'gamelist.txt' -d "/data/encore" >&2
[ ! -f /data/encore/kill_logd ] && echo 0 >/data/encore/kill_logd

# KSU WebUI for Magisk user
if [ "$(which magisk)" ]; then
	extract "$ZIPFILE" 'action.sh' $MODPATH

	if ! pm list packages | grep -q io.github.a13e300.ksuwebui; then
		ui_print "- Magisk detected, Installing KSU WebUI for Magisk"
		extract "$ZIPFILE" 'webui.apk' $TMPDIR
		pm install $TMPDIR/webui.apk >&2
		rm -f $TMPDIR/webui.apk

		if ! pm list packages | grep -q io.github.a13e300.ksuwebui; then
			ui_print "- Can't install KSU WebUI due to selinux restrictions"
			ui_print "  Please install the app manually after installation."
		else
			ui_print "- Please open and grant root permission for KSU WebUI"
			ui_print "  after this installation."
		fi
	fi
fi

# Bellavita Toast
if ! pm list packages | grep -q bellavita.toast; then
	ui_print "- Installing bellavita Toast"
	extract "$ZIPFILE" 'toast.apk' $TMPDIR
	pm install $TMPDIR/toast.apk >&2
	rm -f $TMPDIR/toast.apk
	if ! pm list packages | grep -q bellavita.toast; then
		ui_print "- Can't install Bellavita Toast due to selinux restrictions"
		ui_print "  Please install the app manually after installation."
	fi
fi

# Permission settings
ui_print "- Permission setup"
set_perm_recursive "$MODPATH/system/bin" 0 0 0755 0755

# Determine Chipset
chipset=$(grep "Hardware" /proc/cpuinfo | uniq | cut -d ':' -f 2 | sed 's/^[ \t]*//')

if [ -z "$chipset" ]; then
	chipset="$(getprop ro.board.platform) $(getprop ro.hardware)"
fi

case "$chipset" in
*mt* | *MT*) soc=1 && ui_print "- Implementing tailored tweaks for Mediatek" ;;
*sm* | *qcom* | *SM* | *QCOM* | *Qualcomm*) soc=2 && ui_print "- Implementing tailored tweaks for Snapdragon" ;;
*exynos*) soc=3 && ui_print "- Implementing tailored tweaks for Exynos" ;;
*Unisoc* | *unisoc*) soc=4 && ui_print "- Implementing tailored tweak for Unisoc" ;;
*gs*) soc=5 && ui_print "- Implementing tailored tweaking for Google Tensor" ;;
*)
	if [ -f /sys/devices/soc0/machine ] && [ ! -d /sys/kernel/gpu ]; then
		soc=2
		ui_print "- Implementing tailored tweaks for Snapdragon"
	else
		soc=0
	fi
	;;
esac

if [ $soc -eq 0 ]; then
	ui_print "- Unknown SoC manufacturer, skipping some tweaks implementation"
fi

echo $soc >/data/encore/soc_recognition

ui_print ""
case "$((RANDOM % 6 + 1))" in
1) ui_print "- Wooly's Fairy Tale" ;;
2) ui_print "- Sheep-counting Lullaby" ;;
3) ui_print "- Fog? The Black Shores!" ;;
4) ui_print "- Adventure? Let's go!" ;;
5) ui_print "- Hero Takes the Stage!" ;;
6) ui_print "- Woolies Save the World!" ;;
esac
