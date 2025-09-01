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

# shellcheck disable=SC2013

###################################
# Variables
###################################

# Config dir
MODULE_CONFIG="/data/adb/.config/encore"

# SoC recognition
SOC=$(<$MODULE_CONFIG/soc_recognition)

# Lite mode
LITE_MODE=$(<$MODULE_CONFIG/lite_mode)

# PPM policies settings for MediaTek devices
PPM_POLICY=$(<$MODULE_CONFIG/ppm_policies_mediatek)

# Default CPU Governor
if [ -f $MODULE_CONFIG/custom_default_cpu_gov ]; then
	DEFAULT_CPU_GOV="$(<$MODULE_CONFIG/custom_default_cpu_gov)"
else
	DEFAULT_CPU_GOV="$(<$MODULE_CONFIG/default_cpu_gov)"
fi

# Device specific bug workaround
DEVICE_MITIGATION="$(<$MODULE_CONFIG/device_mitigation)"

###################################
# Common Function
###################################

apply() {
	[ ! -f "$2" ] && return 1
	chmod 644 "$2" >/dev/null 2>&1
	echo "$1" >"$2" 2>/dev/null
	chmod 444 "$2" >/dev/null 2>&1
}

write() {
	[ ! -f "$2" ] && return 1
	chmod 644 "$2" >/dev/null 2>&1
	echo "$1" >"$2" 2>/dev/null
}

change_cpu_gov() {
	chmod 644 /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
	echo "$1" | tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor >/dev/null
	chmod 444 /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
	chmod 444 /sys/devices/system/cpu/cpufreq/policy*/scaling_governor
}

set_dnd() {
	case $1 in
	# Turn off DND mode
	0) cmd notification set_dnd off ;;
	# Turn on DND mode
	1) cmd notification set_dnd priority ;;
	esac
}

###################################
# Frequency fetching
###################################

which_maxfreq() {
	tr ' ' '\n' <"$1" | sort -nr | head -n 1
}

which_minfreq() {
	tr ' ' '\n' <"$1" | grep -v '^[[:space:]]*$' | sort -n | head -n 1
}

which_midfreq() {
	total_opp=$(wc -w <"$1")
	mid_opp=$(((total_opp + 1) / 2))
	tr ' ' '\n' <"$1" | grep -v '^[[:space:]]*$' | sort -nr | head -n $mid_opp | tail -n 1
}

# MediaTek gpufreq
# Returns OPP index of the frequency

mtk_gpufreq_minfreq_index() {
	awk -F'[][]' '{print $2}' "$1" | tail -n 1
}

mtk_gpufreq_midfreq_index() {
	total_opp=$(wc -l <"$1")
	mid_opp=$(((total_opp + 1) / 2))
	awk -F'[][]' '{print $2}' "$1" | head -n $mid_opp | tail -n 1
}

###################################
# Yamada Line 
###################################

yamada_cpufreq_mid_perf() {
    for path in /sys/devices/system/cpu/*/cpufreq; do
	    mid_freq=$(which_midfreq "$path/scaling_available_frequencies")
		apply "$mid_freq" "$path/scaling_min_freq"
    done
}

yamada_cpufreq_ppm_mid_perf() {
    cluster=0
    for path in /sys/devices/system/cpu/cpufreq/policy*; do
        mid_freq=$(which_midfreq "$path/scaling_available_frequencies")
        apply "$cluster $mid_freq" "/proc/ppm/policy/hard_userlimit_min_cpu_freq"
        ((cluster++))
    done
}

###################################
# Frequency settings
###################################

cpufreq_ppm_max_perf() {
	if [ $LITE_MODE -eq 1 ]; then
		yamada_cpufreq_ppm_mid_perf
		cluster=-1
		for path in /sys/devices/system/cpu/cpufreq/policy*; do
			((cluster++))
			cpu_maxfreq=$(<"$path/cpuinfo_max_freq")
			apply "$cluster $cpu_maxfreq" /proc/ppm/policy/hard_userlimit_max_cpu_freq
		done
	else
		cluster=-1
		for path in /sys/devices/system/cpu/cpufreq/policy*; do
			((cluster++))
			cpu_maxfreq=$(<"$path/cpuinfo_max_freq")
			apply "$cluster $cpu_maxfreq" /proc/ppm/policy/hard_userlimit_max_cpu_freq
			apply "$cluster $cpu_maxfreq" /proc/ppm/policy/hard_userlimit_min_cpu_freq
		done
	fi
}


cpufreq_max_perf() {
	if [ $LITE_MODE -eq 1 ]; then
		yamada_cpufreq_mid_perf
		for path in /sys/devices/system/cpu/*/cpufreq; do
			cpu_maxfreq=$(<"$path/cpuinfo_max_freq")
			apply "$cpu_maxfreq" "$path/scaling_max_freq"
		done
	else
		for path in /sys/devices/system/cpu/*/cpufreq; do
			cpu_maxfreq=$(<"$path/cpuinfo_max_freq")
			apply "$cpu_maxfreq" "$path/scaling_max_freq"
			apply "$cpu_maxfreq" "$path/scaling_min_freq"
		done
	fi
	chmod -f 444 /sys/devices/system/cpu/cpufreq/policy*/scaling_*_freq
}

