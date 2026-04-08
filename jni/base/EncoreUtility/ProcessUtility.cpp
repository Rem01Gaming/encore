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
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "EncoreUtility.hpp"

namespace fs = std::filesystem;

pid_t pidof(std::string_view target_name, bool strict) {
    if (target_name.empty()) return 0;

    pid_t found_pid = 0;

    try {
        for (const auto &entry : fs::directory_iterator("/proc")) {
            const std::string dir_name = entry.path().filename().string();

            // Skip non-process directories
            if (!entry.is_directory() || !std::all_of(dir_name.begin(), dir_name.end(), ::isdigit)) {
                continue;
            }

            std::ifstream cmd_file(entry.path() / "cmdline", std::ios::binary);
            if (!cmd_file.is_open()) continue;

            // Read ONLY the first null-terminated string
            std::string actual_name;
            if (!std::getline(cmd_file, actual_name, '\0')) {
                continue;
            }

            bool is_match = false;
            if (strict) {
                // "com.mobile.legends" matches ONLY "com.mobile.legends"
                is_match = (actual_name == target_name);
            } else {
                // "com.mobile.legends" matches "com.mobile.legends:anything"
                is_match = (actual_name.find(target_name) != std::string::npos);
            }

            if (is_match) {
                found_pid = static_cast<pid_t>(std::stoul(dir_name));
                break;
            }
        }
    } catch (const fs::filesystem_error &e) {
        LOGE_TAG("pidof", "Filesystem error: %s", e.what());
    } catch (const std::exception &e) {
        LOGE_TAG("pidof", "Error: %s", e.what());
    } catch (...) {
        LOGE_TAG("pidof", "Unknown exception occurred");
    }

    return found_pid;
}

uid_t get_uid_by_package_name(const std::string &package_name) {
    struct stat st{};

    if (stat(("/data/data/" + package_name).c_str(), &st) != 0) {
        return 0;
    }

    return st.st_uid;
}
