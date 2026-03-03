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

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <thread>

#include <DeviceMitigationStore.hpp>
#include <Dumpsys.hpp>
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

// ---------------------------------------------------------------------------
// Global registry
// ---------------------------------------------------------------------------

GameRegistry game_registry;

// ---------------------------------------------------------------------------
// Daemon timing constants
// ---------------------------------------------------------------------------

static constexpr auto INGAME_LOOP_INTERVAL = std::chrono::milliseconds(500);
static constexpr auto NORMAL_LOOP_INTERVAL = std::chrono::seconds(7);

static_assert(
    NORMAL_LOOP_INTERVAL % INGAME_LOOP_INTERVAL == std::chrono::milliseconds(0),
    "NORMAL_LOOP_INTERVAL must be a multiple of INGAME_LOOP_INTERVAL"
);

// ---------------------------------------------------------------------------
// Daemon States
// ---------------------------------------------------------------------------

struct DaemonState {
    EncoreProfileMode cur_mode = PERFCOMMON;
    DumpsysWindowDisplays window_displays = {};

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
[[nodiscard]] static std::string get_active_game(const DumpsysWindowDisplays &window_info, GameRegistry &registry) {
    if (registry.is_game_registered(window_info.focused_app)) {
        return window_info.focused_app;
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
    if (state.window_displays.focused_app == state.active_package) {
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
 * Tries the fast `cmd settings` path first, then falls back to DumpsysPower.
 * Returns std::nullopt when both methods fail.
 */
[[nodiscard]] static std::optional<bool> get_battery_saver_state() {
    static bool use_settings_method = true;

    if (use_settings_method) {
        auto pipe = popen_direct({"/system/bin/cmd", "settings", "get", "global", "low_power"});
        if (pipe.stream) {
            char buffer[16];
            if (fgets(buffer, sizeof(buffer), pipe.stream)) {
                std::string result = buffer;
                result.erase(
                    std::remove_if(
                        result.begin(),
                        result.end(),
                        [](unsigned char c) {
                            return std::isspace(c);
                        }
                    ),
                    result.end()
                );
                if (result == "1") return true;
                if (result == "0") return false;
            }
        }
        use_settings_method = false;
    }

    try {
        DumpsysPower dumpsys_power;
        Dumpsys::Power(dumpsys_power);
        return dumpsys_power.battery_saver;
    } catch (const std::runtime_error &e) {
        LOGE_TAG("DumpsysPower", "{}", e.what());
        return std::nullopt;
    }
}

/**
 * @brief Returns the PID of @p package_name, or 0 on failure.
 * @note This function uses dumpsys to grep exact PID of the app, different from EncoreUtility's @p pidof()
 */
[[nodiscard]] static pid_t pidof_game(const std::string &package_name) {
    try {
        return Dumpsys::GetAppPID(package_name);
    } catch (const std::runtime_error &e) {
        LOGE_TAG("DumpsysGetAppPID", "{}", e.what());
        return 0;
    }
}

/**
 * @brief Checks if Do Not Disturb mode is currently enabled.
 * * @return true if zen_mode is 1 (Priority), 2 (Total Silence), or 3 (Alarms Only).
 * @return false if zen_mode is 0 or if the shell command fails.
 */
bool is_dnd_enabled() {
    PipeResult result = popen_direct({"settings", "get", "global", "zen_mode"});

    if (result.stream == nullptr) {
        return false; // Failed to fork or execute
    }

    char buffer[16];
    if (fgets(buffer, sizeof(buffer), result.stream) != nullptr) {
        // We check if the first character is NOT '0'
        return buffer[0] != '0';
    }

    return false;
}

/**
 * @brief Attempt to refresh window-display data.
 * @return true on success, false if the dumpsys call threw.
 */
[[nodiscard]] static bool refresh_window_displays(DaemonState &state) {
    try {
        Dumpsys::WindowDisplays(state.window_displays);
        state.last_full_check = std::chrono::steady_clock::now();
        return true;
    } catch (const std::runtime_error &e) {
        LOGE_TAG("DumpsysWindowDisplays", "{}", e.what());
        return false;
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
    state.active_package.clear();
    state.pid_tracker.invalidate();
    state.mlbb_tracker.invalidate();
    state.in_game_session = false;
    state.need_profile_checkup = true;
    state.is_mlbb = false;

    // Refresh immediately so the balance/powersave decision below sees
    // the current foreground app rather than stale data.
    if (!refresh_window_displays(state)) {
        LOGE_TAG("DumpsysWindowDisplays", "Failed to refresh after game exit");
    }
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
    if (!state.active_package.empty() && state.window_displays.screen_awake) {
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
        // Module update check
        if (access(MODULE_UPDATE, F_OK) == 0) [[unlikely]] {
            LOGI("Module update detected, exiting");
            notify("Please reboot your device to complete module update.");
            break;
        }

        // Decide whether a full (slow) check is due
        const auto now = std::chrono::steady_clock::now();
        const bool do_full_check = !state.in_game_session || (now - state.last_full_check) >= NORMAL_LOOP_INTERVAL;

        if (do_full_check && !refresh_window_displays(state)) {
            std::this_thread::sleep_for(INGAME_LOOP_INTERVAL);
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

        // Discover active game
        if (state.active_package.empty()) {
            state.active_package = get_active_game(state.window_displays, game_registry);
            if (!state.active_package.empty()) {
                state.in_game_session = true;
                state.prev_dnd_state = is_dnd_enabled();
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

        // Sleep
        std::this_thread::sleep_for(state.in_game_session ? INGAME_LOOP_INTERVAL : NORMAL_LOOP_INTERVAL);
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

    if (!check_dumpsys_sanity()) {
        std::cerr << "\033[31mERROR:\033[0m Dumpsys sanity check failed\n";
        notify_fatal_error("Dumpsys sanity check failed");
        LOGC("Dumpsys sanity check failed");
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

    InotifyWatcher file_watcher;
    if (!init_file_watcher(file_watcher)) {
        LOGC("Failed to initialize file watcher");
        notify_fatal_error("Failed to initialize file watcher");
        return EXIT_FAILURE;
    }

    LOGI("Encore Tweaks daemon started");
    set_module_description_status("\xF0\x9F\x98\x8B Tweaks applied successfully");
    encore_main_daemon();

    LOGW("Encore Tweaks daemon exited");
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
