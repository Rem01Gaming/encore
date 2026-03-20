/*
 * Copyright (C) 2024-2026 Rem01Gaming
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <thread>

#include <poll.h>
#include <sys/eventfd.h>

#include <DeviceMitigationStore.hpp>
#include <Encore.hpp>
#include <EncoreCLI.hpp>
#include <EncoreConfig.hpp>
#include <EncoreConfigStore.hpp>
#include <EncoreLog.hpp>
#include <EncoreUtility.hpp>
#include <GameRegistry.hpp>
#include <InotifyWatcher.hpp>
#include <LockFile.hpp>
#include <ModuleProperty.hpp>
#include <PIDTracker.hpp>
#include <ShellUtility.hpp>
#include <SignalHandler.hpp>
#include <SystemStatus.hpp>

// ---------------------------------------------------------------------------
// Global registry
// ---------------------------------------------------------------------------

GameRegistry game_registry;

// ---------------------------------------------------------------------------
// module.prop management
// ---------------------------------------------------------------------------

void set_module_description_status(const std::string &status) {
    const std::string description = "[" + status + "] Special performance module for your Device.";
    const std::vector<ModuleProperties> props{{"description", description}};
    try {
        ModuleProperty::Change(MODULE_PROP, props);
    } catch (const std::runtime_error &e) {
        LOGE("Failed to apply module properties: {}", e.what());
    }
}

void notify_fatal_error(const std::string &error_msg) {
    notify(("ERROR: " + error_msg).c_str());
    set_module_description_status("\xE2\x9D\x8C " + error_msg);
}

// ---------------------------------------------------------------------------
// Global event signaling for immediate daemon wake-up
// ---------------------------------------------------------------------------

int system_status_event_fd = -1;

std::atomic<bool> daemon_stop_requested{false};

/**
 * @brief Signal the daemon to wake pool loop
 */
void signal_daemon_update() {
    if (system_status_event_fd >= 0) {
        uint64_t val = 1;
        ssize_t ret = write(system_status_event_fd, &val, sizeof(val));
        (void)ret; // Suppress unused warning
    }
}

/**
 * @brief Signal the daemon to stop immediately
 */
void signal_daemon_stop() {
    daemon_stop_requested.store(true, std::memory_order_relaxed);
    signal_daemon_update();
}

// ---------------------------------------------------------------------------
// Lock file management
// ---------------------------------------------------------------------------

static LockFile daemon_lock{LOCK_FILE};
static LockFile java_lock{JAVA_LOCK_FILE};

/**
 * @brief Acquire the daemon singleton lock (non-blocking)
 *
 * Creates LOCK_FILE if absent and tries F_SETLK. Returns false (and leaves
 * the file unlocked) when another daemon instance is already running.
 */
[[nodiscard]] bool create_lock_file() {
    return daemon_lock.acquire(LockFile::AcquireMode::NonBlocking, LockFile::LockType::Exclusive);
}

/**
 * @brief Arm the Java companion daemon liveness watch
 *
 * Installs a LockFile::watch() on JAVA_LOCK_FILE. The watch callback fires
 * signal_daemon_stop() the instant the Java daemon releases its lock.
 */
void watch_java_lock() {
    java_lock.watch([](bool became_free) {
        if (became_free) {
            LOGC("Java daemon lock released, companion daemon exited or crashed, stopping daemon");
            notify_fatal_error("Java companion daemon crashed");
            signal_daemon_stop();
        }
    });
}

// ---------------------------------------------------------------------------
// Daemon States
// ---------------------------------------------------------------------------

struct DaemonState {
    EncoreProfileMode cur_mode = PERFCOMMON;
    SystemStatus system_status = {};

    std::string active_package;

    bool in_game_session = false;
    bool battery_saver_state = false;
    bool need_profile_checkup = false;
    bool game_requested_dnd = false;
    bool prev_dnd_state = false;

    int focus_loss_count = 0;

    PIDTracker pid_tracker;
};

// ---------------------------------------------------------------------------
// Helper functions
// ---------------------------------------------------------------------------

/**
 * @brief Returns the package name of the focused registered game,
 *        or an empty string if none is active.
 */
[[nodiscard]] static std::string get_active_game(const SystemStatus &status, GameRegistry &registry) {
    if (registry.is_game_registered(status.focused_app)) {
        return status.focused_app;
    }
    return {};
}

/**
 * @brief Returns true if the active game still holds focus.
 *
 * Uses a 3-strike debounce so transient focus blips (e.g. in-game overlays)
 * are not mistaken for a genuine exit. Process-death is handled exclusively
 * by the PID tracker callback, so this function only examines focus state.
 */
