#!/system/bin/sh
# Encore AppMonitoringUtil

pids=($(dumpsys window visible-apps | grep -oE "Session{*.*.}" | awk -F'[ :]+' '{print $2}'))
for pid in ${pids[@]}; do
	cat /proc/$pid/cmdline | tr '\0' ' ' | grep -Eo $(cat /data/encore/gamelist.txt)
done
