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

#pragma once

#include <string>

#define NOTIFY_TITLE "Encore Tweaks"
#define LOG_TAG "EncoreTweaks"

#define CONFIG_DIR "/data/adb/.config/encore"
#define MODPATH "/data/adb/modules/encore"

#define LOCK_FILE CONFIG_DIR "/.lock"
#define LOG_FILE CONFIG_DIR "/encore.log"
#define PROFILE_MODE CONFIG_DIR "/current_profile"
#define GAME_INFO CONFIG_DIR "/gameinfo"
#define CONFIG_FILE CONFIG_DIR "/config.json"
#define DEFAULT_CPU_GOV CONFIG_DIR "/default_cpu_gov"
#define ENCORE_GAMELIST CONFIG_DIR "/gamelist.json"

#define MODULE_PROP MODPATH "/module.prop"
#define MODULE_UPDATE MODPATH "/update"

enum EncoreProfileMode : char {
    PERFCOMMON,
    PERFORMANCE_PROFILE,
    BALANCE_PROFILE,
    POWERSAVE_PROFILE
};

struct EncoreGameList {
    std::string package_name;
    bool lite_mode;
    bool enable_dnd;
};
