#!/system/bin/sh
# Encore AppMonitoringUtil
dumpsys window | grep -E 'mFocusedApp' | grep -Eo $(cat /data/encore/gamelist.txt)

#for pid in $(dumpsys window | grep "Session Session{" | awk '{print $3}' | awk -F':' '{print $1}'); do
#	cmdline=$(cat /proc/$pid/cmdline | tr '\0' ' ') # Replace null bytes with spaces
#	echo "$cmdline" | grep -Eo $(cat /data/encore/gamelist.txt)
#done
