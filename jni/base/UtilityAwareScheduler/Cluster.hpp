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

#include "UtilityAwareScheduler.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace UtilityAwareScheduler::internal {

struct EMA {
    float alpha;
    float value = 0.0f;

    explicit EMA(float a)
        : alpha(a) {
    }

    float update(float sample) {
        value = alpha * sample + (1.0f - alpha) * value;
        return value;
    }
};

struct ClusterState {
    int policy_id = 0;
    int cluster_index = 0;
    std::string policy_path;
    std::vector<int> cpus;
    std::vector<uint32_t> opp_table;
    uint32_t original_min_freq = 0;
    int floor_index = 0;

    EMA fast_ema;
    EMA slow_ema;

    int raise_counter = 0;
    int drop_counter = 0;
    bool in_burst = false;
    int burst_ticks_remaining = 0;
    int spike_cooldown_remaining = 0;

    ClusterState(float alpha_fast, float alpha_slow)
        : fast_ema(alpha_fast)
        , slow_ema(alpha_slow) {
    }
};

int update_cluster(ClusterState &state, float raw_util, const UtilityAwareScheduler::Config &cfg);

} // namespace UtilityAwareScheduler::internal
