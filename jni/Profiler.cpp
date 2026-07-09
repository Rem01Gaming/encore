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

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <filesystem>

#include "Encore.hpp"
#include "EncoreLog.hpp"
#include "Profiler.hpp"
#include "Write2File.hpp"
#include "DeviceMitigationStore.hpp"
#include "EncoreConfigStore.hpp"

#include <EncoreUtility.hpp>

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Helper functions
// ---------------------------------------------------------------------------

void set_profiler_env_vars() {
    // Get preferences from config store
    auto prefs = config_store.get_preferences();

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

    // Use cached mitigation items instead of re-evaluating rules
    auto mitigation_items = device_mitigation_store.get_cached_mitigation_items(prefs.use_device_mitigation);

    // Set environment variable for mitigation items
    for (const auto &item : mitigation_items) {
        std::string env_var = "ENCORE_" + item;
        std::transform(env_var.begin(), env_var.end(), env_var.begin(), [](unsigned char c) {
            if (!std::isalnum(c) && c != '_') return '_';
            return static_cast<char>(std::toupper(c));
        });

        setenv(env_var.c_str(), "1", 1);
        LOGD_TAG("Profiler", "Set mitigation env var: {}", env_var);
    }

    // Set CPU Governor variables
    EncoreConfigStore::CPUGovernor cpu_governor_preference = config_store.get_cpu_governor();
    setenv("ENCORE_BALANCED_CPUGOV", cpu_governor_preference.balance.c_str(), 1);
    setenv("ENCORE_POWERSAVE_CPUGOV", cpu_governor_preference.powersave.c_str(), 1);
}

// ---------------------------------------------------------------------------
// cpuctl management
// ---------------------------------------------------------------------------

static const fs::path cpuctl_path = "/dev/cpuctl/top-app/encore";
static const fs::path stune_path = "/dev/stune/top-app/encore";

void init_cpuctl() {
    if (!fs::create_directories(cpuctl_path)) {
        LOGE_TAG("Profiler", "Failed to create cpuctl directory");
        return;
    }

    write2file(cpuctl_path / "cpu.shares", "2048");
    write2file(cpuctl_path / "cpu.uclamp.latency_sensitive", "1");

    fs::path min_uclamp_path = cpuctl_path / "cpu.uclamp.min";
    if (fs::exists(min_uclamp_path)) {
        write2file(min_uclamp_path, "25");
    }

    if (fs::exists("/dev/stune")) {
        if (!fs::create_directories(stune_path)) {
            LOGE_TAG("Profiler", "Failed to create stune directory");
        }

        fs::path min_stune_path = stune_path / "schedtune.util.min";
        if (fs::exists(min_stune_path)) {
            write2file(min_stune_path, "25");
        }
    }
}

void change_min_uclamp(int min_uclamp) {
    if (min_uclamp < 0) min_uclamp = 0;
    if (min_uclamp > 100) min_uclamp = 100;

    fs::path min_uclamp_path = cpuctl_path / "cpu.uclamp.min";
    if (fs::exists(min_uclamp_path)) {
        write2file(min_uclamp_path, min_uclamp);
    }

    fs::path min_stune_path = stune_path / "schedtune.util.min";
    if (fs::exists(min_stune_path)) {
        write2file(min_stune_path, min_uclamp);
    }
}

void add_process_to_cpuctl(pid_t pid) {
    fs::path cpuctl_procs = cpuctl_path / "cgroup.procs";
    if (fs::exists(cpuctl_procs)) {
        write2file(cpuctl_procs, pid);
    }

    fs::path stune_procs = stune_path / "cgroup.procs";
    if (fs::exists(stune_procs)) {
        write2file(stune_procs, pid);
    }
}

// ---------------------------------------------------------------------------
// Profile management
// ---------------------------------------------------------------------------

void run_perfcommon(void) {
    write2file(GAME_INFO, "NULL 0 0\n");
    write2file(PROFILE_MODE, static_cast<int>(PERFCOMMON), "\n");

    if (config_store.get_preferences().disable_tweaks) {
        LOGI_TAG("Profiler", "Tweaks are disabled in config, skipping perfcommon");
        return;
    }

    init_cpuctl();
    set_profiler_env_vars();

    if (system("encore_profiler perfcommon")) {
        LOGE("Unable to execute profiler changes to perfcommon");
    }
}

void apply_performance_profile(bool lite_mode, std::string game_pkg, pid_t game_pid, uid_t game_uid) {
    write2file(GAME_INFO, game_pkg, " ", game_pid, " ", game_uid, "\n");
    write2file(PROFILE_MODE, static_cast<int>(PERFORMANCE_PROFILE), "\n");

    if (config_store.get_preferences().disable_tweaks) {
        LOGI_TAG("Profiler", "Tweaks are disabled in config, skipping performance profile");
        return;
    }

    add_process_to_cpuctl(game_pid);
    set_profiler_env_vars();

    if (lite_mode) {
        LOGD("Lite mode is enabled");
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
    write2file(GAME_INFO, "NULL 0 0\n");
    write2file(PROFILE_MODE, static_cast<int>(BALANCE_PROFILE), "\n");

    if (config_store.get_preferences().disable_tweaks) {
        LOGI_TAG("Profiler", "Tweaks are disabled in config, skipping balance profile");
        return;
    }

    set_profiler_env_vars();

    if (system("encore_profiler balance") != 0) {
        LOGE("Unable to execute profiler changes to balance");
    }
}

void apply_powersave_profile() {
    write2file(GAME_INFO, "NULL 0 0\n");
    write2file(PROFILE_MODE, static_cast<int>(POWERSAVE_PROFILE), "\n");

    if (config_store.get_preferences().disable_tweaks) {
        LOGI_TAG("Profiler", "Tweaks are disabled in config, skipping powersave profile");
        return;
    }

    set_profiler_env_vars();

    if (system("encore_profiler powersave") != 0) {
        LOGE("Unable to execute profiler changes to powersave");
    }
}
