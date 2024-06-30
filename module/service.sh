#!/bin/sh
while [ -z "$(getprop sys.boot_completed)" ]; do
	sleep 40
done

case $(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_available_governors) in
*"schedplus"*) default_cpu_gov="schedplus" ;;
*"sugov_ext"*) default_cpu_gov="sugov_ext" ;;
*"walt"*) default_cpu_gov="walt" ;;
*) default_cpu_gov="schedutil" ;;
esac

echo $default_cpu_gov >/data/encore/default_cpu_gov

nice -n -20 encore-service >/dev/null 2>&1
