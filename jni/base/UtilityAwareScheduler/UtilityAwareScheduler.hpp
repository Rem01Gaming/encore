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

#include <atomic>
#include <chrono>
#include <thread>

namespace UtilityAwareScheduler {

struct Config {
    float alpha_fast = 0.3f;
    float alpha_slow = 0.05f;
    float raise_threshold = 0.65f;
    float drop_threshold = 0.30f;
    float spike_threshold = 0.35f;
    int raise_confirm_ticks = 3;
    int drop_confirm_ticks = 12;
    int burst_hold_ticks = 20;
    int burst_floor_steps = 2;
    int spike_cooldown_ticks = 20;
    float normal_max_boost_percent = 0.62f;
    std::chrono::milliseconds poll_interval{250};
};

class Scheduler {
public:
    explicit Scheduler(Config config = {});
    ~Scheduler();

    void start();
    void stop();

    Scheduler(const Scheduler &) = delete;
    Scheduler &operator=(const Scheduler &) = delete;

private:
    Config config_;
    std::thread thread_;
    std::atomic<bool> running_{false};

    void run();
};

} // namespace UtilityAwareScheduler
