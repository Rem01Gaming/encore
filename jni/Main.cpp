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
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <mutex>
#include <unistd.h>
#include <signal.h>
#include <fstream>
#include <string>
#include <filesystem>

#include "DeviceMitigationStore.hpp"
#include "EncoreConfigStore.hpp"
#include "InotifyHandler.hpp"
#include "Profiler.hpp"
#include "BinderMonitor.hpp"

#include <Encore.hpp>
#include <EncoreLog.hpp>
#include <EncoreUtility.hpp>
#include <GameRegistry.hpp>
#include <ModuleProperty.hpp>
#include <ShellUtility.hpp>
#include <SignalHandler.hpp>

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Global registry & state
// ---------------------------------------------------------------------------

GameRegistry game_registry;

struct DaemonState {
    EncoreProfileMode cur_mode = PERFCOMMON;
    std::string active_package;
    pid_t active_game_pid = 0;
    uid_t active_game_uid = 0;
    pid_t last_applied_pid = 0;

    // Is the game really on screen? The process observer only tells us about
    // events, not about the current state, and we deliberately ignore its
    // "background" events (some devices send stale ones). So this flag is
    // cleared by verifying against the activity manager, see schedule_foreground_recheck().
    bool game_foreground = false;
    uint64_t fg_generation = 0; // bumped on every foreground event, cancels stale rechecks

    bool screen_awake = true;
    bool battery_saver_state = false;
    bool game_requested_dnd = false;
    bool prev_dnd_state = false;
};

DaemonState g_state;
std::mutex g_state_mtx;
std::atomic<bool> daemon_stop_requested{false};

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
// Global event signaling
// ---------------------------------------------------------------------------

void signal_daemon_stop() {
    daemon_stop_requested.store(true, std::memory_order_relaxed);
    // Exit immediately since BinderMonitor::joinThreadPool() blocks the main thread
    _exit(0);
}

// ---------------------------------------------------------------------------
// Helper functions
// ---------------------------------------------------------------------------

static std::string remove_null_char(std::string raw) {
    if (auto null_pos = raw.find('\0'); null_pos != std::string::npos) {
        raw.resize(null_pos);
    }
    return raw;
}

static void clear_dnd_if_needed(DaemonState &state) {
    if (state.game_requested_dnd) {
        set_do_not_disturb(state.prev_dnd_state);
        state.game_requested_dnd = false;
    }
}

[[nodiscard]] static bool apply_game_profile(DaemonState &state) {
    auto *active_game = game_registry.find_game_ptr(state.active_package);
    if (!active_game) {
        LOGI("Game {} is no longer listed in registry", state.active_package);
        state.active_package.clear();
        state.active_game_pid = 0;
        state.active_game_uid = 0;
        state.last_applied_pid = 0;
        state.game_foreground = false;
        return false;
    }

    if (kill(state.active_game_pid, 0) != 0) {
        LOGW("Game {} (PID: {}) exited while applying profile, aborting session",
             state.active_package, state.active_game_pid);
        state.active_package.clear();
        state.active_game_pid = 0;
        state.active_game_uid = 0;
        state.last_applied_pid = 0;
        state.game_foreground = false;
        return false;
    }

    state.cur_mode = PERFORMANCE_PROFILE;
    LOGI("Applying performance profile for {} (PID: {})", state.active_package, state.active_game_pid);

    const bool lite_mode = active_game->lite_mode || config_store.get_preferences().enforce_lite_mode;
    apply_performance_profile(lite_mode, state.active_package, state.active_game_pid, state.active_game_uid);

    if (active_game->enable_dnd) {
        state.game_requested_dnd = true;
        set_do_not_disturb(true);
    } else {
        state.game_requested_dnd = false;
        set_do_not_disturb(state.prev_dnd_state);
    }
    return true;
}

static void evaluate_and_apply_profile(DaemonState &state) {
    // Track user's DND preference while we are not overriding it
    if (!state.game_requested_dnd) {
        state.prev_dnd_state = (BinderMonitor::get().getZenMode() != 0);
    }

    if (state.active_game_pid != 0 && state.game_foreground && state.screen_awake) {
        // Only skip if we are already in performance mode AND it's for the exact same game PID
        if (state.cur_mode == PERFORMANCE_PROFILE && state.last_applied_pid == state.active_game_pid) return;
        if (apply_game_profile(state)) {
            state.last_applied_pid = state.active_game_pid;
            return;
        }
    }

    if (state.battery_saver_state) {
        if (state.cur_mode == POWERSAVE_PROFILE) return;
        state.cur_mode = POWERSAVE_PROFILE;
        state.last_applied_pid = 0;
        state.active_game_uid = 0;
        LOGI("Applying powersave profile");
        apply_powersave_profile();
        clear_dnd_if_needed(state);
        return;
    }

    if (state.cur_mode == BALANCE_PROFILE) return;
    state.cur_mode = BALANCE_PROFILE;
    state.last_applied_pid = 0;
    state.active_game_uid = 0;
    LOGI("Applying balance profile");
    apply_balance_profile();
    clear_dnd_if_needed(state);
}

