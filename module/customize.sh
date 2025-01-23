# shellcheck disable=SC2034
SKIPUNZIP=1
SOC=0

abort_unsupported_arch() {
	ui_print "*********************************************************"
	ui_print "! Unsupported ARCH: $ARCH"
	ui_print "! Encore Tweaks does not support your CPU architecture"
	ui_print "! If you think this is wrong, please report to maintainer"
	abort "*********************************************************"
}

abort_corrupted() {
	ui_print "*********************************************************"
	ui_print "! Unable to extract verify.sh!"
	ui_print "! This zip may be corrupted, please try downloading again"
	abort "*********************************************************"
}

soc_recognition_extra() {
	[ -d /sys/class/kgsl/kgsl-3d0/devfreq ] && {
		SOC=2
		ui_print "- Implementing tweaks for Snapdragon"
		return 0
	}

	[ -d /sys/devices/platform/kgsl-2d0.0/kgsl ] && {
		SOC=2
		ui_print "- Implementing tweaks for Snapdragon"
		return 0
	}

	[ -d /sys/kernel/ged/hal ] && {
		SOC=1
		ui_print "- Implementing tweaks for MediaTek"
		return 0
	}

	[ -d /sys/kernel/tegra_gpu ] && {
		SOC=7
		ui_print "- Implementing tweaks for Nvidia Tegra"
		return 0
	}

	return 1
}

recognize_soc() {
	case "$1" in
	*mt* | *MT*) SOC=1 && ui_print "- Implementing tweaks for MediaTek" ;;
	*sm* | *qcom* | *SM* | *QCOM* | *Qualcomm*) SOC=2 && ui_print "- Implementing tweaks for Snapdragon" ;;
	*exynos* | *Exynos* | *EXYNOS* | *universal* | *samsung*) SOC=3 && ui_print "- Implementing tweaks for Exynos" ;;
	*Unisoc* | *unisoc*) SOC=4 && ui_print "- Implementing tweaks for Unisoc" ;;
	*gs*) SOC=5 && ui_print "- Implementing tweaks for Google Tensor" ;;
	*Intel* | *intel*) SOC=6 && ui_print "- Implementing tweaks for Intel" ;;
	*) return 1 ;;
	esac
}

# Flashable integrity checkup
ui_print "- Extracting verify.sh"
unzip -o "$ZIPFILE" 'verify.sh' -d "$TMPDIR" >&2
[ ! -f "$TMPDIR/verify.sh" ] && abort_corrupted
source "$TMPDIR/verify.sh"

# Extract module files
ui_print "- Extracting module files"
extract "$ZIPFILE" 'module.prop' $MODPATH
extract "$ZIPFILE" 'service.sh' $MODPATH
extract "$ZIPFILE" 'uninstall.sh' $MODPATH
extract "$ZIPFILE" 'system/bin/encore_profiler' $MODPATH
extract "$ZIPFILE" 'system/bin/encore_utility' $MODPATH
extract "$ZIPFILE" 'system/bin/encore_bypass_chg' $MODPATH

# Extract executables
case $ARCH in
"arm64") ARCH_TMP="arm64-v8a" ;;
"arm") ARCH_TMP="armeabi-v7a" ;;
"x64") ARCH_TMP="x86_64" ;;
"x86") ARCH_TMP="x86" ;;
"riscv64") ARCH_TMP="riscv64" ;;
*) abort_unsupported_arch ;;
esac

extract "$ZIPFILE" "libs/$ARCH_TMP/encored" "$TMPDIR"
cp $TMPDIR/libs/$ARCH_TMP/* "$MODPATH/system/bin"
rm -rf "$TMPDIR/libs"

# Extract webroot
ui_print "- Extracting webroot"
unzip -o "$ZIPFILE" "webroot/*" -d "$MODPATH" >&2

# Set configs
ui_print "- Encore Tweaks configuration setup"
[ ! -d /data/encore ] && mkdir /data/encore
[ ! -f /data/encore/kill_logd ] && echo 0 >/data/encore/kill_logd
[ ! -f /data/encore/bypass_charging ] && echo 0 >/data/encore/bypass_charging
[ ! -f /data/encore/gamelist.txt ] && extract "$ZIPFILE" 'gamelist.txt' "/data/encore"
extract "$ZIPFILE" 'encore_logo.png' "/data/local/tmp"
touch /data/encore/_files_on_this_directory_is_critical_for_encore_module__please_DO_NOT_REMOVE_OR_MODIFY

# Action script for Magisk user
if [ "$(which magisk)" ]; then
	extract "$ZIPFILE" 'action.sh' $MODPATH
fi

# Install Bellavita Toast
if ! pm list packages | grep -q bellavita.toast; then
	ui_print "- Installing bellavita Toast"
	extract "$ZIPFILE" 'toast.apk' $TMPDIR
	pm install $TMPDIR/toast.apk >&2
	rm -f $TMPDIR/toast.apk
fi

if ! pm list packages | grep -q bellavita.toast; then
	ui_print "! Can't install Bellavita Toast due to selinux restrictions"
	ui_print "! Please install the app manually after installation."
fi

# Permission settings
ui_print "- Permission setup"
set_perm_recursive "$MODPATH/system/bin" 0 0 0755 0755

# SOC CODE:
# 1 = MediaTek
# 2 = Qualcomm Snapdragon
# 3 = Exynos
# 4 = Unisoc
# 5 = Google Tensor
# 6 = Intel
# 7 = Nvidia Tegra

# Recognize Chipset
soc_recognition_extra
[ $SOC -eq 0 ] && recognize_soc "$(grep -E "Hardware|Processor" /proc/cpuinfo | uniq | cut -d ':' -f 2 | sed 's/^[ \t]*//')" # Try normal way
[ $SOC -eq 0 ] && recognize_soc "$(grep "model\sname" /proc/cpuinfo | uniq | cut -d ':' -f 2 | sed 's/^[ \t]*//')"           # Try Intel (or X86) way
[ $SOC -eq 0 ] && recognize_soc "$(getprop ro.board.platform) $(getprop ro.hardware) $(getprop ro.hardware.chipname)"        # Try Android way
[ $SOC -eq 0 ] && ui_print "! Unknown SoC, skipping some tweaks"                                                             # Unrecognizable :(
echo $SOC >/data/encore/soc_recognition

# Easter Egg
case "$((RANDOM % 6 + 1))" in
1) ui_print "- Wooly's Fairy Tale" ;;
2) ui_print "- Sheep-counting Lullaby" ;;
3) ui_print "- Fog? The Black Shores!" ;;
4) ui_print "- Adventure? Let's go!" ;;
5) ui_print "- Hero Takes the Stage!" ;;
6) ui_print "- Woolies Save the World!" ;;
esac