[[nodiscard]] static bool is_game_still_active(DaemonState &state) {
    if (state.system_status.focused_app == state.active_package) {
        state.focus_loss_count = 0;
        return true;
    }

    // Only return false if the focus lost persists for 3 consecutive status updates
    state.focus_loss_count++;
    if (state.focus_loss_count < 3) {
        LOGD("is_game_still_active: Focus lost for {}, strike {}/3", state.active_package, state.focus_loss_count);
        return true;
    }

    LOGD("is_game_still_active: Game {} no longer active (3 strikes reached)", state.active_package);
    state.focus_loss_count = 0;
    return false;
}

/**
 * @brief Queries the current battery-saver state.
 *
 * Reads the battery_saver field from the inotify-fed SystemStatusCache.
 * Returns std::nullopt when both methods fail.
 */
[[nodiscard]] static std::optional<bool> get_battery_saver_state() {
    SystemStatus status;
    if (!system_status_cache.get(status)) {
        return std::nullopt;
    }
    return status.battery_saver;
}

/**
 * @brief Returns the PID of @p package_name, or 0 on failure.
 */
[[nodiscard]] static pid_t pidof_game(const std::string &package_name) {
    SystemStatus status;
    if (system_status_cache.get(status) && status.focused_app == package_name && status.focused_pid > 0) {
        return status.focused_pid;
    }

    // Fallback to scan /proc for the process name
    const pid_t pid = pidof(package_name, false);
    if (pid != 0) return pid;
    LOGE_TAG("pidof_game", "Could not find PID for {}", package_name);
    return 0;
}

/**
 * @brief Checks if Do Not Disturb mode is currently enabled.
 * @return true if zen_mode is 1 (Priority), 2 (Total Silence), or 3 (Alarms Only).
 * @return false if zen_mode is 0 or if the java daemon fails to fetch zen_mode.
 */
bool is_dnd_enabled() {
    SystemStatus status;
    if (system_status_cache.get(status)) {
        return status.zen_mode != 0;
    }

    return false;
}

/**
 * @brief Pull the latest system-status snapshot from the inotify-fed cache.
 */
[[nodiscard]] static bool refresh_system_status(DaemonState &state) {
    if (!system_status_cache.get(state.system_status)) {
        LOGW_TAG("SystemStatus", "Cache not yet populated, waiting for SystemMonitor");
        return false;
    }

    return true;
}

/**
 * @brief Clear Do Not Disturb if the game had requested it.
 */
static void clear_dnd_if_needed(DaemonState &state) {
    if (state.game_requested_dnd) {
        set_do_not_disturb(state.prev_dnd_state); // Respect User's DND setting
        state.game_requested_dnd = false;
    }
}

/**
 * @brief Handle the transition when the tracked game process exits.
 *
 * Clears session state and does an immediate system-status refresh so the
 * next profile decision is based on fresh data.
 */
static void handle_game_exit(DaemonState &state) {
    LOGI("Game {} exited", state.active_package);
    clear_dnd_if_needed(state);
    state.active_package.clear();
    state.pid_tracker.invalidate();
    state.in_game_session = false;
    state.need_profile_checkup = true;

    // Refresh immediately so the balance/powersave decision below sees
    // the current foreground app rather than stale data.
    if (!refresh_system_status(state)) {
        LOGE_TAG("SystemStatus", "Failed to refresh system status after game exit");
    }
}

/**
 * @brief Apply the performance profile for the active game.
 *
 * Resolves the game entry and PID, applies the profile, and arms the PID
 * tracker.
 *
 * @return true if the profile was applied; false if the session was cleared.
 */
[[nodiscard]] static bool apply_game_profile(DaemonState &state) {
    auto *active_game = game_registry.find_game_ptr(state.active_package);
    if (!active_game) {
        LOGI("Game {} is no longer listed in registry", state.active_package);
        state.active_package.clear();
        state.pid_tracker.invalidate();
        state.in_game_session = false;
        return false;
    }

    const pid_t game_pid = pidof_game(state.active_package);
    if (game_pid == 0) {
        LOGE("Unable to fetch PID of {}", state.active_package);
        state.active_package.clear();
        state.pid_tracker.invalidate();
        state.in_game_session = false;
        return false;
    }

    state.need_profile_checkup = false;
    state.cur_mode = PERFORMANCE_PROFILE;

    LOGI("Applying performance profile for {} (PID: {})", state.active_package, game_pid);
    const bool lite_mode = active_game->lite_mode || config_store.get_preferences().enforce_lite_mode;
    apply_performance_profile(lite_mode, state.active_package, game_pid);

    // For MLBB and other games by Moonton, UnityKillsMe is the foreground process.
    // For all other games, track the main game PID.
    const pid_t mlbb_pid = pidof(state.active_package + ":UnityKillsMe", true);
    if (mlbb_pid != 0) {
        LOGD("Found UnityKillsMe thread for {} (PID: {}), tracking as game process", state.active_package, mlbb_pid);
        state.pid_tracker.set_pid(mlbb_pid);
    } else {
        state.pid_tracker.set_pid(game_pid);
    }

    // DND handling
    if (active_game->enable_dnd) {
        state.game_requested_dnd = true;
        set_do_not_disturb(true);
    } else {
        state.game_requested_dnd = false;
        set_do_not_disturb(state.prev_dnd_state); // Respect User's DND setting
    }

    return true;
}

