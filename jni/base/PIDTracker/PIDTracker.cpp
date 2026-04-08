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

#include "PIDTracker.hpp"

#include <EncoreLog.hpp>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <sys/eventfd.h>
#include <sys/syscall.h>
#include <unistd.h>

PIDTracker::PIDTracker(ExitCallback callback)
    : on_exit_callback(std::move(callback)) {
    wakeup_fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    tracking_thread = std::thread(&PIDTracker::tracking_loop, this);
}

PIDTracker::~PIDTracker() {
    stop_requested.store(true, std::memory_order_release);
    wakeup_poll();
    tracker_cv.notify_all();
    if (tracking_thread.joinable()) tracking_thread.join();
    if (wakeup_fd >= 0) close(wakeup_fd);
}

void PIDTracker::set_pid(pid_t pid) {
    if (pid > 0) {
        current_pid.store(pid, std::memory_order_release);
        wakeup_poll();
        tracker_cv.notify_one();
    }
}

void PIDTracker::set_callback(ExitCallback callback) {
    std::lock_guard<std::mutex> lock(callback_mutex);
    on_exit_callback = std::move(callback);
}

void PIDTracker::invalidate() {
    current_pid.store(0, std::memory_order_release);
    wakeup_poll();
    tracker_cv.notify_one();
}

void PIDTracker::wakeup_poll() {
    if (wakeup_fd >= 0) {
        uint64_t val = 1;
        write(wakeup_fd, &val, sizeof(val));
    }
}

bool PIDTracker::is_pid_running(pid_t pid) {
    return kill(pid, 0) == 0;
}

pid_t PIDTracker::get_current_pid() const {
    return current_pid.load(std::memory_order_acquire);
}

void PIDTracker::tracking_loop() {
    pthread_setname_np(pthread_self(), "PIDTracker");

    while (!stop_requested.load(std::memory_order_acquire)) {
        pid_t pid = current_pid.load(std::memory_order_acquire);

        // Wait for a PID or Shutdown
        if (pid == 0) {
            std::unique_lock<std::mutex> lock(tracker_mutex);
            tracker_cv.wait(lock, [this] {
                return stop_requested.load(std::memory_order_acquire) || current_pid.load(std::memory_order_acquire) > 0;
            });
            continue;
        }

        // Try tracking using pidfd_open if supported (>= Linux 5.3)
        int pidfd = syscall(__NR_pidfd_open, pid, 0);
        if (pidfd >= 0) {
            bool exited = tracking_loop_pidfd(pidfd);
            close(pidfd);

            if (exited) {
                if (current_pid.compare_exchange_strong(pid, 0, std::memory_order_acq_rel)) {
                    LOGD_TAG("PIDTracker", "PID {} terminated (pidfd)", pid);
                    std::lock_guard<std::mutex> lock(callback_mutex);
                    if (on_exit_callback) on_exit_callback(pid);
                }
            }
            continue;
        }

        // pidfd_open failed not supported, fallback to poll
        if (is_pid_running(pid)) {
            std::unique_lock<std::mutex> lock(tracker_mutex);
            tracker_cv.wait_for(lock, LOOP_INTERVAL_BUSY, [this, pid] {
                return stop_requested.load(std::memory_order_acquire) || current_pid.load(std::memory_order_acquire) != pid;
            });
        } else {
            if (current_pid.compare_exchange_strong(pid, 0, std::memory_order_acq_rel)) {
                LOGD_TAG("PIDTracker", "PID {} terminated (fallback)", pid);
                std::lock_guard<std::mutex> lock(callback_mutex);
                if (on_exit_callback) on_exit_callback(pid);
            }
        }
    }
}

bool PIDTracker::tracking_loop_pidfd(int pidfd) {
    struct pollfd pfds[2] = {{pidfd, POLLIN, 0}, {wakeup_fd, POLLIN, 0}};

    // Block until the process dies or interrupted by set_pid/invalidate/stop
    int ret = poll(pfds, 2, -1);

    if (ret > 0) {
        // Interrupted
        if (pfds[1].revents & POLLIN) {
            uint64_t val;
            read(wakeup_fd, &val, sizeof(val));
            return false;
        }

        // Process exited
        if (pfds[0].revents & POLLIN) {
            return true;
        }
    }

    return false;
}
