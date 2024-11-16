#!/bin/sh
while [ -z "$(getprop sys.boot_completed)" ]; do
	sleep 40
done

chmod 444 /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
default_gov="$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor)"
echo $default_gov >/data/encore/default_cpu_gov
[ ! -f /data/encore/powersave_cpu_gov ] && echo $default_gov >/data/encore/powersave_cpu_gov

rm -f /data/encore/encore_log
touch /data/encore/encore_log

encore-service
code=$?
if [ $code -gt 0 ] && [ ! $code -eq 143 ]; then
	su -lp 2000 -c "cmd notification post -S bigtext -t 'Encore Tweaks' 'Tag$(date +%s)' \"encore-service exited abnormally, please check logs and report this to chat group. (exit code $code)"\"
fi
