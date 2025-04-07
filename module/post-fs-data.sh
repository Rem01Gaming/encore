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
chmod 444 "$CPUFREQ/scaling_governor"
default_gov=$(cat "$CPUFREQ/scaling_governor")

# Make boot up faster
echo performance | tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

# Write default governor
echo "$default_gov" >/data/encore/default_cpu_gov
