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

#include <EncoreLog.hpp>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string_view>
#include "EncoreUtility.hpp"

pid_t pidof(std::string_view target_name, bool strict) {
    if (target_name.empty()) return 0;
    
    DIR* dir = opendir("/proc");
    if (!dir) return 0;

    pid_t found_pid = 0;
    struct dirent* entry;
    char cmdline_path[256];
    char buffer[256];

    while ((entry = readdir(dir)) != nullptr) {
        // Skip non-directories and non-numeric directories
        if (entry->d_type != DT_DIR && entry->d_type != DT_UNKNOWN) continue;
        if (!isdigit(entry->d_name[0])) continue;

        snprintf(cmdline_path, sizeof(cmdline_path), "/proc/%s/cmdline", entry->d_name);
        int fd = open(cmdline_path, O_RDONLY | O_CLOEXEC);
        if (fd < 0) continue;

        ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
        close(fd);

        if (bytes_read > 0) {
            // First string in cmdline is null-terminated
            buffer[bytes_read] = '\0';
            std::string_view actual_name(buffer);
            
            bool is_match = strict ? (actual_name == target_name) 
                                   : (actual_name.find(target_name) != std::string_view::npos);
            
            if (is_match) {
                found_pid = static_cast<pid_t>(std::atoi(entry->d_name));
                break;
            }
        }
    }
    
    closedir(dir);
    return found_pid;
}

uid_t get_uid_by_package_name(const std::string &package_name) {
    struct stat st{};

    if (stat(("/data/data/" + package_name).c_str(), &st) != 0) {
        return 0;
    }

    return st.st_uid;
}