// ---------------------------------------------------------------------------
// Foreground verification
// ---------------------------------------------------------------------------

// A game process can stay alive in the background for a long time after the user left it.
// Without a check, a screen on/off or a Battery Saver toggle would bring performance profile
// back while the user is looking at something else. After a foreground event (or screen on)
// we ask the activity manager who is really on screen and only drop the game when another
// app clearly is. Unknown answers change nothing.
static void schedule_foreground_recheck(DaemonState &state, std::chrono::milliseconds delay) {
    if (state.active_game_pid == 0) return;

    const pid_t pid = state.active_game_pid;
    const std::string pkg = state.active_package;
    const uint64_t generation = ++state.fg_generation;

    std::thread([pid, pkg, generation, delay]() {
        std::this_thread::sleep_for(delay);

        {
            // A newer event already superseded this check, skip the expensive dumpsys
            std::lock_guard<std::mutex> lk(g_state_mtx);
            if (g_state.fg_generation != generation) return;
        }

        const ResumedState resumed = query_package_resumed(pkg);
        if (resumed == ResumedState::Unknown) return;

        std::lock_guard<std::mutex> lk(g_state_mtx);
        if (g_state.active_game_pid != pid || g_state.fg_generation != generation) return;

        const bool now_foreground = (resumed == ResumedState::Resumed);
        if (g_state.game_foreground == now_foreground) return;

        g_state.game_foreground = now_foreground;
        LOGI("{} (PID: {}) is {} the foreground", pkg, pid, now_foreground ? "back in" : "no longer in");
        evaluate_and_apply_profile(g_state);
    }).detach();
}

// Synchronous variant for callbacks that are about to re-evaluate the profile (screen on,
// Battery Saver toggle): drop the game first when another app is clearly on screen, so
// performance profile doesn't flash for a moment before the delayed recheck fixes it.
static void demote_game_if_backgrounded(DaemonState &state) {
    if (state.active_game_pid == 0 || !state.game_foreground) return;

    if (query_package_resumed(state.active_package) == ResumedState::NotResumed) {
        LOGI("{} (PID: {}) is alive but not on screen", state.active_package, state.active_game_pid);
        state.game_foreground = false;
        ++state.fg_generation;
    }
}

// ---------------------------------------------------------------------------
// Main daemon loop
// ---------------------------------------------------------------------------

