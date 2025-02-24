# Wait until boot completed
while [ -z "$(getprop sys.boot_completed)" ]; do
	sleep 40
done

# Parse Governor to use
chmod 444 /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
default_gov="$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor)"

# Handle case when 'default_gov' is performance
if echo $default_gov | grep -q performance; then
	default_gov="schedutil"
fi

echo $default_gov >/data/encore/default_cpu_gov
[ ! -f /data/encore/powersave_cpu_gov ] && echo $default_gov >/data/encore/powersave_cpu_gov

# Touch log file
touch /dev/encore_log

# Expose ProfileMode from here
touch /dev/encore_mode

# Copy gamelist to tmpfs
cp /data/encore/gamelist.txt /dev/encore_gamelist

# Initialize Encore Cgroups
[ -d /dev/cpuctl ] && mkdir /dev/cpuctl/encore
[ -d /dev/stune ] && mkdir /dev/stune/encore

# Start Encore Daemon
encored
