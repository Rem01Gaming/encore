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
#include <sys/syscall.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <fcntl.h>

#include "DeviceMitigationStore.hpp"
#include "EncoreCLI.hpp"
#include "EncoreConfigStore.hpp"
#include "Profiler.hpp"

#include <Encore.hpp>
#include <EncoreLog.hpp>
#include <EncoreUtility.hpp>
#include <GameRegistry.hpp>
#include <LockFile.hpp>
#include <ModuleProperty.hpp>
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

std::atomic<bool> daemon_stop_requested{false};

void signal_daemon_stop() {
    daemon_stop_requested.store(true, std::memory_order_relaxed);
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
    bool dnd_just_cleared = false;

    int focus_loss_count = 0;

    int pidfd = -1;
    pid_t tracked_pid = 0;
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
 * @brief Returns the PID of @p package_name, or 0 on failure.
 */
[[nodiscard]] static pid_t pidof_game(const std::string &package_name) {
    // We don't have a background cache, so we just check /proc directly.
    // The poll loop already updates state.system_status, but pidof_game is called inside apply_game_profile.
    // We can just rely on the fallback pidof() below.

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
bool is_dnd_enabled(const DaemonState &state) {
    return state.system_status.zen_mode != 0;
}

/**
 * @brief Pull the latest system-status snapshot from the inotify-fed cache.
 */
[[nodiscard]] static bool refresh_system_status(DaemonState &state) {
    return SystemStatusReader::read(state.system_status);
}

/**
 * @brief Clear Do Not Disturb if the game had requested it.
 */
static void clear_dnd_if_needed(DaemonState &state) {
    if (state.game_requested_dnd) {
        set_do_not_disturb(state.prev_dnd_state);
        state.game_requested_dnd = false;
        state.dnd_just_cleared = true;
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
    if (state.pidfd != -1) { close(state.pidfd); state.pidfd = -1; } state.tracked_pid = 0;
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
        if (state.pidfd != -1) { close(state.pidfd); state.pidfd = -1; } state.tracked_pid = 0;
        state.in_game_session = false;
        return false;
    }

    const pid_t game_pid = pidof_game(state.active_package);
    if (game_pid == 0) {
        LOGE("Unable to fetch PID of {}", state.active_package);
        state.active_package.clear();
        if (state.pidfd != -1) { close(state.pidfd); state.pidfd = -1; } state.tracked_pid = 0;
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
    const pid_t tracked_pid = (mlbb_pid != 0) ? mlbb_pid : game_pid;

    if (mlbb_pid != 0) {
        LOGD("Found UnityKillsMe thread for {} (PID: {}), tracking as game process", state.active_package, mlbb_pid);
    }

    if (kill(tracked_pid, 0) != 0) {
        LOGW(
            "Game {} (PID: {}) exited while applying profile ({}), aborting session",
            state.active_package,
            tracked_pid,
            strerror(errno)
        );

        state.active_package.clear();
        if (state.pidfd != -1) { close(state.pidfd); state.pidfd = -1; } state.tracked_pid = 0;
        state.in_game_session = false;
        state.need_profile_checkup = true;
        return false;
    }

    state.tracked_pid = tracked_pid;
#ifndef __NR_pidfd_open
#define __NR_pidfd_open 434
#endif
    if (state.pidfd != -1) { close(state.pidfd); state.pidfd = -1; }
    state.pidfd = syscall(__NR_pidfd_open, tracked_pid, 0);
    if (state.pidfd < 0) {
        LOGD("pidfd_open not available or failed for PID {}: {}", tracked_pid, strerror(errno));
    }

    // DND handling
    if (active_game->enable_dnd) {
        state.game_requested_dnd = true;
        set_do_not_disturb(true);
    } else {
        state.game_requested_dnd = false;
        set_do_not_disturb(state.prev_dnd_state);
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
    pthread_setname_np(pthread_self(), "MainThread");

    run_perfcommon();

    int inotify_fd = inotify_init1(IN_CLOEXEC | IN_NONBLOCK);
    if (inotify_fd < 0) {
        LOGC("Failed to init inotify: {}", strerror(errno));
        return;
    }

    int wd_status = inotify_add_watch(inotify_fd, SYSTEM_STATUS_FILE, IN_CLOSE_WRITE);
    int wd_gamelist = inotify_add_watch(inotify_fd, ENCORE_GAMELIST, IN_CLOSE_WRITE);
    int wd_config = inotify_add_watch(inotify_fd, CONFIG_FILE, IN_CLOSE_WRITE);
    int wd_mitigation = inotify_add_watch(inotify_fd, DEVICE_MITIGATION_FILE, IN_CLOSE_WRITE);
    int wd_modpath = inotify_add_watch(inotify_fd, MODPATH, IN_CREATE);

    // Initial read
    (void)refresh_system_status(state);

    while (!daemon_stop_requested.load(std::memory_order_relaxed)) {
        struct pollfd pfds[2];
        pfds[0].fd = inotify_fd;
        pfds[0].events = POLLIN;
        pfds[0].revents = 0;

        int nfds = 1;
        if (state.pidfd != -1) {
            pfds[1].fd = state.pidfd;
            pfds[1].events = POLLIN;
            pfds[1].revents = 0;
            nfds = 2;
        }

        // Poll with a 5000ms timeout to periodically check java_lock and fallback PID check
        const int ret = poll(pfds, nfds, 5000);
        
        if (ret < 0) {
            if (errno == EINTR) continue;
            LOGE_TAG("MainThread", "poll() failed: {}", strerror(errno));
            break;
        }

        if (daemon_stop_requested.load(std::memory_order_relaxed)) [[unlikely]]
            break;

        // Periodic watchdog checks
        if (!java_lock.is_locked()) {
            LOGC("Java daemon lock released, companion daemon exited or crashed, stopping daemon");
            notify_fatal_error("Java companion daemon crashed");
            break;
        }

        // Fallback PID check if pidfd is not supported
        if (state.in_game_session && state.pidfd == -1 && state.tracked_pid > 0) {
            if (kill(state.tracked_pid, 0) != 0) {
                handle_game_exit(state);
            }
        }

        // Process exit detected via pidfd
        if (nfds == 2 && (pfds[1].revents & POLLIN)) {
            handle_game_exit(state);
        }

        bool status_changed = false;

        // Inotify events
        if (pfds[0].revents & POLLIN) {
            char buf[4096] __attribute__((aligned(__alignof__(struct inotify_event))));
            ssize_t len;
            while ((len = read(inotify_fd, buf, sizeof(buf))) > 0) {
                const struct inotify_event *event;
                for (char *ptr = buf; ptr < buf + len; ptr += sizeof(struct inotify_event) + event->len) {
                    event = (const struct inotify_event *)ptr;
                    
                    if (event->wd == wd_status) {
                        status_changed = true;
                    } else if (event->wd == wd_gamelist) {
                        LOGD("Gamelist modified");
                        game_registry.load_from_json(ENCORE_GAMELIST);
                    } else if (event->wd == wd_config) {
                        if (config_store.reload()) {
                            EncoreLog::set_log_level(config_store.get_preferences().log_level);
                        }
                    } else if (event->wd == wd_mitigation) {
                        device_mitigation_store.load_config(DEVICE_MITIGATION_FILE);
                    } else if (event->wd == wd_modpath && event->len > 0) {
                        if (std::string(event->name) == "update") {
                            notify("Please reboot your device to complete module update.");
                            daemon_stop_requested.store(true, std::memory_order_relaxed);
                        }
                    }
                }
            }
            if (len < 0 && errno != EAGAIN) {
                LOGE("inotify read error: {}", strerror(errno));
            }
        }

        if (status_changed) {
            (void)refresh_system_status(state);
        }

        // Handle profile selection logic periodically or when status changes
        if (state.in_game_session && state.pidfd == -1 && state.tracked_pid == 0) {
            handle_game_exit(state);
        }

        if (!state.active_package.empty() && state.in_game_session) {
            if (!is_game_still_active(state)) [[unlikely]] {
                handle_game_exit(state);
            }
        }

        if (!state.game_requested_dnd) {
            if (state.dnd_just_cleared) {
                state.dnd_just_cleared = false;
            } else {
                state.prev_dnd_state = is_dnd_enabled(state);
            }
        }

        if (state.active_package.empty()) {
            state.active_package = get_active_game(state.system_status, game_registry);
            if (!state.active_package.empty()) {
                state.in_game_session = true;
                LOGD("DND state before in_game_session: {}", state.prev_dnd_state ? "ON" : "OFF");
            }
        }

        if (state.active_package.empty()) {
            state.battery_saver_state = state.system_status.battery_saver;
        }

        select_profile(state);
    }
    
    close(inotify_fd);
    if (state.pidfd != -1) close(state.pidfd);
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

    config_store.load_config();

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