static void encore_main_daemon() {
    run_perfcommon();

    auto& binder = BinderMonitor::get();
    if (!binder.initialize()) {
        LOGE("Failed to initialize BinderMonitor");
        notify_fatal_error("Failed to initialize BinderMonitor");
        return;
    }

    // Initialize state
    {
        std::lock_guard<std::mutex> lk(g_state_mtx);
        g_state.screen_awake = true;
        g_state.battery_saver_state = binder.isPowerSave();
        g_state.prev_dnd_state = (binder.getZenMode() != 0);
    }

    ProcessObserverCallbacks pocbs;

    pocbs.onForegroundActivitiesChanged = [&binder](int32_t pid, int32_t uid, bool foreground) {
        std::lock_guard<std::mutex> lk(g_state_mtx);
        LOGT("onForegroundActivitiesChanged: pid={}, uid={}, foreground={}", pid, uid, foreground);

        // Background events can be stale on some devices, so they never end the game
        // session by themselves. DND stays active as long as the game process is alive.
        // We only use them as a hint to double-check who is on screen.
        if (!foreground) {
            if (g_state.active_game_pid == pid && g_state.game_foreground) {
                schedule_foreground_recheck(g_state, std::chrono::milliseconds(1500));
            }
            return;
        }

        std::string pkg = remove_null_char(binder.getPackageNameForUid(uid));
        bool is_game = !pkg.empty() && game_registry.is_game_registered(pkg);
        if (!is_game) return;

        if (g_state.active_game_pid != pid) {
            // Switching to a new game (or starting a game)
            g_state.active_package = pkg;
            g_state.active_game_pid = pid;
            g_state.active_game_uid = uid;
            g_state.game_foreground = true;
            LOGI("Game {} came to foreground (PID: {})", pkg, pid);
            evaluate_and_apply_profile(g_state);
            // Make sure it really is on screen, foreground events can be spurious
            schedule_foreground_recheck(g_state, std::chrono::milliseconds(2000));
        } else if (!g_state.game_foreground) {
            // Same game, coming back from background
            g_state.game_foreground = true;
            LOGI("Game {} is back in the foreground (PID: {})", pkg, pid);
            evaluate_and_apply_profile(g_state);
            schedule_foreground_recheck(g_state, std::chrono::milliseconds(2000));
        }
    };

    pocbs.onProcessDied = [](int32_t pid, int32_t uid) {
        std::lock_guard<std::mutex> lk(g_state_mtx);
        LOGT("onProcessDied: pid={}, uid={}", pid, uid);

        if (g_state.active_game_pid == pid) {
            LOGI("Game {} (PID: {}) exited, resetting profile", g_state.active_package, pid);
            clear_dnd_if_needed(g_state);
            g_state.active_package.clear();
            g_state.active_game_pid = 0;
            g_state.last_applied_pid = 0;
            g_state.game_foreground = false;
            ++g_state.fg_generation;
            evaluate_and_apply_profile(g_state);
        }
    };

    binder.setProcessObserverCallbacks(pocbs);

    binder.setDisplayStateCallback([](bool isInteractive) {
        std::lock_guard<std::mutex> lk(g_state_mtx);
        LOGT("DisplayStateCallback: isInteractive={}", isInteractive);
        g_state.screen_awake = isInteractive;
        if (isInteractive) demote_game_if_backgrounded(g_state);
        evaluate_and_apply_profile(g_state);

        // Give the user time to get past the lock screen, then check what they landed on
        if (isInteractive && g_state.active_game_pid != 0) {
            schedule_foreground_recheck(g_state, std::chrono::milliseconds(3000));
        }
    });

    binder.setPowerSaveCallback([](bool isPowerSave) {
        std::lock_guard<std::mutex> lk(g_state_mtx);
        LOGT("PowerSaveCallback: isPowerSave={}", isPowerSave);
        g_state.battery_saver_state = isPowerSave;
        demote_game_if_backgrounded(g_state);
        evaluate_and_apply_profile(g_state);
    });

    // Initial profile evaluation
    {
        std::lock_guard<std::mutex> lk(g_state_mtx);
        evaluate_and_apply_profile(g_state);
    }

    LOGI("Encore Tweaks daemon started");
    set_module_description_status("\xF0\x9F\x98\x8B Tweaks applied successfully");

    // Block main thread until daemon is stopped
    binder.joinThreadPool();
}

// ---------------------------------------------------------------------------
// CMD Handler
// ---------------------------------------------------------------------------

int cmd_run_daemon() {
    std::atexit([]() {
        SignalHandler::cleanup_before_exit();
    });
    SignalHandler::setup_signal_handlers();

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

    InotifyWatcher file_watcher;
    if (!init_file_watcher(file_watcher)) {
        LOGC("Failed to initialize file watcher");
        notify_fatal_error("Failed to initialize file watcher");
        return EXIT_FAILURE;
    }

    encore_main_daemon();

    LOGW("Encore Tweaks daemon exited");
    SignalHandler::cleanup_before_exit();
    return EXIT_SUCCESS;
}

std::string get_module_version() {
    std::ifstream prop_file(MODULE_PROP);
    std::string line;
    std::string version = "unknown";

    if (!prop_file.is_open()) {
        std::cerr << "\033[33mERROR:\033[0m Could not open " << MODULE_PROP << std::endl;
        return version;
    }

    while (std::getline(prop_file, line)) {
        if (line.find("version=") == 0) {
            // Remove "version=" prefix
            version = line.substr(8);
            // Remove quotes if present
            if (!version.empty() && version.front() == '"' && version.back() == '"') {
                version = version.substr(1, version.length() - 2);
            }
            break;
        }
    }
    prop_file.close();
    return version;
}

int cmd_version() {
    std::string module_version = get_module_version();
    std::cout << "Encore Tweaks " << module_version << std::endl;
    std::cout << "Built on " << __TIME__ << " " << __DATE__ << std::endl;
    return EXIT_SUCCESS;
}

