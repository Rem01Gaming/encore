#!/system/bin/sh
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

change_cpu_gov() {
	chmod 644 /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
	echo "$1" | tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
	chmod 444 /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
}

set_dnd() {
	case $1 in
	# Turn off DND mode
	0) cmd notification set_dnd off ;;
	# Turn on DND mode
	1) cmd notification set_dnd priority ;;
	esac
}

save_logs() {
	[ ! -d /sdcard/Download ] && mkdir /sdcard/Download
	log_file="/sdcard/Download/encore_bugreport_$(date +"%Y-%m-%d_%H_%M").txt"
	SOC="Unknown"

	case $(</data/encore/soc_recognition) in
	1) SOC="MediaTek" ;;
	2) SOC="Snapdragon" ;;
	3) SOC="Exynos" ;;
	4) SOC="Unisoc" ;;
	5) SOC="Tensor" ;;
	6) SOC="Intel" ;;
	7) SOC="Tegra" ;;
	8) SOC="Kirin" ;;
	esac

	echo "$log_file"
	cat <<EOF >"$log_file"
*****************************************************
Encore Tweaks Log

Module Version: $(awk -F'=' '/version=/ {print $2}' /data/adb/modules/encore/module.prop)
Chipset: $SOC $(getprop ro.board.platform)
Fingerprint: $(getprop ro.build.fingerprint)
Android SDK: $(getprop ro.build.version.sdk)
Kernel: $(uname -r -m)
*****************************************************

$(</dev/encore_log)
EOF
}

# shellcheck disable=SC2068
$@