cpufreq_ppm_unlock() {
	cluster=0
	for path in /sys/devices/system/cpu/cpufreq/policy*; do
		cpu_maxfreq=$(<"$path/cpuinfo_max_freq")
		cpu_minfreq=$(<"$path/cpuinfo_min_freq")
		write "$cluster $cpu_maxfreq" /proc/ppm/policy/hard_userlimit_max_cpu_freq
		write "$cluster $cpu_minfreq" /proc/ppm/policy/hard_userlimit_min_cpu_freq
		((cluster++))
	done
}

cpufreq_unlock() {
	for path in /sys/devices/system/cpu/*/cpufreq; do
		cpu_maxfreq=$(<"$path/cpuinfo_max_freq")
		cpu_minfreq=$(<"$path/cpuinfo_min_freq")
		write "$cpu_maxfreq" "$path/scaling_max_freq"
		write "$cpu_minfreq" "$path/scaling_min_freq"
	done
	chmod -f 644 /sys/devices/system/cpu/cpufreq/policy*/scaling_*_freq
}

devfreq_max_perf() {
	[ ! -f "$1/available_frequencies" ] && return 1
	max_freq=$(which_maxfreq "$1/available_frequencies")
	apply "$max_freq" "$1/max_freq"
	apply "$max_freq" "$1/min_freq"
}

devfreq_mid_perf() {
	[ ! -f "$1/available_frequencies" ] && return 1
	max_freq=$(which_maxfreq "$1/available_frequencies")
	mid_freq=$(which_midfreq "$1/available_frequencies")
	apply "$max_freq" "$1/max_freq"
	apply "$mid_freq" "$1/min_freq"
}

devfreq_unlock() {
	[ ! -f "$1/available_frequencies" ] && return 1
	max_freq=$(which_maxfreq "$1/available_frequencies")
	min_freq=$(which_minfreq "$1/available_frequencies")
	write "$max_freq" "$1/max_freq"
	write "$min_freq" "$1/min_freq"
}

devfreq_min_perf() {
	[ ! -f "$1/available_frequencies" ] && return 1
	freq=$(which_minfreq "$1/available_frequencies")
	apply "$freq" "$1/min_freq"
	apply "$freq" "$1/max_freq"
}

qcom_cpudcvs_max_perf() {
	[ ! -f "$1/available_frequencies" ] && return 1
	freq=$(which_maxfreq "$1/available_frequencies")
	apply "$freq" "$1/hw_max_freq"
	apply "$freq" "$1/hw_min_freq"
}

qcom_cpudcvs_mid_perf() {
	[ ! -f "$1/available_frequencies" ] && return 1
	max_freq=$(which_maxfreq "$1/available_frequencies")
	mid_freq=$(which_midfreq "$1/available_frequencies")
	apply "$max_freq" "$1/hw_max_freq"
	apply "$mid_freq" "$1/hw_min_freq"
}

qcom_cpudcvs_unlock() {
	[ ! -f "$1/available_frequencies" ] && return 1
	max_freq=$(which_maxfreq "$1/available_frequencies")
	min_freq=$(which_minfreq "$1/available_frequencies")
	write "$max_freq" "$1/hw_max_freq"
	write "$min_freq" "$1/hw_min_freq"
}

qcom_cpudcvs_min_perf() {
	[ ! -f "$1/available_frequencies" ] && return 1
	freq=$(which_minfreq "$1/available_frequencies")
	apply "$freq" "$1/hw_min_freq"
	apply "$freq" "$1/hw_max_freq"
}

###################################
# Device-specific performance profile
###################################

