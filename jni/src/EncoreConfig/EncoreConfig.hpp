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

#include <Encore.hpp>
#include <EncoreLog.hpp>

#include <x-watcher/x-watcher.h>

extern x_watcher *json_watcher;

bool init_file_watcher(void);

void on_gamelist_modified(
    XWATCHER_FILE_EVENT event, const char *path, int context, void *additional_data);
bool load_gamelist_from_json(const std::string &filename, std::vector<EncoreGameList> &gamelist);
