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

logcat() {
	# Clear screen
	echo -ne "\e[H\e[2J\e[3J"

	# Trap CTRL+C and exit gracefully
	trap 'echo -ne "\e[H\e[2J\e[3J"; exit 0' INT

	# Detect SoC
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

	# Header
	echo -e "\e[1;36m┌────────────────────────────────────────────┐"
	echo -e "│          \e[1;37mEncore Tweaks Log Viewer\e[1;36m          │"
	echo -e "└────────────────────────────────────────────┘\e[0m"

	# Info block
	echo -e "
\e[1;32mModule Version:\e[0m $(awk -F'=' '/version=/ {print $2}' /data/adb/modules/encore/module.prop)
\e[1;32mChipset:\e[0m        $SOC $(getprop ro.board.platform)
\e[1;32mFingerprint:\e[0m    $(getprop ro.build.fingerprint)
\e[1;32mAndroid SDK:\e[0m    $(getprop ro.build.version.sdk)
\e[1;32mKernel:\e[0m         $(uname -r -m)

\e[1;33m[Log Stream Started — press CTRL+C to exit]\e[0m
"

	# Tail log
	tail -f /dev/encore_log | while read -r line; do
		timestamp="${line:0:23}"
		level_char=$(echo "$line" | awk '{print $3}')
		msg="${line:24}"

		# Set color based on level
		case "$level_char" in
		W) level_color="\e[1;33m" ;; # Yellow
		E) level_color="\e[1;31m" ;; # Red
		F) level_color="\e[1;31m" ;; # Red
		*) level_color="\e[0m" ;;    # Default
		esac

		echo -e "\e[1;32m$timestamp\e[0m ${level_color}${msg}\e[0m"
	done
}

# shellcheck disable=SC2068
$@
