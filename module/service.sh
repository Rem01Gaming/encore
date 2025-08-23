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

MODULE_CONFIG="/data/adb/.config/encore"
CPUFREQ="/sys/devices/system/cpu/cpu0/cpufreq"

# Clear old logs
rm -f "$MODULE_CONFIG/encore.log"

# Parse Governor to use
chmod 644 "$CPUFREQ/scaling_governor"
default_gov=$(cat "$CPUFREQ/scaling_governor")
echo "$default_gov" >$MODULE_CONFIG/default_cpu_gov

# Wait until boot completed
while [ -z "$(getprop sys.boot_completed)" ]; do
	sleep 40
done

# Handle case when 'default_gov' is performance
# Skip this routine custom_default_cpu_gov is defined
if [ "$default_gov" == "performance" ] && [ ! -f $MODULE_CONFIG/custom_default_cpu_gov ]; then
	for gov in scx schedhorizon walt sched_pixel sugov_ext uag schedplus energy_step schedutil interactive conservative powersave; do
		grep -q "$gov" "$CPUFREQ/scaling_available_governors" && {
			echo "$gov" >$MODULE_CONFIG/default_cpu_gov
			default_gov="$gov"
			break
		}
	done
fi

# Revert to normal CPU governor
custom_gov="$MODULE_CONFIG/custom_default_cpu_gov"
[ -f "$custom_gov" ] && default_gov=$(cat "$custom_gov")
echo "$default_gov" | tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
[ ! -f $MODULE_CONFIG/powersave_cpu_gov ] && echo "$default_gov" >$MODULE_CONFIG/powersave_cpu_gov

# Mitigate buggy thermal throttling on post-startup
# in old MediaTek devices.
ENABLE_PPM="/proc/ppm/enabled"
if [ -f "$ENABLE_PPM" ]; then
	echo 0 >"$ENABLE_PPM"
	sleep 1
	echo 1 >"$ENABLE_PPM"
fi

# Start Encore Daemon
encored

# Handle rare case if Encore Daemon crashed
# By: Kanagawa Yamada

if ! pgrep -x encored > /dev/null; then # Change if incorrect 
    /data/adb/modules/encore/service.sh # Change if path is not correct 
fi
    
