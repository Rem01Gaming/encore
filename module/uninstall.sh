#!/bin/sh

rm -f /data/local/tmp/encore_logo.png
rm -rf /data/encore
need_gone="encored encore_profiler encore_utility"
manager_paths="/data/adb/ap/bin /data/adb/ksu/bin"

for dir in $manager_paths; do
	[ -d "$dir" ] && {
		for bin in $need_gone; do
			rm "$dir/$bin"
		done
	}
done
