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

#include "Dumpsys.hpp"

namespace Dumpsys {

void WindowDisplays(DumpsysWindowDisplays &result) {
    // Clear previous results
    result.screen_awake = false;
    result.recent_app.clear();

    auto pipe = popen_direct({"/system/bin/dumpsys", "window", "visible-apps"});

    if (!pipe.stream) {
        std::string error_msg = "popen failed: ";
        error_msg += strerror(errno);
        throw std::runtime_error(error_msg);
    }

    char buffer[1024];
    bool found_task_section = false;
    bool exited_task_section = false;
    bool found_awake = false;
    std::string current_task_line;

    while (fgets(buffer, sizeof(buffer), pipe.stream) != nullptr) {
        std::string line(buffer);

        // We've got all information needed, do not process any further
        if (exited_task_section && found_awake) {
            break;
        }

        // Remove trailing newline
        if (!line.empty() && line.back() == '\n') {
            line.pop_back();
        }

        // Check for screen awake state
        if (!found_awake && line.find("mAwake=") != std::string::npos) {
            result.screen_awake = line.find("mAwake=true") != std::string::npos;
            found_awake = true;
            continue;
        }

        // Look for task section start
        if (!found_task_section && line.find("Application tokens in top down Z order:") != std::string::npos) {
            found_task_section = true;
            continue;
        }

        // Bailout if we aren't in task section
        if (!found_task_section || exited_task_section) continue;

        // Check if we've reached the end of the task section
        if (line.empty()) {
            exited_task_section = true;
            continue;
        }

        // Look for task lines and store it for processing
        if (line.find("* Task{") != std::string::npos && line.find("type=standard") != std::string::npos) {
            current_task_line = line;
        }
        // Look for ActivityRecord lines that follow task lines
        else if (!current_task_line.empty() && line.find("* ActivityRecord{") != std::string::npos) {
            RecentAppList app;

            // Extract visibility from the task line
            app.visible = current_task_line.find("visible=true") != std::string::npos;

            // Extract package name from ActivityRecord line
            // Format: * ActivityRecord{91afba8 u0 com.termux/.app.TermuxActivity t1624}
            size_t u0_pos = line.find(" u0 ");
            if (u0_pos != std::string::npos) {
                size_t package_start = u0_pos + 4; // Skip " u0 "
                size_t slash_pos = line.find("/", package_start);
                if (slash_pos != std::string::npos) {
                    app.package_name = line.substr(package_start, slash_pos - package_start);
                    result.recent_app.push_back(app);
                }
            }

            // Clear the current task line after processing
            current_task_line.clear();
        }
    }

    // Handle missing information
    if (!found_task_section) {
        throw std::runtime_error("unable to find task section");
    }

    if (!found_awake) {
        throw std::runtime_error("unable to find screen state info");
    }
}

void Power(DumpsysPower &result) {
    // Clear previous results
    result.screen_awake = false;
    result.is_plugged = false;
    result.battery_saver = false;
    result.battery_saver_sticky = false;

    auto pipe = popen_direct({"/system/bin/dumpsys", "power"});

    if (!pipe.stream) {
        std::string error_msg = "popen failed: ";
        error_msg += strerror(errno);
        throw std::runtime_error(error_msg);
    }

    char buffer[1024];
    bool found_wakefulness = false;
    bool found_is_plugged = false;
    bool found_battery_saver = false;
    bool found_battery_saver_sticky = false;

    while (fgets(buffer, sizeof(buffer), pipe.stream) != nullptr) {
        std::string line(buffer);

        // We've got all information needed, do not process any further
        if (found_wakefulness && found_is_plugged && found_battery_saver && found_battery_saver_sticky) {
            break;
        }

        // Remove trailing newline
        if (!line.empty() && line.back() == '\n') {
            line.pop_back();
        }

        if (!found_wakefulness && line.find("mWakefulness=") != std::string::npos) {
            result.screen_awake = line.find("mWakefulness=Awake") != std::string::npos;
            found_wakefulness = true;
            continue;
        }

        if (!found_is_plugged && line.find("mIsPowered=") != std::string::npos) {
            result.is_plugged = line.find("mIsPowered=true") != std::string::npos;
            found_is_plugged = true;
            continue;
        }

        if (!found_battery_saver && line.find("mSettingBatterySaverEnabled=") != std::string::npos) {
            result.battery_saver = line.find("mSettingBatterySaverEnabled=true") != std::string::npos;
            found_battery_saver = true;
            continue;
        }

        if (!found_battery_saver_sticky && line.find("mSettingBatterySaverEnabledSticky=") != std::string::npos) {
            result.battery_saver_sticky = line.find("mSettingBatterySaverEnabledSticky=true") != std::string::npos;
            found_battery_saver_sticky = true;
            continue;
        }
    }

    // Handle missing information
    if (!found_wakefulness) {
        throw std::runtime_error("unable to find wakefulness state");
    }

    if (!found_is_plugged) {
        throw std::runtime_error("unable to find charging state");
    }

    if (!found_battery_saver) {
        throw std::runtime_error("unable to find battery saver state");
    }

    if (!found_battery_saver_sticky) {
        throw std::runtime_error("unable to find battery saver sticky state");
    }
}

pid_t GetAppPID(const std::string &package_name) {
    auto pipe = popen_direct({"/system/bin/dumpsys", "activity", "top"});

    if (!pipe.stream) {
        std::string error_msg = "popen failed: ";
        error_msg += strerror(errno);
        throw std::runtime_error(error_msg);
    }

    char buffer[1024];
    bool found_package = false;
    int pid = 0;

    while (fgets(buffer, sizeof(buffer), pipe.stream) != nullptr) {
        // Found our package, no need to continue
        if (found_package) break;

        std::string line(buffer);

        // Remove trailing newline
        if (!line.empty() && line.back() == '\n') {
            line.pop_back();
        }

        // Look for ACTIVITY line containing our package
        if (line.contains("ACTIVITY") && line.contains(package_name)) {
            found_package = true;

            // Extract PID from the line
            // Format: ACTIVITY tw.nekomimi.nekogram/org.telegram.messenger.NoxIcon ac9376b pid=32288
            size_t pid_pos = line.find("pid=");
            if (pid_pos != std::string::npos) {
                std::string pid_str = line.substr(pid_pos + 4); // Skip "pid="

                // Remove any trailing non-digit characters
                pid_str = pid_str.substr(0, pid_str.find_first_not_of("0123456789"));

                try {
                    pid = std::stoi(pid_str);
                } catch (const std::exception &e) {
                    throw std::runtime_error("failed to parse PID: " + std::string(e.what()));
                }
            }
        }
    }

    if (!found_package) {
        throw std::runtime_error("unable to find " + package_name + "package in activity top");
    }

    if (pid == 0) {
        throw std::runtime_error("unable to extract PID for " + package_name);
    }

    return pid;
}

} // namespace Dumpsys
