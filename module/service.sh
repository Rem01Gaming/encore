#!/bin/sh
while [ -z "$(getprop sys.boot_completed)" ]; do
	sleep 40
done

chmod 444 /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor >/data/encore/default_cpu_gov

rm -f /data/encore/last_fault
touch /data/encore/last_fault

encore-service
code=$?
if [ $code -gt 0 ] && [ ! $code -eq 143 ]; then
	su -lp 2000 -c "cmd notification post -S bigtext -t 'ENCORE Tweaks' 'Tag$(date +%s)' \"encore-service exited abnormally, please report this to chat group. (exit code $code)"\"
fi