mediatek_performance() {
	# PPM policies
	if [ -d /proc/ppm ]; then
		grep -E "$PPM_POLICY" /proc/ppm/policy_status | while read -r row; do
			apply "${row:1:1} 0" /proc/ppm/policy_status
		done
	fi

	# Force off FPSGO
	apply 0 /sys/kernel/fpsgo/common/force_onoff

	# MTK Power and CCI mode
	apply 1 /proc/cpufreq/cpufreq_cci_mode
	apply 3 /proc/cpufreq/cpufreq_power_mode

	# DDR Boost mode
	apply 1 /sys/devices/platform/boot_dramboost/dramboost/dramboost

	# EAS/HMP Switch
	apply 0 /sys/devices/system/cpu/eas/enable

	# Disable GED KPI
	apply 0 /sys/module/sspm_v3/holders/ged/parameters/is_GED_KPI_enabled

	# GPU Frequency
	if [ $LITE_MODE -eq 0 ]; then
		if [ -d /proc/gpufreqv2 ]; then
			apply 0 /proc/gpufreqv2/fix_target_opp_index
		else
			gpu_freq=$(sed -n 's/.*freq = \([0-9]\{1,\}\).*/\1/p' /proc/gpufreq/gpufreq_opp_dump | head -n 1)
			apply "$gpu_freq" /proc/gpufreq/gpufreq_opp_freq
		fi
	else
		apply 0 /proc/gpufreq/gpufreq_opp_freq
		apply -1 /proc/gpufreqv2/fix_target_opp_index

		# Set min freq via GED
		if [ -d /proc/gpufreqv2 ]; then
			mid_oppfreq=$(mtk_gpufreq_midfreq_index /proc/gpufreqv2/gpu_working_opp_table)
		else
			mid_oppfreq=$(mtk_gpufreq_midfreq_index /proc/gpufreq/gpufreq_opp_dump)
		fi

		apply $mid_oppfreq /sys/kernel/ged/hal/custom_boost_gpu_freq
	fi

	# Disable GPU Power limiter
	[ -f "/proc/gpufreq/gpufreq_power_limited" ] && {
		for setting in ignore_batt_oc ignore_batt_percent ignore_low_batt ignore_thermal_protect ignore_pbm_limited; do
			apply "$setting 1" /proc/gpufreq/gpufreq_power_limited
		done
	}

	# Disable battery current limiter
	apply "stop 1" /proc/mtk_batoc_throttling/battery_oc_protect_stop

	# DRAM Frequency
	if [ $LITE_MODE -eq 0 ]; then
		apply 0 /sys/devices/platform/10012000.dvfsrc/helio-dvfsrc/dvfsrc_req_ddr_opp
		apply 0 /sys/kernel/helio-dvfsrc/dvfsrc_force_vcore_dvfs_opp
		devfreq_max_perf /sys/class/devfreq/mtk-dvfsrc-devfreq
	else
		apply -1 /sys/devices/platform/10012000.dvfsrc/helio-dvfsrc/dvfsrc_req_ddr_opp
		apply -1 /sys/kernel/helio-dvfsrc/dvfsrc_force_vcore_dvfs_opp
		devfreq_mid_perf /sys/class/devfreq/mtk-dvfsrc-devfreq
	fi

	# Eara Thermal
	apply 0 /sys/kernel/eara_thermal/enable
}

