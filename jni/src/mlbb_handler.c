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

// Cached MLBB PID
pid_t mlbb_pid = 0;

/***********************************************************************************
 * Function Name      : handle_mlbb
 * Inputs             : const char *gamestart - Game package name
 * Returns            : MLBBState (enum)
 * Description        : Checks if Mobile Legends: Bang Bang IS actually running
 *                      on foreground, not in the background.
 ***********************************************************************************/
MLBBState handle_mlbb(const char* gamestart) {
    // Is Gamestart MLBB?
    if (IS_MLBB(gamestart) == false) {
        mlbb_pid = 0;
        return MLBB_NOT_RUNNING;
    }

    // Check if cached PID is still valid
    if (mlbb_pid != 0) {
        if (kill(mlbb_pid, 0) == 0) [[clang::likely]] {
            return MLBB_RUNNING;
        }

        mlbb_pid = 0;
    }

    // Concatenate gamestart with ':UnityKillsMe'
    char mlbb_proc[40];
    snprintf(mlbb_proc, sizeof(mlbb_proc), "%s%s", gamestart, ":UnityKillsMe");

    // Fetch new PID if cache is invalid
    mlbb_pid = pidof(mlbb_proc);
    if (mlbb_pid != 0) {
        log_encore(LOG_INFO, "Boosting MLBB process %s", mlbb_proc);
        return MLBB_RUNNING;
    }

    // MLBB is in the background
    return MLBB_RUN_BG;
}
