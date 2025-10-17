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

#include "EncoreUtility.hpp"

void run_perfcommon(void) {
    if (systemv("encore_profiler perfcommon")) {
        LOGE("Unable to execute profiler changes to perfcommon");
    }
}

void apply_performance_profile(bool lite_mode, std::string game_pkg, pid_t game_pid) {
    is_kanged();

    uid_t game_uid = get_uid_by_package_name(game_pkg);
    write2file(GAME_INFO, game_pkg, " ", game_pid, " ", game_uid, "\n");
    write2file(PROFILE_MODE, static_cast<int>(PERFORMANCE_PROFILE), "\n");

    if (lite_mode) {
        if (system("encore_profiler performance_lite") != 0) {
            LOGE("Unable to execute profiler changes to performance_lite");
        }

        return;
    }

    if (system("encore_profiler performance") != 0) {
        LOGE("Unable to execute profiler changes to performance");
    }
}

void apply_balance_profile() {
    is_kanged();

    write2file(GAME_INFO, "NULL 0 0\n");
    write2file(PROFILE_MODE, static_cast<int>(BALANCE_PROFILE), "\n");

    if (system("encore_profiler balance") != 0) {
        LOGE("Unable to execute profiler changes to balance");
    }
}

void apply_powersave_profile() {
    is_kanged();

    write2file(GAME_INFO, "NULL 0 0\n");
    write2file(PROFILE_MODE, static_cast<int>(POWERSAVE_PROFILE), "\n");

    if (system("encore_profiler powersave") != 0) {
        LOGE("Unable to execute profiler changes to powersave");
    }
}
