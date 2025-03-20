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

/***********************************************************************************
 * Function Name      : handle_mlbb
 * Inputs             : const char *gamestart - Game package name
 * Returns            : char - 2 if MLBB is running in foreground
 *                             1 if MLBB is running in background
 *                             0 if gamestart is not MLBB
 * Description        : Checks if Mobile Legends: Bang Bang IS actually running
 *                      on foreground, not in the background.
 ***********************************************************************************/
char handle_mlbb(const char* gamestart) {
    static pid_t cached_pid = -1;

    // Is Gamestart MLBB?
    if (IS_MLBB(gamestart) != true) {
        cached_pid = -1;
        return 0;
    }

    // Check if cached PID is still valid
    if (cached_pid != -1) {
        if (kill(cached_pid, 0) == 0) [[clang::likely]] {
            return 2;
        }

        cached_pid = -1;
    }

    // Fetch new PID if cache is invalid
    cached_pid = pidof(GAME_STRESS);
    if (cached_pid != -1)
        return 2;

    // MLBB is in the background
    return 1;
}