snapdragon_performance() {
	# Qualcomm CPU Bus and DRAM frequencies
	[ $DEVICE_MITIGATION -eq 0 ] && {
		for path in /sys/class/devfreq/*cpu*-lat \
			/sys/class/devfreq/*cpu*-bw \
			/sys/class/devfreq/*llccbw* \
			/sys/class/devfreq/*bus_llcc* \
			/sys/class/devfreq/*bus_ddr* \
			/sys/class/devfreq/*memlat* \
			/sys/class/devfreq/*cpubw* \
			/sys/class/devfreq/*kgsl-ddr-qos*; do

			[ $LITE_MODE -eq 1 ] &&
				devfreq_mid_perf "$path" ||
				devfreq_max_perf "$path"
		done &

		for component in DDR LLCC L3; do
			path="/sys/devices/system/cpu/bus_dcvs/$component"
			[ "$LITE_MODE" -eq 1 ] &&
				qcom_cpudcvs_mid_perf "$path" ||
				qcom_cpudcvs_max_perf "$path"
		done &
	}

	# GPU tweak
	gpu_path="/sys/class/kgsl/kgsl-3d0/devfreq"
	[ "$LITE_MODE" -eq 0 ] && devfreq_max_perf "$gpu_path" || devfreq_mid_perf "$gpu_path"

	# Disable GPU Bus split
	apply 0 /sys/class/kgsl/kgsl-3d0/bus_split

	# Force GPU clock on
	apply 1 /sys/class/kgsl/kgsl-3d0/force_clk_on
}

tegra_performance() {
	gpu_path="/sys/kernel/tegra_gpu"
	if [ -d "$gpu_path" ]; then
		max_freq=$(which_maxfreq "$gpu_path/available_frequencies")
		apply "$max_freq" "$gpu_path/gpu_cap_rate"

		if [ $LITE_MODE -eq 1 ]; then
			mid_freq=$(which_midfreq "$gpu_path/available_frequencies")
			apply "$mid_freq" "$gpu_path/gpu_floor_rate"
		else
			apply "$max_freq" "$gpu_path/gpu_floor_rate"
		fi
	fi
}

exynos_performance() {
	# GPU Frequency
	gpu_path="/sys/kernel/gpu"
	[ -d "$gpu_path" ] && {
		max_freq=$(which_maxfreq "$gpu_path/gpu_available_frequencies")
		apply "$max_freq" "$gpu_path/gpu_max_clock"

		if [ $LITE_MODE -eq 1 ]; then
			mid_freq=$(which_midfreq "$gpu_path/gpu_available_frequencies")
			apply "$mid_freq" "$gpu_path/gpu_min_clock"
		else
			apply "$max_freq" "$gpu_path/gpu_min_clock"
		fi
	}

	mali_sysfs=$(find /sys/devices/platform/ -iname "*.mali" -print -quit 2>/dev/null)
	apply always_on "$mali_sysfs/power_policy"

	# DRAM and Buses Frequency
	[ $DEVICE_MITIGATION -eq 0 ] && {
		for path in /sys/class/devfreq/*devfreq_mif*; do
			[ $LITE_MODE -eq 1 ] &&
				devfreq_mid_perf "$path" ||
				devfreq_max_perf "$path"
		done &
	}
}

unisoc_performance() {
	# GPU Frequency
	gpu_path=$(find /sys/class/devfreq/ -type d -iname "*.gpu" -print -quit 2>/dev/null)
	[ -n "$gpu_path" ] && {
		if [ $LITE_MODE -eq 0 ]; then
			devfreq_max_perf "$gpu_path"
		else
			devfreq_mid_perf "$gpu_path"
		fi
	}
}

tensor_performance() {
	# GPU Frequency
	gpu_path=$(find /sys/devices/platform/ -type d -iname "*.mali" -print -quit 2>/dev/null)
	[ -n "$gpu_path" ] && {
		max_freq=$(which_maxfreq "$gpu_path/available_frequencies")
		apply "$max_freq" "$gpu_path/scaling_max_freq"

		if [ $LITE_MODE -eq 1 ]; then
			mid_freq=$(which_midfreq "$gpu_path/available_frequencies")
			apply "$mid_freq" "$gpu_path/scaling_min_freq"
		else
			apply "$max_freq" "$gpu_path/scaling_min_freq"
		fi
	}

	# DRAM frequency
	[ $DEVICE_MITIGATION -eq 0 ] && {
		for path in /sys/class/devfreq/*devfreq_mif*; do
			[ $LITE_MODE -eq 1 ] &&
				devfreq_mid_perf "$path" ||
				devfreq_max_perf "$path"
		done &
	}
}

intel_performance() {
	return 0 # some tweaks soon
}

###################################
# Device-specific normal profile
###################################

mediatek_normal() {
	# PPM policies
	if [ -d /proc/ppm ]; then
		grep -E "$PPM_POLICY" /proc/ppm/policy_status | while read -r row; do
			apply "${row:1:1} 1" /proc/ppm/policy_status
		done
	fi

	# Free FPSGO
	apply 2 /sys/kernel/fpsgo/common/force_onoff

	# MTK Power and CCI mode
	apply 0 /proc/cpufreq/cpufreq_cci_mode
	apply 0 /proc/cpufreq/cpufreq_power_mode

	# DDR Boost mode
	apply 0 /sys/devices/platform/boot_dramboost/dramboost/dramboost

	# EAS/HMP Switch
	apply 2 /sys/devices/system/cpu/eas/enable

	# Enable GED KPI
	apply 1 /sys/module/sspm_v3/holders/ged/parameters/is_GED_KPI_enabled

	# GPU Frequency
	write 0 /proc/gpufreq/gpufreq_opp_freq
	write -1 /proc/gpufreqv2/fix_target_opp_index

	# Reset min freq via GED
	if [ -d /proc/gpufreqv2 ]; then
		mid_oppfreq=$(mtk_gpufreq_minfreq_index /proc/gpufreqv2/gpu_working_opp_table)
	else
		min_oppfreq=$(mtk_gpufreq_minfreq_index /proc/gpufreq/gpufreq_opp_dump)
	fi

	apply $min_oppfreq /sys/kernel/ged/hal/custom_boost_gpu_freq

	# GPU Power limiter
	[ -f "/proc/gpufreq/gpufreq_power_limited" ] && {
		for setting in ignore_batt_oc ignore_batt_percent ignore_low_batt ignore_thermal_protect ignore_pbm_limited; do
			apply "$setting 0" /proc/gpufreq/gpufreq_power_limited
		done
	}

	# Enable battery current limiter
	apply "stop 0" /proc/mtk_batoc_throttling/battery_oc_protect_stop

	# DRAM Frequency
	write -1 /sys/devices/platform/10012000.dvfsrc/helio-dvfsrc/dvfsrc_req_ddr_opp
	write -1 /sys/kernel/helio-dvfsrc/dvfsrc_force_vcore_dvfs_opp
	devfreq_unlock /sys/class/devfreq/mtk-dvfsrc-devfreq

	# Eara Thermal
	apply 1 /sys/kernel/eara_thermal/enable
}

snapdragon_normal() {
	# Qualcomm CPU Bus and DRAM frequencies
	[ $DEVICE_MITIGATION -eq 0 ] && {
		for path in /sys/class/devfreq/*cpu*-lat \
			/sys/class/devfreq/*cpu*-bw \
			/sys/class/devfreq/*llccbw* \
			/sys/class/devfreq/*bus_llcc* \
			/sys/class/devfreq/*bus_ddr* \
			/sys/class/devfreq/*memlat* \
			/sys/class/devfreq/*cpubw* \
			/sys/class/devfreq/*kgsl-ddr-qos*; do

			devfreq_unlock "$path"
		done &

		for component in DDR LLCC L3; do
			qcom_cpudcvs_unlock /sys/devices/system/cpu/bus_dcvs/$component
		done
	}

	# Revert GPU tweak
	devfreq_unlock /sys/class/kgsl/kgsl-3d0/devfreq

	# Enable back GPU Bus split
	apply 1 /sys/class/kgsl/kgsl-3d0/bus_split

	# Free GPU clock on/off
	apply 0 /sys/class/kgsl/kgsl-3d0/force_clk_on
}

tegra_normal() {
	gpu_path="/sys/kernel/tegra_gpu"
	[ -d "$gpu_path" ] && {
		max_freq=$(which_maxfreq "$gpu_path/available_frequencies")
		min_freq=$(which_minfreq "$gpu_path/available_frequencies")
		write "$max_freq" "$gpu_path/gpu_cap_rate"
		write "$min_freq" "$gpu_path/gpu_floor_rate"
	}
}

exynos_normal() {
	# GPU Frequency
	gpu_path="/sys/kernel/gpu"
	[ -d "$gpu_path" ] && {
		max_freq=$(which_maxfreq "$gpu_path/gpu_available_frequencies")
		min_freq=$(which_minfreq "$gpu_path/gpu_available_frequencies")
		write "$max_freq" "$gpu_path/gpu_max_clock"
		write "$min_freq" "$gpu_path/gpu_min_clock"
	}

	mali_sysfs=$(find /sys/devices/platform/ -iname "*.mali" -print -quit 2>/dev/null)
	apply coarse_demand "$mali_sysfs/power_policy"

	# DRAM frequency
	[ $DEVICE_MITIGATION -eq 0 ] && {
		for path in /sys/class/devfreq/*devfreq_mif*; do
			devfreq_unlock "$path"
		done &
	}
}

unisoc_normal() {
	# GPU Frequency
	gpu_path=$(find /sys/class/devfreq/ -type d -iname "*.gpu" -print -quit 2>/dev/null)
	[ -n "$gpu_path" ] && devfreq_unlock "$gpu_path"
}

tensor_normal() {
	# GPU Frequency
	gpu_path=$(find /sys/devices/platform/ -type d -iname "*.mali" -print -quit 2>/dev/null)
	[ -n "$gpu_path" ] && {
		max_freq=$(which_maxfreq "$gpu_path/available_frequencies")
		min_freq=$(which_minfreq "$gpu_path/available_frequencies")
		write "$max_freq" "$gpu_path/scaling_max_freq"
		write "$min_freq" "$gpu_path/scaling_min_freq"
	}

	# DRAM frequency
	[ $DEVICE_MITIGATION -eq 0 ] && {
		for path in /sys/class/devfreq/*devfreq_mif*; do
			devfreq_unlock "$path"
		done &
	}
}

intel_normal() {
	return 0 # some tweaks soon
}

###################################
# Device-specific powersave profile
###################################

mediatek_powersave() {
	# MTK CPU Power mode to low power
	apply 1 /proc/cpufreq/cpufreq_power_mode

	# GPU Frequency
	if [ -d /proc/gpufreqv2 ]; then
		min_gpufreq_index=$(mtk_gpufreq_minfreq_index /proc/gpufreqv2/gpu_working_opp_table)
		apply "$min_gpufreq_index" /proc/gpufreqv2/fix_target_opp_index
	else
		gpu_freq=$(sed -n 's/.*freq = \([0-9]\{1,\}\).*/\1/p' /proc/gpufreq/gpufreq_opp_dump | tail -n 1)
		apply "$gpu_freq" /proc/gpufreq/gpufreq_opp_freq
	fi
}

snapdragon_powersave() {
	# GPU Frequency
	devfreq_min_perf /sys/class/kgsl/kgsl-3d0/devfreq
}

tegra_powersave() {
	gpu_path="/sys/kernel/tegra_gpu"
	[ -d "$gpu_path" ] && {
		freq=$(which_minfreq "$gpu_path/available_frequencies")
		apply "$freq" "$gpu_path/gpu_floor_rate"
		apply "$freq" "$gpu_path/gpu_cap_rate"
	}
}

exynos_powersave() {
	# GPU Frequency
	gpu_path="/sys/kernel/gpu"
	[ -d "$gpu_path" ] && {
		freq=$(which_minfreq "$gpu_path/gpu_available_frequencies")
		apply "$freq" "$gpu_path/gpu_min_clock"
		apply "$freq" "$gpu_path/gpu_max_clock"
	}
}

unisoc_powersave() {
	# GPU Frequency
	gpu_path=$(find /sys/class/devfreq/ -type d -iname "*.gpu" -print -quit 2>/dev/null)
	[ -n "$gpu_path" ] && devfreq_min_perf "$gpu_path"
}

tensor_powersave() {
	# GPU Frequency
	gpu_path=$(find /sys/devices/platform/ -type d -iname "*.mali" -print -quit 2>/dev/null)
	[ -n "$gpu_path" ] && {
		freq=$(which_minfreq "$gpu_path/available_frequencies")
		apply "$freq" "$gpu_path/scaling_min_freq"
		apply "$freq" "$gpu_path/scaling_max_freq"
	}
}

intel_powersave() {
	return 0 # some tweaks soon
}

###################################
# Main Performance scripts
###################################

perfcommon() {
	# Disable Kernel panic
	# Workaround for kernel panic on startup in S25U.
	# This is wrong, you know it and I know it.
	# Move on and call me an idiot later.
	apply 0 /proc/sys/kernel/panic
	apply 0 /proc/sys/kernel/panic_on_oops
	apply 0 /proc/sys/kernel/panic_on_warn
	apply 0 /proc/sys/kernel/softlockup_panic

	# Sync to data in the rare case a device crashes
	sync

	# Push Notification
	su -lp 2000 -c "/system/bin/cmd notification post -t 'Encore Tweaks' 'encore' 'Tweaks successfully applied' </dev/null 2>&1 | cat" >/dev/null

	# I/O Tweaks
	for dir in /sys/block/*; do
		# Disable I/O statistics accounting
		apply 0 "$dir/queue/iostats"

		# Don't use I/O as random spice
		apply 0 "$dir/queue/add_random"
	done &

	# Networking tweaks
	for algo in bbr3 bbr2 bbrplus bbr westwood cubic; do
		if grep -q "$algo" /proc/sys/net/ipv4/tcp_available_congestion_control; then
			apply "$algo" /proc/sys/net/ipv4/tcp_congestion_control
			break
		fi
	done

	apply 1 /proc/sys/net/ipv4/tcp_low_latency
	apply 1 /proc/sys/net/ipv4/tcp_ecn
	apply 3 /proc/sys/net/ipv4/tcp_fastopen
	apply 1 /proc/sys/net/ipv4/tcp_sack
	apply 0 /proc/sys/net/ipv4/tcp_timestamps

	# Limit max perf event processing time to this much CPU usage
	apply 3 /proc/sys/kernel/perf_cpu_time_max_percent

	# Disable schedstats
	apply 0 /proc/sys/kernel/sched_schedstats

	# Disable Oppo/Realme cpustats
	apply 0 /proc/sys/kernel/task_cpustats_enable

	# Disable Sched auto group
	apply 0 /proc/sys/kernel/sched_autogroup_enabled

	# Enable CRF
	apply 1 /proc/sys/kernel/sched_child_runs_first

	# Improve real time latencies by reducing the scheduler migration time
	apply 32 /proc/sys/kernel/sched_nr_migrate

	# Tweaking scheduler to reduce latency
	apply 50000 /proc/sys/kernel/sched_migration_cost_ns
	apply 1000000 /proc/sys/kernel/sched_min_granularity_ns
	apply 1500000 /proc/sys/kernel/sched_wakeup_granularity_ns

	# Disable read-ahead for swap devices
	apply 0 /proc/sys/vm/page-cluster

	# Update /proc/stat less often to reduce jitter
	apply 15 /proc/sys/vm/stat_interval

	# Disable compaction_proactiveness
	apply 0 /proc/sys/vm/compaction_proactiveness

	# Disable SPI CRC
	apply 0 /sys/module/mmc_core/parameters/use_spi_crc

	# Disable OnePlus opchain
	apply 0 /sys/module/opchain/parameters/chain_on

	# Disable Oplus bloats
	apply 0 /sys/module/cpufreq_bouncing/parameters/enable
	apply 0 /proc/task_info/task_sched_info/task_sched_info_enable
	apply 0 /proc/oplus_scheduler/sched_assist/sched_assist_enabled

	# Report max CPU capabilities to these libraries
	apply "libunity.so, libil2cpp.so, libmain.so, libUE4.so, libgodot_android.so, libgdx.so, libgdx-box2d.so, libminecraftpe.so, libLive2DCubismCore.so, libyuzu-android.so, libryujinx.so, libcitra-android.so, libhdr_pro_engine.so, libandroidx.graphics.path.so, libeffect.so" /proc/sys/kernel/sched_lib_name
	apply 255 /proc/sys/kernel/sched_lib_mask_force

	# Set thermal governor to step_wise
	for dir in /sys/class/thermal/thermal_zone*; do
		apply "step_wise" "$dir/policy"
	done
}

performance_profile() {
	# Enable Do not Disturb
	[ "$(<$MODULE_CONFIG/dnd_gameplay)" -eq 1 ] && set_dnd 1

	# Disable battery saver module
	[ -f /sys/module/battery_saver/parameters/enabled ] && {
		if grep -qo '[0-9]\+' /sys/module/battery_saver/parameters/enabled; then
			apply 0 /sys/module/battery_saver/parameters/enabled
		else
			apply N /sys/module/battery_saver/parameters/enabled
		fi
	}

	# Disable split lock mitigation
	apply 0 /proc/sys/kernel/split_lock_mitigate

	if [ -f "/sys/kernel/debug/sched_features" ]; then
		# Consider scheduling tasks that are eager to run
		apply NEXT_BUDDY /sys/kernel/debug/sched_features

		# Some sources report large latency spikes during large migrations
		apply NO_TTWU_QUEUE /sys/kernel/debug/sched_features
	fi

	if [ -d "/dev/stune/" ]; then
		# Prefer to schedule top-app tasks on idle CPUs
		apply 1 /dev/stune/top-app/schedtune.prefer_idle

		# Mark top-app as boosted, find high-performing CPUs
		apply 1 /dev/stune/top-app/schedtune.boost
	fi

	# Oppo/Oplus/Realme Touchpanel
	tp_path="/proc/touchpanel"
	if [ -d "$tp_path" ]; then
		apply 1 $tp_path/game_switch_enable
		apply 0 $tp_path/oplus_tp_limit_enable
		apply 0 $tp_path/oppo_tp_limit_enable
		apply 1 $tp_path/oplus_tp_direction
		apply 1 $tp_path/oppo_tp_direction
	fi

	# Memory tweak
	apply 80 /proc/sys/vm/vfs_cache_pressure

	# eMMC and UFS frequency
	for path in /sys/class/devfreq/*.ufshc \
		/sys/class/devfreq/mmc*; do

		[ $LITE_MODE -eq 1 ] &&
			devfreq_mid_perf "$path" ||
			devfreq_max_perf "$path"
	done &

	# Set CPU governor to performance.
	# performance governor in this case is only used for "flex"
	# since the frequencies already maxed out (ifykyk).
	# If lite mode enabled, use the default governor instead.
	# device mitigation also will prevent performance gov to be
	# applied (some device hates performance governor).
	[ $LITE_MODE -eq 0 ] && [ $DEVICE_MITIGATION -eq 0 ] &&
		change_cpu_gov performance ||
		change_cpu_gov "$DEFAULT_CPU_GOV"

	# Force CPU to highest possible frequency.
	[ -d /proc/ppm ] && cpufreq_ppm_max_perf || cpufreq_max_perf

	# I/O Tweaks
	for dir in /sys/block/mmcblk0 /sys/block/mmcblk1 /sys/block/sd*; do
		# Reduce heuristic read-ahead in exchange for I/O latency
		apply 32 "$dir/queue/read_ahead_kb"

		# Reduce the maximum number of I/O requests in exchange for latency
		apply 32 "$dir/queue/nr_requests"
	done &

	case $SOC in
	1) mediatek_performance ;;
	2) snapdragon_performance ;;
	3) exynos_performance ;;
	4) unisoc_performance ;;
	5) tensor_performance ;;
	6) intel_performance ;;
	7) tegra_performance ;;
	esac

	echo 3 >/proc/sys/vm/drop_caches
}

normal_profile() {
	[ "$(<$MODULE_CONFIG/dnd_gameplay)" -eq 1 ] && set_dnd 0

	# Disable battery saver module
	[ -f /sys/module/battery_saver/parameters/enabled ] && {
		if grep -qo '[0-9]\+' /sys/module/battery_saver/parameters/enabled; then
			apply 0 /sys/module/battery_saver/parameters/enabled
		else
			apply N /sys/module/battery_saver/parameters/enabled
		fi
	}

	# Enable split lock mitigation
	apply 1 /proc/sys/kernel/split_lock_mitigate

	if [ -f "/sys/kernel/debug/sched_features" ]; then
		# Consider scheduling tasks that are eager to run
		apply NEXT_BUDDY /sys/kernel/debug/sched_features

		# Schedule tasks on their origin CPU if possible
		apply TTWU_QUEUE /sys/kernel/debug/sched_features
	fi

	if [ -d "/dev/stune/" ]; then
		# We are not concerned with prioritizing latency
		apply 0 /dev/stune/top-app/schedtune.prefer_idle

		# Mark top-app as boosted, find high-performing CPUs
		apply 1 /dev/stune/top-app/schedtune.boost
	fi

	# Oppo/Oplus/Realme Touchpanel
	tp_path="/proc/touchpanel"
	if [ -d "$tp_path" ]; then
		apply 0 $tp_path/game_switch_enable
		apply 1 $tp_path/oplus_tp_limit_enable
		apply 1 $tp_path/oppo_tp_limit_enable
		apply 0 $tp_path/oplus_tp_direction
		apply 0 $tp_path/oppo_tp_direction
	fi

	# Memory Tweaks
	apply 120 /proc/sys/vm/vfs_cache_pressure

	# eMMC and UFS frequency
	for path in /sys/class/devfreq/*.ufshc \
		/sys/class/devfreq/mmc*; do
		devfreq_unlock "$path"
	done &

	# Restore min CPU frequency
	change_cpu_gov "$DEFAULT_CPU_GOV"
	[ -d /proc/ppm ] && cpufreq_ppm_unlock || cpufreq_unlock

	# I/O Tweaks
	for dir in /sys/block/mmcblk0 /sys/block/mmcblk1 /sys/block/sd*; do
		# Reduce heuristic read-ahead in exchange for I/O latency
		apply 128 "$dir/queue/read_ahead_kb"

		# Reduce the maximum number of I/O requests in exchange for latency
		apply 64 "$dir/queue/nr_requests"
	done &

	case $SOC in
	1) mediatek_normal ;;
	2) snapdragon_normal ;;
	3) exynos_normal ;;
	4) unisoc_normal ;;
	5) tensor_normal ;;
	6) intel_normal ;;
	7) tegra_normal ;;
	esac
}

powersave_profile() {
	normal_profile

	# Enable battery saver module
	[ -f /sys/module/battery_saver/parameters/enabled ] && {
		if grep -qo '[0-9]\+' /sys/module/battery_saver/parameters/enabled; then
			apply 1 /sys/module/battery_saver/parameters/enabled
		else
			apply Y /sys/module/battery_saver/parameters/enabled
		fi
	}

	# eMMC and UFS frequency
	for path in /sys/class/devfreq/*.ufshc \
		/sys/class/devfreq/mmc*; do
		devfreq_min_perf "$path"
	done &

	# CPU governor
	change_cpu_gov "$(<$MODULE_CONFIG/powersave_cpu_gov)"

	case $SOC in
	1) mediatek_powersave ;;
	2) snapdragon_powersave ;;
	3) exynos_powersave ;;
	4) unisoc_powersave ;;
	5) tensor_powersave ;;
	6) intel_powersave ;;
	7) tegra_powersave ;;
	esac
}

###################################
# Main Function
###################################

case "$1" in
0) perfcommon ;;
1) performance_profile ;;
2) normal_profile ;;
3) powersave_profile ;;
esac

wait
exit 0
