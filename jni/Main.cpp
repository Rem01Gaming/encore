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
// Daemon timing constants
// ---------------------------------------------------------------------------

static constexpr auto INGAME_LOOP_INTERVAL = std::chrono::milliseconds(500);
static constexpr auto NORMAL_LOOP_INTERVAL = std::chrono::seconds(7);
static constexpr auto JAVA_CHECK_INTERVAL = std::chrono::seconds(1);

static constexpr int POLL_TIMEOUT_MS = 50;
static constexpr int JAVA_CHECK_MAX_ERRORS = 10;

static_assert(
    NORMAL_LOOP_INTERVAL % INGAME_LOOP_INTERVAL == std::chrono::milliseconds(0),
    "NORMAL_LOOP_INTERVAL must be a multiple of INGAME_LOOP_INTERVAL"
);

// ---------------------------------------------------------------------------
// Global event signaling for immediate daemon wake-up
// ---------------------------------------------------------------------------

int system_status_event_fd = -1;

std::atomic<bool> daemon_stop_requested{false};

void signal_daemon_update() {
    if (system_status_event_fd >= 0) {
        uint64_t val = 1;
        ssize_t ret = write(system_status_event_fd, &val, sizeof(val));
        (void)ret; // Suppress unused warning
    }
}

void signal_daemon_stop() {
    daemon_stop_requested.store(true, std::memory_order_relaxed);
    // Wake the daemon poll loop immediately
    signal_daemon_update();
}

// ---------------------------------------------------------------------------
// Daemon States
// ---------------------------------------------------------------------------

struct DaemonState {
    EncoreProfileMode cur_mode = PERFCOMMON;
    SystemStatus system_status = {};

    std::string active_package;
    std::chrono::steady_clock::time_point last_full_check = std::chrono::steady_clock::now();

    bool in_game_session = false;
    bool battery_saver_state = false;
    bool battery_saver_state_oops = false;
    bool need_profile_checkup = false;
    bool game_requested_dnd = false;
    bool prev_dnd_state = false;
    bool is_mlbb = false;

    int focus_loss_count = 0;

    int java_check_fail_count = 0;
    std::chrono::steady_clock::time_point last_java_check = std::chrono::steady_clock::now();

