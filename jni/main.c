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

#include <encore.h>

int main(void) {
    // Handle case when not running on root
    // Try grant KSU ROOT via prctl
    if (getuid() != 0 && ksu_grant_root() != true) {
        fprintf(stderr, "\033[31mERROR:\033[0m Please run this daemon as root\n");
        exit(EXIT_FAILURE);
    }

    // Make sure only one instance is running
    if (create_lock_file() != 0) {
        fprintf(stderr, "\033[31mERROR:\033[0m Another instance of Encore Daemon is already running!\n");
        exit(EXIT_FAILURE);
    }

    // Handle case when module modified by 3rd party
    is_kanged();

    // Handle missing Gamelist
    if (access(GAMELIST, F_OK) != 0) {
        fprintf(stderr, "\033[31mFATAL ERROR:\033[0m Unable to access Gamelist, either has been removed or moved.\n");
        log_encore(LOG_FATAL, "Critical file not found (%s)", GAMELIST);
        exit(EXIT_FAILURE);
    }

    // Daemonize service
    if (daemon(0, 0)) {
        log_encore(LOG_FATAL, "Unable to daemonize service");
        exit(EXIT_FAILURE);
    }

    // Register signal handlers
    signal(SIGINT, sighandler);
    signal(SIGTERM, sighandler);

    // Initialize variables
    char* gamestart = NULL;
    pid_t game_pid = 0;
    bool need_profile_checkup = false;
    MLBBState mlbb_is_running = MLBB_NOT_RUNNING;
    ProfileMode cur_mode = PERFCOMMON;

    log_encore(LOG_INFO, "Daemon started as PID %d", getpid());
    run_profiler(PERFCOMMON); // exec perfcommon

    while (1) {
        sleep(LOOP_INTERVAL);

        // Handle case when module gets updated
        if (access(MODULE_UPDATE, F_OK) == 0) [[clang::unlikely]] {
            log_encore(LOG_INFO, "Module update detected, exiting.");
            notify("Please reboot your device to complete module update.");
            break;
        }

        // Only fetch gamestart when user not in-game
        // prevent overhead from dumpsys commands.
        if (!gamestart) {
            gamestart = get_gamestart();
        } else if (game_pid != 0 && kill(game_pid, 0) == -1) [[clang::unlikely]] {
            log_encore(LOG_INFO, "Game %s exited, resetting profile...", gamestart);
            game_pid = 0;
            free(gamestart);
            gamestart = get_gamestart();

            // Force profile recheck to make sure new game session get boosted
            need_profile_checkup = true;
        }

        if (gamestart)
            mlbb_is_running = handle_mlbb(gamestart);

        if (gamestart && get_screenstate() && mlbb_is_running != MLBB_RUN_BG) {
            // Bail out if we already on performance profile
            // However we will pass this if need_profile_checkup was true
            if (!need_profile_checkup && cur_mode == PERFORMANCE_PROFILE)
                continue;

            // Get PID and check if the game is "real" running program
            // Handle weird behavior of MLBB
            game_pid = (mlbb_is_running == MLBB_RUNNING) ? mlbb_pid : pidof(gamestart);
            if (game_pid == 0) [[clang::unlikely]] {
                log_encore(LOG_ERROR, "Unable to fetch PID of %s", gamestart);
                free(gamestart);
                gamestart = NULL;
                continue;
            }

            cur_mode = PERFORMANCE_PROFILE;
            need_profile_checkup = false;
            log_encore(LOG_INFO, "Applying performance profile for %s", gamestart);
            run_profiler(PERFORMANCE_PROFILE);
            set_priority(game_pid);
        } else if (get_low_power_state()) {
            // Bail out if we already on powersave profile
            if (cur_mode == POWERSAVE_PROFILE)
                continue;

            cur_mode = POWERSAVE_PROFILE;
            need_profile_checkup = false;
            log_encore(LOG_INFO, "Applying powersave profile");
            run_profiler(POWERSAVE_PROFILE);
        } else {
            // Bail out if we already on normal profile
            if (cur_mode == NORMAL_PROFILE)
                continue;

            cur_mode = NORMAL_PROFILE;
            need_profile_checkup = false;
            log_encore(LOG_INFO, "Applying normal profile");
            run_profiler(NORMAL_PROFILE);
        }
    }

    return 0;
}
