need_gone="encored encore_profiler encore_utility"
for file in $need_gone; do
	rm /data/adb/ap/bin/$file
	rm /data/adb/ksu/bin/$file
done
