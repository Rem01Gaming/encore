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

#include "EncoreConfig.hpp"

#include <DeviceMitigationStore.hpp>
#include <Encore.hpp>
#include <EncoreConfigStore.hpp>
#include <EncoreLog.hpp>
#include <GameRegistry.hpp>
#include <SystemStatus.hpp>

// signal_daemon_update and signal_daemon_stop are defined in Main.cpp
extern void signal_daemon_update();
extern void signal_daemon_stop();

enum WatchContext {
    WATCH_CONTEXT_GAMELIST,
    WATCH_CONTEXT_CONFIG,
    WATCH_CONTEXT_DEVICE_MITIGATION,
    WATCH_CONTEXT_SYSTEM_STATUS,
    WATCH_CONTEXT_MODULE_UPDATE,
};

void on_json_modified(const struct inotify_event *event, const std::string &path, int context, void *additional_data) {
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

    auto OnSystemStatusModified = [&](const std::string &path) -> void {
        // Spammy log...
        // LOGD_TAG("InotifyHandler", "Callback OnSystemStatusModified reached");

        SystemStatus status;
        if (SystemStatusReader::read(status, path.c_str())) {
            system_status_cache.update(status);
            signal_daemon_update(); // Wake up the daemon immediately
        } else {
            LOGW_TAG("InotifyHandler", "Failed to parse system_status file: {}", path);
        }
    };

    auto OnModuleUpdateCreated = [&]() -> void {
        LOGI_TAG("InotifyHandler", "Module update file detected, signaling daemon to stop");
        notify("Please reboot your device to complete module update.");
        signal_daemon_stop();
    };

    // React immediately after the writer has closed the file
    if (event->mask & IN_CLOSE_WRITE) {
        switch (context) {
            case WATCH_CONTEXT_GAMELIST: OnGamelistModified(path); break;
            case WATCH_CONTEXT_CONFIG: OnConfigModified(path); break;
            case WATCH_CONTEXT_DEVICE_MITIGATION: OnDeviceMitigationModified(path); break;
            case WATCH_CONTEXT_SYSTEM_STATUS: OnSystemStatusModified(path); break;
            default: break;
        }
    }

    // React when MODULE_UPDATE is created inside MODPATH directory
    if ((event->mask & IN_CREATE) && context == WATCH_CONTEXT_MODULE_UPDATE) {
        if (std::string(event->name) == "update") {
            OnModuleUpdateCreated();
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

        // Seed the cache with whatever is already on-disk (if any)
        {
            SystemStatus initial;
            if (SystemStatusReader::read(initial)) {
                system_status_cache.update(initial);
                LOGD_TAG("InotifyHandler", "Pre-seeded SystemStatusCache from existing status file");
            }
        }

        // Set up file watchers
        InotifyWatcher::WatchReference gamelist_ref{ENCORE_GAMELIST, on_json_modified, WATCH_CONTEXT_GAMELIST, nullptr};

        InotifyWatcher::WatchReference config_ref{CONFIG_FILE, on_json_modified, WATCH_CONTEXT_CONFIG, nullptr};

        InotifyWatcher::WatchReference device_mitigation_ref{
            DEVICE_MITIGATION_FILE, on_json_modified, WATCH_CONTEXT_DEVICE_MITIGATION, nullptr
        };

        InotifyWatcher::WatchReference system_status_ref{
            SYSTEM_STATUS_FILE, on_json_modified, WATCH_CONTEXT_SYSTEM_STATUS, nullptr
        };

        // Watch the module directory for creation of the "update" file.
        // The file does not exist yet, so we must watch the parent directory.
        InotifyWatcher::WatchReference module_update_ref{MODPATH, on_json_modified, WATCH_CONTEXT_MODULE_UPDATE, nullptr};

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

        if (!watcher.addFile(system_status_ref)) {
            LOGE_TAG("InotifyWatcher", "Failed to add system_status watch");
            return false;
        }

        if (!watcher.addFile(module_update_ref)) {
            LOGE_TAG("InotifyWatcher", "Failed to add module update watch");
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
