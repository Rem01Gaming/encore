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

#include <SystemStatus.hpp>

#include <cerrno>
#include <cstdio>
#include <cstring>

// Global singleton
SystemStatusCache system_status_cache;

namespace SystemStatusReader {

bool read(SystemStatus &out, const char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) return false;

    out = {}; // zero-init defaults
    bool parsed_any = false;

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        char val1[128] = {};
        int ival = 0;

        // focused_app <package> <pid> <uid>
        int pid = 0, uid = 0;
        if (sscanf(line, "focused_app %127s %d %d", val1, &pid, &uid) >= 1) {
            out.focused_app = val1;
            out.focused_pid = static_cast<pid_t>(pid);
            out.focused_uid = static_cast<uid_t>(uid);
            parsed_any = true;
            continue;
        }

        if (sscanf(line, "screen_awake %d", &ival) == 1) {
            out.screen_awake = (ival != 0);
            parsed_any = true;
            continue;
        }

        if (sscanf(line, "battery_saver %d", &ival) == 1) {
            out.battery_saver = (ival != 0);
            parsed_any = true;
            continue;
        }

        if (sscanf(line, "zen_mode %d", &ival) == 1) {
            out.zen_mode = ival;
            parsed_any = true;
            continue;
        }
    }

    fclose(fp);
    return parsed_any;
}

} // namespace SystemStatusReader