int cmd_setup_gamelist(const std::string& base_file_path) {
    bool success = GameRegistry::populate_from_base(ENCORE_GAMELIST, base_file_path);
    if (!success) {
        std::cerr << "\033[31mERROR:\033[0m Failed to setup gamelist from " << base_file_path << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int cmd_check_gamelist() {
    if (access(ENCORE_GAMELIST, F_OK) != 0) {
        std::cerr << "\033[33mERROR:\033[0m " << ENCORE_GAMELIST << " does not exist" << std::endl;
        return EXIT_FAILURE;
    }
    GameRegistry registry;
    if (!registry.load_from_json(ENCORE_GAMELIST)) {
        std::cerr << "\033[31mERROR:\033[0m Failed to parse " << ENCORE_GAMELIST << std::endl;
        return EXIT_FAILURE;
    }
    // stderr output is intentional for module installation
    std::cerr << ENCORE_GAMELIST << " is valid" << std::endl;
    std::cerr << "Registered games: " << registry.size() << std::endl;
    return EXIT_SUCCESS;
}

// ---------------------------------------------------------------------------
// Usage & Help
// ---------------------------------------------------------------------------

void print_usage(const std::string & program_name) {
    std::cout << "Encore Tweaks CLI\n\n";
    std::cout << "Usage: " << program_name << " <COMMAND> [OPTIONS]\n\n";
    std::cout << "Commands:\n";
    std::cout << "  daemon               Start Encore Tweaks daemon\n";
    std::cout << "  setup_gamelist       Setup initial gamelist from base file\n";
    std::cout << "  check_gamelist       Validate gamelist file\n";
    std::cout << "  version              Show version information\n";
    std::cout << "\nGlobal Options:\n";
    std::cout << "  -h, --help           Show this help message\n";
    std::cout << "  -V, --version        Show version information\n";
    std::cout << "\nRun '" << program_name << " <COMMAND> --help' for more information on a command.\n";
}

void print_daemon_help(const std::string & program_name) {
    std::cout << "Usage: " << program_name << " daemon\n\n";
    std::cout << "Start the Encore Tweaks background daemon.\n";
}

void print_setup_gamelist_help(const std::string & program_name) {
    std::cout << "Usage: " << program_name << " setup_gamelist <base_file_path>\n\n";
    std::cout << "Setup initial gamelist from a base file.\n\n";
    std::cout << "Arguments:\n";
    std::cout << "  <base_file_path>     Path to the base gamelist JSON file\n";
}

void print_check_gamelist_help(const std::string & program_name) {
    std::cout << "Usage: " << program_name << " check_gamelist\n\n";
    std::cout << "Validate the gamelist file and print registered games count.\n";
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

int main(int argc, char *argv[]) {
    if (getuid() != 0) {
        std::cerr << "\033[31mERROR:\033[0m Please run this program as root\n";
        return EXIT_FAILURE;
    }

    if (argc == 0 || argv[0] == nullptr) return EXIT_FAILURE;

    fs::path program_path = argv[0];
    std::string program_name = program_path.stem().string();

    if (argc < 2) {
        print_usage(program_name);
        return EXIT_FAILURE;
    }

    const std::string cmd = argv[1];
    bool is_sub_help = (argc >= 3 && (std::string(argv[2]) == "-h" || std::string(argv[2]) == "--help"));

    if (cmd == "-h" || cmd == "--help") {
        print_usage(program_name);
        return EXIT_SUCCESS;
    }

    if (cmd == "-V" || cmd == "--version" || cmd == "version") {
        return cmd_version();
    }

    if (cmd == "daemon") {
        if (is_sub_help) {
            print_daemon_help(program_name);
            return EXIT_SUCCESS;
        }

        return cmd_run_daemon();
    }

    if (cmd == "setup_gamelist") {
        if (is_sub_help) {
            print_setup_gamelist_help(program_name);
            return EXIT_SUCCESS;
        }

        if (argc != 3) {
            std::cerr << "\033[31mERROR:\033[0m Invalid arguments.\n";
            print_setup_gamelist_help(program_name);
            return EXIT_FAILURE;
        }

        return cmd_setup_gamelist(argv[2]);
    }

    if (cmd == "check_gamelist") {
        if (is_sub_help) {
            print_check_gamelist_help(program_name);
            return EXIT_SUCCESS;
        }

        return cmd_check_gamelist();
    }

    std::cerr << "\033[31mERROR:\033[0m Unknown command: " << cmd << "\n";
    std::cerr << "See '" << program_name << " --help' for available commands.\n";
    return EXIT_FAILURE;
}
