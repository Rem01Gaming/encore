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

#include "EncoreConfig.hpp"

enum WatchContext {
    WATCH_CONTEXT_GAMELIST,
    WATCH_CONTEXT_CONFIG,
};

void on_json_modified(
    const struct inotify_event *event, const std::string &path, int context,
    void *additional_data) {
    (void)additional_data;

    auto OnGamelistModified = [&](const std::string &path) -> void {
        LOGD_TAG("InotifyHandler", "Callback OnGamelistModified reached", path);
        load_gamelist_from_json(path, game_registry);
    };

//    auto OnConfigModified = [&](const std::string &path) -> void {
//
//    };

    // After the JSON was closed for writing
    if (event->mask & IN_CLOSE_WRITE) {
        switch (context) {
            case WATCH_CONTEXT_GAMELIST: OnGamelistModified(path); break;
            // case WATCH_CONTEXT_CONFIG: OnConfigModified(path); break;
            default: break;
        }
    }
}

bool init_file_watcher(InotifyWatcher& watcher) {
    try {
        InotifyWatcher::WatchReference gamelist_ref{
            ENCORE_GAMELIST,
            on_json_modified,
            WATCH_CONTEXT_GAMELIST,
            nullptr
        };

        watcher.addFile(gamelist_ref);
        watcher.start();

        return true;
    } catch (const std::runtime_error &e) {
        std::string error_msg = e.what();
        LOGE_TAG("InotifyWatcher", "{}", error_msg);
        return false;
    }
}
