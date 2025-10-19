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

x_watcher* json_watcher = nullptr;

void on_gamelist_modified(
    XWATCHER_FILE_EVENT event, const char *path, int context, void *additional_data) {
    (void)context;
    (void)additional_data;

    if (event == XWATCHER_FILE_MODIFIED || event == XWATCHER_FILE_CREATED) {
        LOGI_TAG("XWatcher", "{} changed, reloading...", path);
        load_gamelist_from_json(path, game_registry);
    }
}

bool init_file_watcher(void) {
    json_watcher = xWatcher_create();
    if (!json_watcher) return false;

    xWatcher_reference ref;
    ref.path = const_cast<char*>(ENCORE_GAMELIST);
    ref.callback_func = on_gamelist_modified;
    ref.context = 0;
    ref.additional_data = nullptr;

    if (!xWatcher_appendFile(json_watcher, &ref)) {
        xWatcher_destroy(json_watcher);
        json_watcher = nullptr;
        return false;
    }

    return xWatcher_start(json_watcher);
}