/**
 * @brief Select and apply the appropriate profile based on current state.
 *
 * Priority order:
 *   1. Active game + screen awake  → performance profile
 *   2. Battery saver active        → powersave profile
 *   3. Otherwise                   → balance profile
 *
 * Each branch is a no-op when the current mode already matches.
 */
static void select_profile(DaemonState &state) {
    if (!state.active_package.empty() && state.system_status.screen_awake) {
        if (!state.need_profile_checkup && state.cur_mode == PERFORMANCE_PROFILE) return;
        if (apply_game_profile(state)) return;
    }

    if (state.battery_saver_state) {
        if (state.cur_mode == POWERSAVE_PROFILE) return;
        state.cur_mode = POWERSAVE_PROFILE;
        state.need_profile_checkup = false;
        LOGI("Applying powersave profile");
        apply_powersave_profile();
        clear_dnd_if_needed(state);
        return;
    }

    if (state.cur_mode == BALANCE_PROFILE) return;
    state.cur_mode = BALANCE_PROFILE;
    state.need_profile_checkup = false;
    LOGI("Applying balance profile");
    apply_balance_profile();
    clear_dnd_if_needed(state);
}

// ---------------------------------------------------------------------------
// Main daemon loop
// ---------------------------------------------------------------------------

static void encore_main_daemon() {
    DaemonState state;
    run_perfcommon();
    pthread_setname_np(pthread_self(), "MainThread");

    // On any tracked process death, immediately wake the
    // main poll so handle_game_exit runs without delay.
    state.pid_tracker.set_callback([](pid_t) {
        signal_daemon_update();
    });

    // Spin until the inotify-fed cache has its first snapshot.
    // The InotifyWatcher will call signal_daemon_update() as soon as the
    // watched file changes, so we can block here rather than busy-wait.
    while (!daemon_stop_requested.load(std::memory_order_relaxed)) {
        if (refresh_system_status(state)) break;
        struct pollfd pfd = {system_status_event_fd, POLLIN, 0};
        poll(&pfd, 1, -1); // block until InotifyWatcher signals
        uint64_t val;
        ssize_t ret = read(system_status_event_fd, &val, sizeof(val));
        (void)ret;
    }

    // Single event source: system_status_event_fd
    //   - inotify updates from InotifyWatcher
    //   - pid_tracker process-death callbacks
    //   - signal_daemon_stop() wakeups (from java_lock watch or signal handler)
    struct pollfd pfd = {system_status_event_fd, POLLIN, 0};

    while (!daemon_stop_requested.load(std::memory_order_relaxed)) {
        const int ret = poll(&pfd, 1, -1); // sleep until something happens
        if (ret < 0) {
            if (errno == EINTR) continue;
            LOGE_TAG("MainThread", "poll() failed: {}", strerror(errno));
            break;
        }

        if (daemon_stop_requested.load(std::memory_order_relaxed)) [[unlikely]]
            break;

        if (pfd.revents & POLLIN) {
            // Drain all accumulated wakeups in one read.
            uint64_t val;
            ssize_t rd = read(system_status_event_fd, &val, sizeof(val));
            (void)rd;

            if (state.in_game_session && state.pid_tracker.get_current_pid() == 0) {
                handle_game_exit(state);
                // Fall through, select_profile below will apply balance/powersave.
            }

            if (!refresh_system_status(state)) continue;

            // Focus-loss check (3-strike debounce against transient blips)
            if (state.in_game_session && !state.active_package.empty()) {
                if (!is_game_still_active(state)) [[unlikely]] {
                    handle_game_exit(state);
                }
            }

            // Track user's DND preference while we are not overriding it
            if (!state.game_requested_dnd) {
                state.prev_dnd_state = is_dnd_enabled();
            }

            // Discover a newly focused game
            if (state.active_package.empty()) {
                state.active_package = get_active_game(state.system_status, game_registry);
                if (!state.active_package.empty()) {
                    state.in_game_session = true;
                    LOGD("DND state before in_game_session: {}", state.prev_dnd_state ? "ON" : "OFF");
                }
            }

            if (state.active_package.empty()) {
                const auto bs_state = get_battery_saver_state();
                if (bs_state.has_value()) {
                    state.battery_saver_state = *bs_state;
                } else {
                    LOGW("get_battery_saver_state: cache not yet populated, retaining last known value");
                }
            }

            select_profile(state);
        }
    }
}

