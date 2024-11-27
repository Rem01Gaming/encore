#!/bin/sh

# Wait until boot completed
while [ -z "$(getprop sys.boot_completed)" ]; do
	sleep 40
done

# Parse Governor to use
chmod 444 /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
default_gov="$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor)"
echo $default_gov >/data/encore/default_cpu_gov
[ ! -f /data/encore/powersave_cpu_gov ] && echo $default_gov >/data/encore/powersave_cpu_gov

# Clear old logs
rm -f /data/encore/encore_log
touch /data/encore/encore_log

# Start Encore Daemon
encored
