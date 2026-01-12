/*
 * Copyright (C) 2026 Rem01Gaming
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

#include "EncoreConfig.hpp"

#include <DeviceMitigationStore.hpp>
#include <Encore.hpp>
#include <EncoreConfigStore.hpp>
#include <EncoreLog.hpp>
#include <GameRegistry.hpp>

enum WatchContext {
    WATCH_CONTEXT_GAMELIST,
    WATCH_CONTEXT_CONFIG,
    WATCH_CONTEXT_DEVICE_MITIGATION,
};

void on_json_modified(
    const struct inotify_event *event, const std::string &path, int context,
    void *additional_data) {
    (void)additional_data;

    auto OnGamelistModified = [&](const std::string &path) -> void {
        LOGD_TAG("InotifyHandler", "Callback OnGamelistModified reached");
        game_registry.load_from_json(path);
    };

    auto OnDeviceMitigationModified = [&](const std::string &path) -> void {
        LOGD_TAG("InotifyHandler", "Callback OnDeviceMitigationModified reached");
        device_mitigation_store.load_config(path);
    };

    auto OnConfigModified = [&](const std::string &path) -> void {
        LOGD_TAG("InotifyHandler", "Callback OnConfigModified reached");

        if (!config_store.reload()) {
            LOGW_TAG("InotifyHandler", "Failed to reload config from {}", path);
            return;
        }

        // Apply new log level
        auto prefs = config_store.get_preferences();
        EncoreLog::set_log_level(prefs.log_level);
    };

    // After the JSON was closed for writing
    if (event->mask & IN_CLOSE_WRITE) {
        switch (context) {
            case WATCH_CONTEXT_GAMELIST: OnGamelistModified(path); break;
            case WATCH_CONTEXT_CONFIG: OnConfigModified(path); break;
            case WATCH_CONTEXT_DEVICE_MITIGATION: OnDeviceMitigationModified(path); break;
            default: break;
        }
    }
}

bool init_file_watcher(InotifyWatcher &watcher) {
    try {
        // Initialize config store first
        if (!config_store.load_config()) {
            LOGE_TAG("EncoreConfig", "Failed to load config file");
            return false;
        }

        // Apply log level from config
        auto prefs = config_store.get_preferences();
        EncoreLog::set_log_level(prefs.log_level);

        // Set up file watchers
        InotifyWatcher::WatchReference gamelist_ref{
            ENCORE_GAMELIST, on_json_modified, WATCH_CONTEXT_GAMELIST, nullptr};

        InotifyWatcher::WatchReference config_ref{
            CONFIG_FILE, on_json_modified, WATCH_CONTEXT_CONFIG, nullptr};

        InotifyWatcher::WatchReference device_mitigation_ref{
            DEVICE_MITIGATION_FILE, on_json_modified, WATCH_CONTEXT_DEVICE_MITIGATION, nullptr};

        if (!watcher.addFile(gamelist_ref)) {
            LOGE_TAG("InotifyWatcher", "Failed to add gamelist watch");
            return false;
        }

        if (!watcher.addFile(config_ref)) {
            LOGE_TAG("InotifyWatcher", "Failed to add config watch");
            return false;
        }

        if (!watcher.addFile(device_mitigation_ref)) {
            LOGE_TAG("InotifyWatcher", "Failed to add device mitigation watch");
            return false;
        }

        watcher.start();
        return true;
    } catch (const std::runtime_error &e) {
        std::string error_msg = e.what();
        LOGE_TAG("InotifyWatcher", "{}", error_msg);
        return false;
    }
}
