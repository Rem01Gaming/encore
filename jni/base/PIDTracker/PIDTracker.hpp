/*
 * Copyright (C) 2024-2026 Rem01Gaming
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <sys/types.h>
#include <thread>

/**
 * @class PIDTracker
 * @brief Tracks a PID and executes a callback upon process termination.
 */
class PIDTracker {
public:
    using ExitCallback = std::function<void(pid_t)>;

    /**
     * @brief Constructs the tracker.
     * @param callback The function to call when the tracked PID exits.
     */
    explicit PIDTracker(ExitCallback callback = nullptr);

    /**
     * @brief Cleans up the background thread.
     */
    ~PIDTracker();

    /**
     * @brief Sets the process ID to be tracked.
     * @param pid The process ID to track.
     */
    void set_pid(pid_t pid);

    /**
     * @brief Sets or updates the exit callback.
     */
    void set_callback(ExitCallback callback);

    /**
     * @brief Stops tracking the current PID immediately.
     */
    void invalidate();

    /**
     * @return The PID currently being monitored, or 0 if idle.
     */
    pid_t get_current_pid() const;

private:
    static constexpr auto LOOP_INTERVAL_BUSY = std::chrono::milliseconds(150);

    void tracking_loop();
    bool tracking_loop_pidfd(pid_t pid);

    bool is_pid_running(pid_t pid);
    void wakeup_poll();

    std::atomic<pid_t> current_pid{0};
    std::atomic<bool> stop_requested{false};

    ExitCallback on_exit_callback;
    mutable std::mutex callback_mutex;

    std::mutex tracker_mutex;
    std::condition_variable tracker_cv;
    int wakeup_fd{-1};

    std::thread tracking_thread;
};
