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

#define LOCK_FILE "/data/adb/.config/encore/.lock"
#define LOG_FILE "/data/adb/.config/encore/encore.log"
#define PROFILE_MODE "/data/adb/.config/encore/current_profile"
#define GAME_INFO "/data/adb/.config/encore/gameinfo"
#define ENCORE_GAMELIST "/data/adb/.config/encore/gamelist.json"
#define MODULE_PROP "/data/adb/modules/encore/module.prop"
#define MODULE_UPDATE "/data/adb/modules/encore/update"

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
