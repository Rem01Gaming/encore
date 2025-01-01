# Wait until boot completed
while [ -z "$(getprop sys.boot_completed)" ]; do
	sleep 40
done

MODPATH=${0%/*}

# Create symlink at every boot these tmpfs dirs 
# Magisk will add /debug_ramdisk, /sbin on $rwdir
[ -f /data/adb/magisk/magisk ] && {
	[ -w /sbin ] && rwdir=/sbin
	[ -w /debug_ramdisk ] && rwdir=/debug_ramdisk
	ln -sf "$MODPATH/system/bin/encored" "$rwdir/encored"
	ln -sf "$MODPATH/system/bin/encore_profiler" "$rwdir/encore_profiler"
	ln -sf "$MODPATH/system/bin/encore_utility" "$rwdir/encore_utility"
}

# Parse Governor to use
chmod 444 /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
default_gov="$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor)"

# Handle case when 'default_gov' is performance
if echo $default_gov | grep -q performance; then
	default_gov="schedutil"
fi

echo $default_gov >/data/encore/default_cpu_gov
[ ! -f /data/encore/powersave_cpu_gov ] && echo $default_gov >/data/encore/powersave_cpu_gov

# Clear old logs
rm -f /data/encore/encore_log
touch /data/encore/encore_log

# Start Encore Daemon
encored
