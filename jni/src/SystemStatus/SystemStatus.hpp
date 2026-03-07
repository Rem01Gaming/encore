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

#pragma once

#include <cstdio>
#include <mutex>
#include <string>

#include <Encore.hpp>

/**
 * Snapshot of the system_status file written by SystemMonitor.java.
 *
 * File format (one key per line):
 *   focused_app  <package> <pid> <uid>
 *   screen_awake <0|1>
 *   battery_saver <0|1>
 *   zen_mode     <0|1|2|3>
 */
struct SystemStatus {
    std::string focused_app; ///< foreground package name, or "unknown"
    pid_t focused_pid = 0;
    uid_t focused_uid = 0;
    bool screen_awake = false;
    bool battery_saver = false;
    int zen_mode = 0; ///< 0 = off, 1-3 = various DND levels
};

namespace SystemStatusReader {

/**
 * @brief Parse SYSTEM_STATUS_FILE into @p out.
 *
 * Reads the file written by SystemMonitor.java and fills every field of
 * @p out.  Fields that are missing in the file are left at their zero-
 * initialised defaults so the caller always receives a valid struct.
 *
 * @param out   Destination struct.
 * @param path  Path to the status file (defaults to SYSTEM_STATUS_FILE).
 * @return true  if the file was opened and at least one field was parsed.
 * @return false if the file could not be opened.
 */
bool read(SystemStatus &out, const char *path = SYSTEM_STATUS_FILE);

} // namespace SystemStatusReader

/**
 * @brief Thread-safe wrapper around the latest SystemStatus snapshot.
 *
 * The InotifyHandler writes a fresh snapshot on every IN_CLOSE_WRITE
 * event; the main daemon loop reads the snapshot without blocking.
 */
class SystemStatusCache {
public:
    /** Replace the cached snapshot (called from inotify thread). */
    void update(const SystemStatus &s) {
        std::lock_guard<std::mutex> lk(mtx_);
        status_ = s;
        valid_ = true;
    }

    /**
     * Copy the latest snapshot into @p out.
     * @return false if no snapshot has been stored yet.
     */
    bool get(SystemStatus &out) const {
        std::lock_guard<std::mutex> lk(mtx_);
        if (!valid_) return false;
        out = status_;
        return true;
    }

    bool is_valid() const {
        std::lock_guard<std::mutex> lk(mtx_);
        return valid_;
    }

private:
    mutable std::mutex mtx_;
    SystemStatus status_;
    bool valid_ = false;
};

/** Global cache shared between the inotify thread and the main daemon. */
extern SystemStatusCache system_status_cache;
