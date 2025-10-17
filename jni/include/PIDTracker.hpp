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

#pragma once

#include <atomic>
#include <thread>
#include <unistd.h>

#include <EncoreLog.hpp>

/**
 * @class PIDTracker
 * @brief A class to track the status of a PID in a background thread.
 *
 * This class continuously monitors a given PID to check if the process is still running.
 * It automatically invalidates the PID when the process terminates.
 */
class PIDTracker {
private:
    /// The current process ID being tracked.
    std::atomic<pid_t> current_pid{0};
    /// Flag to signal the tracking thread to stop.
    std::atomic<bool> stop_requested{false};
    /// Flag indicating whether the current PID is considered valid.
    std::atomic<bool> pid_valid{false};
    /// The background thread that performs the tracking.
    std::thread tracking_thread;

    /**
     * @brief Checks if a process with the given PID is running.
     * @param pid The process ID to check.
     * @return True if the process is running, false otherwise.
     */
    bool is_pid_running(pid_t pid) {
        return kill(pid, 0) == 0;
    }

    /**
     * @brief The main loop for the background tracking thread.
     *
     * This loop periodically checks the status of the `current_pid`.
     * Marking PID as invalid as soon as the process terminates.
     */
    void tracking_loop() {
        while (!stop_requested.load(std::memory_order_acquire)) {
            pid_t pid = current_pid.load(std::memory_order_acquire);

            if (pid == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                continue;
            }

            if (is_pid_running(pid)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(60));
                continue;
            }

            if (current_pid.compare_exchange_strong(pid, 0, std::memory_order_acq_rel)) {
                pid_valid.store(false, std::memory_order_release);
                LOGD_TAG("PIDTracker", "PID {} terminated", pid);
            }
        }
    }

public:
    /**
     * @brief Constructs a PIDTracker and starts the background tracking thread.
     */
    PIDTracker() {
        tracking_thread = std::thread(&PIDTracker::tracking_loop, this);
    }

    /**
     * @brief Destroys the PIDTracker, signals the tracking thread to stop.
     */
    ~PIDTracker() {
        // Signal the thread to stop and wait for it to finish.
        stop_requested.store(true, std::memory_order_release);
        if (tracking_thread.joinable()) {
            tracking_thread.join();
        }
    }

    /**
     * @brief Sets the process ID to be tracked.
     *
     * @param pid The new process ID to track. If pid <= 0, no action is taken.
     */
    void set_pid(pid_t pid) {
        if (pid > 0) {
            current_pid.store(pid, std::memory_order_release);
            pid_valid.store(true, std::memory_order_release);
        }
    }

    /**
     * @brief Checks if the currently tracked PID is considered valid and running.
     * @return True if the PID is valid and non-zero, false otherwise.
     */
    bool is_valid() {
        return pid_valid.load(std::memory_order_acquire) &&
               current_pid.load(std::memory_order_acquire) != 0;
    }

    /**
     * @brief Gets the current process ID being tracked.
     * @return The current PID, or 0 if no PID is being tracked or it has terminated.
     */
    pid_t get_current_pid() {
        return current_pid.load(std::memory_order_acquire);
    }

    /**
     * @brief Manually invalidates the current PID.
     *
     * This stops tracking the current PID and marks it as invalid.
     */
    void invalidate() {
        current_pid.store(0, std::memory_order_release);
        pid_valid.store(false, std::memory_order_release);
    }
};
