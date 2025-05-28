#
# Copyright (C) 2024-2025 Rem01Gaming
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# shellcheck disable=SC1091,SC2034,SC2317
SKIPUNZIP=1
SOC=0

make_node() {
	[ ! -f "$2" ] && echo "$1" >"$2"
}

make_dir() {
	[ ! -d "$1" ] && mkdir "$1"
}

abort_unsupported_arch() {
	ui_print "*********************************************************"
	ui_print "! Unsupported Architecture: $ARCH"
	ui_print "! Your CPU architecture is not supported by Encore Tweaks."
	abort "*********************************************************"
}

abort_corrupted() {
	ui_print "*********************************************************"
	ui_print "! Unable to extract verify.sh!"
	ui_print "! Installation aborted. The module may be corrupted."
	ui_print "! Please re-download and try again."
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

get_soc_getprop() {
	local SOC_PROP="
ro.board.platform
ro.soc.model
ro.hardware
ro.chipname
ro.hardware.chipname
ro.vendor.soc.model.external_name
ro.vendor.qti.soc_name
ro.vendor.soc.model.part_name
ro.vendor.soc.model
"

	for prop in $SOC_PROP; do
		getprop "$prop"
	done
}

recognize_soc() {
	case "$1" in
	*mt* | *MT*) SOC=1 ;;
	*sm* | *qcom* | *SM* | *QCOM* | *Qualcomm*) SOC=2 ;;
	*exynos* | *Exynos* | *EXYNOS* | *universal* | *samsung* | *erd* | *s5e*) SOC=3 ;;
	*Unisoc* | *unisoc* | *ums*) SOC=4 ;;
	*gs* | *Tensor* | *tensor*) SOC=5 ;;
	*Intel* | *intel*) SOC=6 ;;
	*kirin*) SOC=8 ;;
	esac

	case "$SOC" in
	1) ui_print "- Implementing tweaks for MediaTek" ;;
	2) ui_print "- Implementing tweaks for Snapdragon" ;;
	3) ui_print "- Implementing tweaks for Exynos" ;;
	4) ui_print "- Implementing tweaks for Unisoc" ;;
	5) ui_print "- Implementing tweaks for Google Tensor" ;;
	6) ui_print "- Implementing tweaks for Intel" ;;
	7) ui_print "- Implementing tweaks for Nvidia Tegra" ;;
	8) ui_print "- Implementing tweaks for Kirin" ;;
	0) return 1 ;;
	esac
}

# Flashable integrity checkup
ui_print "- Extracting verify.sh"
unzip -o "$ZIPFILE" 'verify.sh' -d "$TMPDIR" >&2
[ ! -f "$TMPDIR/verify.sh" ] && abort_corrupted
source "$TMPDIR/verify.sh"

# Extract module files
ui_print "- Extracting module files"
extract "$ZIPFILE" 'module.prop' "$MODPATH"
extract "$ZIPFILE" 'service.sh' "$MODPATH"
extract "$ZIPFILE" 'uninstall.sh' "$MODPATH"
extract "$ZIPFILE" 'action.sh' "$MODPATH"
extract "$ZIPFILE" 'system/bin/encore_profiler' "$MODPATH"
extract "$ZIPFILE" 'system/bin/encore_utility' "$MODPATH"

# Target architecture
case $ARCH in
"arm64") ARCH_TMP="arm64-v8a" ;;
"arm") ARCH_TMP="armeabi-v7a" ;;
"x64") ARCH_TMP="x86_64" ;;
"x86") ARCH_TMP="x86" ;;
"riscv64") ARCH_TMP="riscv64" ;;
*) abort_unsupported_arch ;;
esac

# Extract executables
extract "$ZIPFILE" "libs/$ARCH_TMP/encored" "$TMPDIR"
cp "$TMPDIR"/libs/"$ARCH_TMP"/* "$MODPATH/system/bin"
rm -rf "$TMPDIR/libs"

if [ "$KSU" = "true" ] || [ "$APATCH" = "true" ]; then
	# remove action on APatch / KernelSU
	rm "$MODPATH/action.sh"
	# skip mount on APatch / KernelSU
	touch "$MODPATH/skip_mount"
	ui_print "- KSU/AP Detected, skipping module mount (skip_mount)"
	# symlink ourselves on $PATH
	manager_paths="/data/adb/ap/bin /data/adb/ksu/bin"
	BIN_PATH="/data/adb/modules/encore/system/bin"
	for dir in $manager_paths; do
		[ -d "$dir" ] && {
			ui_print "- Creating symlink in $dir"
			ln -sf "$BIN_PATH/encored" "$dir/encored"
			ln -sf "$BIN_PATH/encore_profiler" "$dir/encore_profiler"
			ln -sf "$BIN_PATH/encore_utility" "$dir/encore_utility"
		}
	done
fi

# Extract webroot
ui_print "- Extracting webroot"
unzip -o "$ZIPFILE" "webroot/*" -d "$MODPATH" >&2

# Set configs
ui_print "- Encore Tweaks configuration setup"
make_dir /data/encore
make_node 0 /data/encore/lite_mode
make_node 0 /data/encore/dnd_gameplay
make_node 0 /data/encore/device_mitigation
[ ! -f /data/encore/ppm_policies_mediatek ] && echo 'PWR_THRO|THERMAL' >/data/encore/ppm_policies_mediatek
[ ! -f /data/encore/gamelist.txt ] && extract "$ZIPFILE" 'gamelist.txt' "/data/encore"
extract "$ZIPFILE" 'encore_logo.png' "/data/local/tmp"
touch /data/encore/_files_on_this_directory_is_critical_for_encore_module__please_DO_NOT_REMOVE_OR_MODIFY

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
# 8 = Kirin

# Recognize Chipset
soc_recognition_extra
[ $SOC -eq 0 ] && recognize_soc "$(get_soc_getprop)"
[ $SOC -eq 0 ] && recognize_soc "$(grep -E "Hardware|Processor" /proc/cpuinfo | uniq | cut -d ':' -f 2 | sed 's/^[ \t]*//')"
[ $SOC -eq 0 ] && recognize_soc "$(grep "model\sname" /proc/cpuinfo | uniq | cut -d ':' -f 2 | sed 's/^[ \t]*//')"
[ $SOC -eq 0 ] && {
	ui_print "! Unknown SoC, skipping some tweaks"
	ui_print "! If you think this is wrong, please report to maintainer"
}

echo $SOC >/data/encore/soc_recognition

# Easter Egg
case "$((RANDOM % 8 + 1))" in
1) ui_print "- Wooly's Fairy Tale" ;;
2) ui_print "- Sheep-counting Lullaby" ;;
3) ui_print "- Fog? The Black Shores!" ;;
4) ui_print "- Adventure? Let's go!" ;;
5) ui_print "- Hero Takes the Stage!" ;;
6) ui_print "- Woolies Save the World!" ;;
7) ui_print "- How much people will let you live for Encore?" ;;
8) ui_print "- Wen Donate?" ;;
esac
