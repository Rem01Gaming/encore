# Wait until boot completed
while [ -z "$(getprop sys.boot_completed)" ]; do
	sleep 40
done

CPUFREQ="/sys/devices/system/cpu/cpu0/cpufreq"

# Parse Governor to use
chmod 444 "$CPUFREQ/scaling_governor"
default_gov=$(cat "$CPUFREQ/scaling_governor")

# Handle case when 'default_gov' is performance
if [ "$default_gov" == "performance" ]; then
	for gov in schedhorizon sugov_ext walt schedutil; do
		grep -q "$gov" "$CPUFREQ/scaling_available_frequencies" && {
			default_gov="$gov"
			break
		}
	done
fi

echo "$default_gov" >/data/encore/default_cpu_gov
[ ! -f /data/encore/powersave_cpu_gov ] && echo "$default_gov" >/data/encore/powersave_cpu_gov

# Touch log file
touch /dev/encore_log

# Expose ProfileMode from here
touch /dev/encore_mode

# Copy gamelist to tmpfs
cp /data/encore/gamelist.txt /dev/encore_gamelist

# Start Encore Daemon
encored
