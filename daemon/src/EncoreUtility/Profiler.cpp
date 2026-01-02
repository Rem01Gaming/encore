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

#include <algorithm>
#include <cstdlib>

#include "EncoreConfigStore.hpp"
#include "EncoreUtility.hpp"

void set_profiler_env_vars() {
    auto device_mitigation = config_store.get_device_mitigation();

    // Clear all existing _ENCORE_* environment variables
    extern char **environ;
    for (char **env = environ; *env; ++env) {
        std::string env_str(*env);
        if (env_str.find("ENCORE_") == 0) {
            // Extract the variable name (up to '=')
            size_t eq_pos = env_str.find('=');
            if (eq_pos != std::string::npos) {
                std::string var_name = env_str.substr(0, eq_pos);
                unsetenv(var_name.c_str());
            }
        }
    }

    // Set device mitigation variables
    if (device_mitigation.enable && !device_mitigation.items.empty()) {
        setenv("ENCORE_MITIGATION_ENABLED", "1", 1);

        // Set environment variable for each mitigation item
        for (const auto &item : device_mitigation.items) {
            std::string env_var = "ENCORE_" + item;
            std::transform(env_var.begin(), env_var.end(), env_var.begin(), [](unsigned char c) {
                // Convert to uppercase and replace non-alphanumeric characters with underscores
                if (!std::isalnum(c) && c != '_') return '_';
                return static_cast<char>(std::toupper(c));
            });

            setenv(env_var.c_str(), "1", 1);
            LOGD_TAG("Profiler", "Set mitigation env var: {}", env_var);
        }
    }

    // Set CPU Governor variables
    EncoreConfigStore::CPUGovernor cpu_governor_preference = config_store.get_cpu_governor();
    setenv("ENCORE_BALANCED_CPUGOV", cpu_governor_preference.balance.c_str(), 1);
    setenv("ENCORE_POWERSAVE_CPUGOV", cpu_governor_preference.powersave.c_str(), 1);
}

void run_perfcommon(void) {
    set_profiler_env_vars();

    if (system("encore_profiler perfcommon")) {
        LOGE("Unable to execute profiler changes to perfcommon");
    }
}

void apply_performance_profile(bool lite_mode, std::string game_pkg, pid_t game_pid) {
    is_kanged();
    set_profiler_env_vars();

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
    set_profiler_env_vars();

    write2file(GAME_INFO, "NULL 0 0\n");
    write2file(PROFILE_MODE, static_cast<int>(BALANCE_PROFILE), "\n");

    if (system("encore_profiler balance") != 0) {
        LOGE("Unable to execute profiler changes to balance");
    }
}

void apply_powersave_profile() {
    is_kanged();
    set_profiler_env_vars();

    write2file(GAME_INFO, "NULL 0 0\n");
    write2file(PROFILE_MODE, static_cast<int>(POWERSAVE_PROFILE), "\n");

    if (system("encore_profiler powersave") != 0) {
        LOGE("Unable to execute profiler changes to powersave");
    }
}