// ---------------------------------------------------------------------------
// Daemon bootstrap
// ---------------------------------------------------------------------------

// Will be called by EncoreCLI
int run_daemon() {
    std::atexit([]() {
        SignalHandler::cleanup_before_exit();
    });

    SignalHandler::setup_signal_handlers();

    if (daemon_lock.is_locked()) {
        std::cerr << "\033[31mERROR:\033[0m Another instance of Encore Daemon is already running!\n";
        return EXIT_FAILURE;
    }

    if (access(MODULE_UPDATE, F_OK) == 0) {
        notify("Please reboot your device to complete module update.");
        return EXIT_FAILURE;
    }

    if (access(ENCORE_GAMELIST, F_OK) != 0) {
        std::cerr << "\033[31mERROR:\033[0m " << ENCORE_GAMELIST << " is missing\n";
        notify_fatal_error("gamelist.json is missing");
        LOGC("{} is missing", ENCORE_GAMELIST);
        return EXIT_FAILURE;
    }

    if (!game_registry.load_from_json(ENCORE_GAMELIST)) {
        std::cerr << "\033[31mERROR:\033[0m Failed to parse " << ENCORE_GAMELIST << '\n';
        notify_fatal_error("Failed to parse gamelist.json");
        LOGC("Failed to parse {}", ENCORE_GAMELIST);
        return EXIT_FAILURE;
    }

    if (!device_mitigation_store.load_config()) {
        std::cerr << "\033[31mERROR:\033[0m Failed to parse " << DEVICE_MITIGATION_FILE << '\n';
        notify_fatal_error("Failed to parse device_mitigation.json");
        LOGC("Failed to parse {}", DEVICE_MITIGATION_FILE);
        return EXIT_FAILURE;
    }

    if (daemon(0, 0) != 0) {
        LOGC("Failed to daemonize service");
        notify_fatal_error("Failed to daemonize service");
        return EXIT_FAILURE;
    }

    if (!create_lock_file()) {
        LOGC("Failed acquire daemon lock after daemonize");
        notify_fatal_error("Failed to acquire lock");
        return EXIT_FAILURE;
    }

    // eventfd for immediate daemon wake-up on system_status changes and PID
    // tracker callbacks.
    system_status_event_fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    if (system_status_event_fd < 0) {
        LOGC("Failed to create eventfd: {}", strerror(errno));
        notify_fatal_error("Failed to create eventfd");
        return EXIT_FAILURE;
    }

    // Check for the Java companion daemon lock before proceeding
    {
        int check = 0;
        const int max_retries = 120;
        while (!java_lock.is_locked()) {
            if (++check > max_retries) {
                LOGC("Java companion daemon absent after {} checks, exiting", max_retries);
                notify_fatal_error("Java companion daemon crashed");
                return EXIT_FAILURE;
            }

            if (check <= 1) LOGW("Java companion daemon lock not held, waiting...");
            sleep(1);
        }
    }

    // Watch the Java companion daemon lock
    watch_java_lock();

    InotifyWatcher file_watcher;
    if (!init_file_watcher(file_watcher)) {
        LOGC("Failed to initialize file watcher");
        notify_fatal_error("Failed to initialize file watcher");
        close(system_status_event_fd);
        system_status_event_fd = -1;
        return EXIT_FAILURE;
    }

    LOGI("Encore Tweaks daemon started");
    set_module_description_status("\xF0\x9F\x98\x8B Tweaks applied successfully");
    encore_main_daemon();

    LOGW("Encore Tweaks daemon exited");

    if (system_status_event_fd >= 0) {
        close(system_status_event_fd);
        system_status_event_fd = -1;
    }

    SignalHandler::cleanup_before_exit();
    return EXIT_SUCCESS;
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

int main(int argc, char *argv[]) {
    if (getuid() != 0) {
        std::cerr << "\033[31mERROR:\033[0m Please run this program as root\n";
        return EXIT_FAILURE;
    }
    return encore_cli(argc, argv);
}
