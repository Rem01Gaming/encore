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

CPUFREQ="/sys/devices/system/cpu/cpu0/cpufreq"

# Parse Governor to use
chmod 644 "$CPUFREQ/scaling_governor"
default_gov=$(cat "$CPUFREQ/scaling_governor")
echo "$default_gov" >/data/encore/default_cpu_gov

# Wait until boot completed
while [ -z "$(getprop sys.boot_completed)" ]; do
	sleep 40
done

# Handle case when 'default_gov' is performance
if [ "$default_gov" == "performance" ]; then
	for gov in schedhorizon walt sugov_ext uag schedplus energy_step schedutil interactive conservative powersave; do
		grep -q "$gov" "$CPUFREQ/scaling_available_governors" && {
			echo "$gov" >/data/encore/default_cpu_gov
			default_gov="$gov"
			break
		}
	done
fi

# Revert to normal CPU governor
custom_gov="/data/encore/custom_default_cpu_gov"
[ -f "$custom_gov" ] && default_gov=$(cat "$custom_gov")
echo "$default_gov" | tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
[ ! -f /data/encore/powersave_cpu_gov ] && echo "$default_gov" >/data/encore/powersave_cpu_gov

# Copy gamelist to tmpfs
cp /data/encore/gamelist.txt /dev/encore_gamelist

# Start Encore Daemon
encored
