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

#include "Cluster.hpp"

#include <algorithm>
#include <cmath>

namespace UtilityAwareScheduler::internal {

int update_cluster(ClusterState &state, float raw_util, const UtilityAwareScheduler::Config &cfg) {
    float fast = state.fast_ema.update(raw_util);
    float slow = state.slow_ema.update(raw_util);

    float trend = fast - slow;
    float deviation = raw_util - fast;

    int old_floor = state.floor_index;
    int max_index = static_cast<int>(state.opp_table.size()) - 1;
    int normal_max_index = static_cast<int>(std::round(cfg.normal_max_boost_percent * max_index));

    if (!state.in_burst && state.spike_cooldown_remaining == 0) {
        if (deviation >= cfg.spike_threshold) {
            state.in_burst = true;
            state.burst_ticks_remaining = cfg.burst_hold_ticks;
            state.raise_counter = 0;
            state.drop_counter = 0;

            int new_floor = std::min(state.floor_index + cfg.burst_floor_steps, max_index);
            state.floor_index = new_floor;

            return (new_floor != old_floor) ? new_floor : -1;
        }
    }

    if (state.in_burst) {
        state.burst_ticks_remaining--;
        if (state.burst_ticks_remaining <= 0) {
            state.in_burst = false;
            state.spike_cooldown_remaining = cfg.spike_cooldown_ticks;
        }
        return -1;
    }

    if (state.spike_cooldown_remaining > 0) {
        state.spike_cooldown_remaining--;
    }

    if (fast > cfg.raise_threshold && trend > 0.0f) {
        state.drop_counter = 0;
        state.raise_counter++;

        if (state.raise_counter >= cfg.raise_confirm_ticks) {
            state.raise_counter = 0;
            int next_floor = state.floor_index + 1;
            if (next_floor <= normal_max_index) {
                state.floor_index = next_floor;
                return next_floor;
            }
        }
    } else if (fast < cfg.drop_threshold && trend < 0.0f) {
        state.raise_counter = 0;
        state.drop_counter++;

        if (state.drop_counter >= cfg.drop_confirm_ticks) {
            state.drop_counter = 0;
            int new_floor = std::max(state.floor_index - 1, 0);
            if (new_floor != state.floor_index) {
                state.floor_index = new_floor;
                return new_floor;
            }
        }
    } else {
        state.raise_counter = 0;
        state.drop_counter = 0;
    }

    return -1;
}

} // namespace UtilityAwareScheduler::internal
