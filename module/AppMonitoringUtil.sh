#!/system/bin/sh
# Encore AppMonitoringUtil

for game in $(echo $(ps -e | grep -Eo \"$(cat /data/encore/gamelist.txt)\")); do
	if dumpsys activity services $game | grep -Eo "uidState: TOP"; then
		active_game=$game
		break
	fi >/dev/null 2>&1
done

echo -n $active_game
