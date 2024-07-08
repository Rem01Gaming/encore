#!/bin/sh
while [ -z "$(getprop sys.boot_completed)" ]; do
	sleep 40
done

# CPU info
chipset=$(grep "Hardware" /proc/cpuinfo | uniq | cut -d ':' -f 2 | sed 's/^[ \t]*//')

if [ -z "$chipset" ]; then
	chipset=$(getprop "ro.hardware")
fi

case $chipset in
*mt* | *MT*) soc=1 ;;
*sm* | *qcom* | *SM* | *QCOM* | *Qualcomm*) soc=2 ;;
*exynos*) soc=3 ;;
*) soc=0 ;;
esac

case $(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_available_governors) in
*"schedplus"*) default_cpu_gov="schedplus" ;;
*"sugov_ext"*) default_cpu_gov="sugov_ext" ;;
*"walt"*) default_cpu_gov="walt" ;;
*"uag"*) default_cpu_gov="uag" ;;
*) default_cpu_gov="schedutil" ;;
esac

echo $default_cpu_gov >/data/encore/default_cpu_gov
echo $soc >/data/encore/soc_recognition

nice -n -20 encore-service >/dev/null 2>&1