    PIDTracker pid_tracker;
    PIDTracker mlbb_tracker;
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
 * @brief Returns true if @p package_name is still running.
 *
 * Handle cases where a game is still boosted when it already closed.
 */
[[nodiscard]] static bool is_game_still_active(DaemonState &state) {
    // Check if MLBB sub-process is still alive
    if (state.is_mlbb && !state.mlbb_tracker.is_valid()) {
        const pid_t new_mlbb_pid = pidof(state.active_package + ":UnityKillsMe", true);
        if (new_mlbb_pid == 0) {
            LOGI("UnityKillsMe thread of {} no longer active", state.active_package);
            return false;
        } else {
            LOGI("New UnityKillsMe thread detected for {} (PID: {})", state.active_package, new_mlbb_pid);
            state.mlbb_tracker.set_pid(new_mlbb_pid);
        }
    }

    // Check if the app is still focused
    if (state.system_status.focused_app == state.active_package) {
        state.focus_loss_count = 0;
        return true;
    }

    // Only return false if the focus lost persists for 3 checks
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
    // Fast path: the focused app's PID is already in the cache.
    SystemStatus status;
    if (system_status_cache.get(status) && status.focused_app == package_name && status.focused_pid > 0) {
        return status.focused_pid;
    }

    // Fallback: scan /proc for the process name.
    const pid_t pid = pidof(package_name, false);
    if (pid != 0) return pid;
    LOGE_TAG("pidof_game", "Could not find PID for {}", package_name);
    return 0;
}

/**
 * @brief Checks if Do Not Disturb mode is currently enabled.
 * * @return true if zen_mode is 1 (Priority), 2 (Total Silence), or 3 (Alarms Only).
 * @return false if zen_mode is 0 or if the shell command fails.
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

    state.last_full_check = std::chrono::steady_clock::now();
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
 * Clears session state and does an immediate window-display refresh so the
 * next profile decision is based on fresh data.
 */
static void handle_game_exit(DaemonState &state) {
    LOGI("Game {} exited", state.active_package);
    clear_dnd_if_needed(state);
    state.active_package.clear();
    state.pid_tracker.invalidate();
    state.mlbb_tracker.invalidate();
    state.in_game_session = false;
    state.need_profile_checkup = true;
    state.is_mlbb = false;

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
 * tracker. On any failure it clears the session so the loop falls back to
 * the balance/powersave path.
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

    // Handle MLBB process
    const pid_t mlbb_pid = pidof(state.active_package + ":UnityKillsMe", true);
    if (mlbb_pid != 0) {
        LOGD("Found UnityKillsMe thread for {} (PID: {})", state.active_package, mlbb_pid);
        state.is_mlbb = true;
        state.mlbb_tracker.set_pid(mlbb_pid);
    } else {
        state.is_mlbb = false;
        state.mlbb_tracker.invalidate();
    }

    state.need_profile_checkup = false;
    state.cur_mode = PERFORMANCE_PROFILE;

    LOGI("Applying performance profile for {} (PID: {})", state.active_package, game_pid);
    const bool lite_mode = active_game->lite_mode || config_store.get_preferences().enforce_lite_mode;
    apply_performance_profile(lite_mode, state.active_package, game_pid);
    state.pid_tracker.set_pid(game_pid);

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

    while (true) {
        if (daemon_stop_requested.load(std::memory_order_relaxed)) [[unlikely]] {
            break;
        }

        {
            const auto now_jc = std::chrono::steady_clock::now();
            if ((now_jc - state.last_java_check) >= JAVA_CHECK_INTERVAL) {
                state.last_java_check = now_jc;

                if (!check_java_daemon()) {
                    if (state.java_check_fail_count) LOGW("Waiting for Java daemon...");
                    ++state.java_check_fail_count;

                    if (state.java_check_fail_count >= JAVA_CHECK_MAX_ERRORS) {
                        LOGC("Java daemon absent for {} consecutive checks, stopping daemon", JAVA_CHECK_MAX_ERRORS);
                        signal_daemon_stop();
                    }
                } else if (state.java_check_fail_count > 0) {
                    LOGI("Java daemon recovered");
                    state.java_check_fail_count = 0;
                }
            }
        }

        // Decide whether a full (slow) check is due
        const auto now = std::chrono::steady_clock::now();
        const bool do_full_check = !state.in_game_session || (now - state.last_full_check) >= NORMAL_LOOP_INTERVAL;

        if (do_full_check && !refresh_system_status(state)) {
            // Cache not yet populated, wait briefly before retrying
            struct pollfd pfd = {system_status_event_fd, POLLIN, 0};
            poll(&pfd, 1, POLL_TIMEOUT_MS);
            continue;
        }

        // Stale-activity check (fixes profile stuck in MLBB etc.)
        if (state.in_game_session && !state.active_package.empty() && do_full_check) {
            if (!is_game_still_active(state)) [[unlikely]] {
                handle_game_exit(state);
            }
        }

        // PID liveness check
        if (state.in_game_session && !state.pid_tracker.is_valid()) [[unlikely]] {
            handle_game_exit(state);
        }

        // Monitor DND state whenever the daemon isn't overriding it
        if (!state.game_requested_dnd && do_full_check) {
            state.prev_dnd_state = is_dnd_enabled();
        }

        // Discover active game
        if (state.active_package.empty()) {
            state.active_package = get_active_game(state.system_status, game_registry);
            if (!state.active_package.empty()) {
                state.in_game_session = true;
                LOGD("DND state before in_game_session: {}", state.prev_dnd_state ? "ON" : "OFF");
            }
        }

        // Battery-saver poll (idle only, while method is still working)
        if (state.active_package.empty() && !state.battery_saver_state_oops && do_full_check) {
            const auto bs_state = get_battery_saver_state();
            state.battery_saver_state_oops = !bs_state.has_value();
            state.battery_saver_state = bs_state.value_or(false);

            if (state.battery_saver_state_oops) {
                LOGE("get_battery_saver_state is out of order!");
            }
        }

        // Profile selection
        select_profile(state);

        // Use remaining time until next full check, or INGAME_LOOP_INTERVAL
        int poll_timeout_ms = POLL_TIMEOUT_MS;
        if (state.in_game_session) {
            poll_timeout_ms = static_cast<int>(INGAME_LOOP_INTERVAL.count());
        } else {
            const auto elapsed = std::chrono::steady_clock::now() - state.last_full_check;
            const auto remaining = NORMAL_LOOP_INTERVAL - elapsed;
            if (remaining.count() > 0) {
                poll_timeout_ms = std::min(static_cast<int>(remaining.count()), static_cast<int>(NORMAL_LOOP_INTERVAL.count()));
            }
        }

        // Wait on eventfd with timeout, drain eventfd if signaled
        struct pollfd pfd = {system_status_event_fd, POLLIN, 0};
        int poll_ret = poll(&pfd, 1, poll_timeout_ms);

        if (poll_ret > 0 && (pfd.revents & POLLIN)) {
            uint64_t val;
            ssize_t ret = read(system_status_event_fd, &val, sizeof(val));
            (void)ret;
        }
    }
}

// ---------------------------------------------------------------------------
// Daemon bootstrap
// ---------------------------------------------------------------------------

// Will be called by EncoreCLI
int run_daemon() {
    auto set_module_description_status = [](const std::string &status) {
        const std::string description = "[" + status + "] Special performance module for your Device.";
        const std::vector<ModuleProperties> props{{"description", description}};
        try {
            ModuleProperty::Change(MODULE_PROP, props);
        } catch (const std::runtime_error &e) {
            LOGE("Failed to apply module properties: {}", e.what());
        }
    };

    auto notify_fatal_error = [&set_module_description_status](const std::string &error_msg) {
        notify(("ERROR: " + error_msg).c_str());
        set_module_description_status("\xE2\x9D\x8C " + error_msg);
    };

    std::atexit([]() {
        SignalHandler::cleanup_before_exit();
    });

    SignalHandler::setup_signal_handlers();

    if (!create_lock_file()) {
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

    // Create eventfd for immediate daemon wake-up on system_status changes
    system_status_event_fd = eventfd(0, EFD_CLOEXEC | EFD_SEMAPHORE);
    if (system_status_event_fd < 0) {
        LOGC("Failed to create eventfd: {}", strerror(errno));
        notify_fatal_error("Failed to create eventfd");
        return EXIT_FAILURE;
    }

    InotifyWatcher file_watcher;
    if (!init_file_watcher(file_watcher)) {
        LOGC("Failed to initialize file watcher");
        notify_fatal_error("Failed to initialize file watcher");
        if (system_status_event_fd >= 0) {
            close(system_status_event_fd);
            system_status_event_fd = -1;
        }
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
