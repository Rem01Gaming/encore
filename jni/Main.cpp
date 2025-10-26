/*
 * Copyright (C) 2024-2025 Rem01Gaming
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
#include <thread>
#include <unordered_map>
#include <vector>

#include "Cli.hpp"
#include <Dumpsys.hpp>
#include <PIDTracker.hpp>
#include <GameRegistry.hpp>
#include <InotifyWatcher.hpp>
#include <ShellUtility.hpp>
#include <Encore.hpp>
#include <EncoreConfig.hpp>
#include <EncoreLog.hpp>
#include <EncoreUtility.hpp>

GameRegistry game_registry;

void encore_main_daemon(void) {
    constexpr static auto INGAME_LOOP_INTERVAL = std::chrono::milliseconds(500);
    constexpr static auto NORMAL_LOOP_INTERVAL = std::chrono::seconds(12);

    static_assert(
        NORMAL_LOOP_INTERVAL % INGAME_LOOP_INTERVAL == std::chrono::milliseconds(0),
        "NORMAL_LOOP_INTERVAL must be a multiple of INGAME_LOOP_INTERVAL");

    EncoreProfileMode cur_mode = PERFCOMMON;
    DumpsysWindowDisplays window_displays;

    std::string active_package;
    auto last_full_check = std::chrono::steady_clock::now();

    bool in_game_session = false;
    bool battery_saver_state = false;
    bool battery_saver_state_oops = false;
    bool need_profile_checkup = false;
    bool game_requested_dnd = false;

    PIDTracker pid_tracker;

    auto GetActiveGame = [&](const std::vector<RecentAppList> &recent_applist,
                             GameRegistry &registry) -> std::string {
        for (const auto &recent : recent_applist) {
            if (!recent.visible) continue;

            if (registry.is_game_registered(recent.package_name)) {
                return recent.package_name;
            }
        }

        return "";
    };

    auto GetBatterySaverState = [&]() -> std::optional<bool> {
        static bool use_settings_method = true;

        if (use_settings_method) {
            auto pipe = popen_direct({"/system/bin/cmd", "settings", "get", "global", "low_power"});

            if (pipe) {
                char buffer[16];
                if (fgets(buffer, sizeof(buffer), pipe.get())) {
                    std::string result = buffer;
                    result.erase(
                        std::remove_if(
                            result.begin(), result.end(),
                            [](unsigned char c) { return std::isspace(c); }),
                        result.end());

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
    };

    auto pidof_game = [&](const std::string &package_name) -> pid_t {
        pid_t game_pid = 0;
        try {
            game_pid = Dumpsys::GetAppPID(package_name);
            return game_pid;
        } catch (const std::runtime_error &e) {
            std::string error_msg = e.what();
            LOGE_TAG("DumpsysGetAppPID", "{}", error_msg);
            return 0;
        }
    };

    run_perfcommon();
    pthread_setname_np(pthread_self(), "MainThread");

    while (true) {
        if (access(MODULE_UPDATE, F_OK) == 0) [[unlikely]] {
            LOGI("Module update detected, exiting");
            notify("Please reboot your device to complete module update.");
            break;
        }

        auto now = std::chrono::steady_clock::now();
        bool do_full_check = !in_game_session || (now - last_full_check) >= INGAME_LOOP_INTERVAL;

        if (do_full_check) {
            try {
                Dumpsys::WindowDisplays(window_displays);
                last_full_check = now;
            } catch (const std::runtime_error &e) {
                LOGE_TAG("DumpsysWindowDisplays", "{}", e.what());
                std::this_thread::sleep_for(INGAME_LOOP_INTERVAL);
                continue;
            }
        }

        // PID check when in game session
        if (in_game_session && !pid_tracker.is_valid()) [[unlikely]] {
            LOGI("Game {} exited", active_package);
            active_package.clear();
            pid_tracker.invalidate();
            in_game_session = false;
            need_profile_checkup = true;

            // Game exited, run dumpsys immediately
            if (!do_full_check) {
                try {
                    Dumpsys::WindowDisplays(window_displays);
                    last_full_check = now;
                } catch (const std::runtime_error &e) {
                    LOGE_TAG("DumpsysGetAppPID", "{}", e.what());
                }
            }
        }

        // Fetch active game package name
        if (active_package.empty()) {
            active_package = GetActiveGame(window_displays.recent_app, game_registry);
            if (!active_package.empty()) {
                in_game_session = true;
            }
        }

        // Battery saver check
        if (active_package.empty() && !battery_saver_state_oops && do_full_check) {
            auto bs_state = GetBatterySaverState();
            battery_saver_state_oops = !bs_state.has_value();
            battery_saver_state = bs_state.value_or(false);

            if (battery_saver_state_oops) {
                LOGE("GetBatterySaverState is out of order!");
            }
        }

        // Profile selection logic
        if (!active_package.empty() && window_displays.screen_awake) {
            if (!need_profile_checkup && cur_mode == PERFORMANCE_PROFILE) goto take_me_to_the_bed;

            auto active_game = game_registry.find_game_ptr(active_package);
            if (!active_game) {
                LOGI("Game {} are no longer listed in registry", active_package);
                active_package.clear();
                pid_tracker.invalidate();
                in_game_session = false;
                goto take_me_to_the_bed;
            }

            pid_t game_pid = pidof_game(active_package);
            if (game_pid == 0) {
                LOGE("Unable to fetch PID of {}", active_package);
                active_package.clear();
                pid_tracker.invalidate();
                in_game_session = false;
                goto take_me_to_the_bed;
            }

            need_profile_checkup = false;
            cur_mode = PERFORMANCE_PROFILE;

            LOGI("Applying performance profile for {} (PID: {})", active_package, game_pid);
            apply_performance_profile(active_game->lite_mode, active_package, game_pid);
            pid_tracker.set_pid(game_pid);

            if (active_game->enable_dnd) {
                game_requested_dnd = true;
                set_do_not_disturb(active_game->enable_dnd);
            } else {
                game_requested_dnd = false;
                set_do_not_disturb(false);
            }
        } else if (battery_saver_state && do_full_check) {
            if (cur_mode == POWERSAVE_PROFILE) goto take_me_to_the_bed;

            cur_mode = POWERSAVE_PROFILE;
            need_profile_checkup = false;
            LOGI("Applying powersave profile");
            apply_powersave_profile();

            if (game_requested_dnd) {
                set_do_not_disturb(false);
                game_requested_dnd = false;
            }
        } else if (do_full_check) {
            if (cur_mode == BALANCE_PROFILE) goto take_me_to_the_bed;

            cur_mode = BALANCE_PROFILE;
            need_profile_checkup = false;
            LOGI("Applying balance profile");
            apply_balance_profile();

            if (game_requested_dnd) {
                set_do_not_disturb(false);
                game_requested_dnd = false;
            }
        }

    take_me_to_the_bed:
        if (in_game_session) {
            std::this_thread::sleep_for(INGAME_LOOP_INTERVAL);
        } else {
            std::this_thread::sleep_for(NORMAL_LOOP_INTERVAL);
        }
    }

    // If we reach this, the daemon is dead
    return;
}

int main(int argc, char *argv[]) {
    auto NotifyFatalError = [](const std::string &error_msg) {
        notify(("ERROR: " + error_msg).c_str());
    };

    if (getuid() != 0) {
        fprintf(stderr, "\033[31mERROR:\033[0m Please run this program as root\n");
        return EXIT_FAILURE;
    }

    // Handle CLI
    if (argc >= 2) {
        return encore_cli(argc, argv);
    }

    if (!create_lock_file()) {
        fprintf(
            stderr,
            "\033[31mERROR:\033[0m Another instance of Encore Daemon is already running!\n");
        return EXIT_FAILURE;
    }

    if (!check_dumpsys_sanity()) {
        fprintf(stderr, "\033[31mERROR:\033[0m Dumpsys sanity check failed\n");
        NotifyFatalError("Dumpsys sanity check failed");
        LOGC("Dumpsys sanity check failed");
        return EXIT_FAILURE;
    }

    if (access(ENCORE_GAMELIST, F_OK) != 0) {
        fprintf(stderr, "\033[31mERROR:\033[0m %s is missing\n", ENCORE_GAMELIST);
        NotifyFatalError("gamelist.json is missing");
        LOGC("{} is missing", ENCORE_GAMELIST);
        return EXIT_FAILURE;
    }

    if (!load_gamelist_from_json(ENCORE_GAMELIST, game_registry)) {
        fprintf(stderr, "\033[31mERROR:\033[0m Failed to parse %s\n", ENCORE_GAMELIST);
        NotifyFatalError("Failed to parse gamelist.json");
        LOGC("Failed to parse {}", ENCORE_GAMELIST);
        return EXIT_FAILURE;
    }

    if (daemon(0, 0)) {
        LOGC("Failed to daemonize service");
        NotifyFatalError("Failed to daemonize service");
        return EXIT_FAILURE;
    }

    InotifyWatcher file_watcher;
    if (!init_file_watcher(file_watcher)) {
        LOGC("Failed to initialize file watcher");
        NotifyFatalError("Failed to initialize file watcher");
        return EXIT_FAILURE;
    }

    LOGI("Encore Tweaks daemon started");
    encore_main_daemon();

    // If we reach this, the daemon is dead
    LOGW("Encore Tweaks daemon exited");
}
