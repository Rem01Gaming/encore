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

#include "UtilityAwareScheduler.hpp"

#include "Cluster.hpp"
#include "ProcStat.hpp"
#include "Sysfs.hpp"

#include <EncoreLog.hpp>

#include <algorithm>
#include <set>
#include <stdexcept>
#include <thread>
#include <vector>

namespace UtilityAwareScheduler {

using namespace internal;

static std::vector<ClusterState> detect_clusters(const Config &cfg) {
    std::vector<PolicyInfo> policies = enumerate_policies();

    if (policies.empty()) {
        throw std::runtime_error("no cpufreq policy directories found under /sys/devices/system/cpu/cpufreq");
    }

    LOGD_TAG("UAS", "Found {} cpufreq policies", policies.size());

    std::vector<ClusterState> clusters;

    for (int i = 0; i < static_cast<int>(policies.size()); i++) {
        const auto &policy = policies[i];
        if (!policy.min_freq) {
            LOGW_TAG("UAS", "policy{}: could not read scaling_min_freq, skipping", policy.policy_id);
            continue;
        }

        LOGD_TAG(
            "UAS",
            "policy{} (cluster{}): path={} cpus={} opp_count={} current_min={}",
            policy.policy_id,
            i,
            policy.path,
            policy.related_cpus.size(),
            policy.opp_table.size(),
            *policy.min_freq
        );

        ClusterState state(cfg.alpha_fast, cfg.alpha_slow);
        state.policy_id = policy.policy_id;
        state.cluster_index = i;
        state.policy_path = policy.path;
        state.cpus = policy.related_cpus;
        state.opp_table = policy.opp_table;
        state.original_min_freq = *policy.min_freq;

        auto it = std::find(policy.opp_table.begin(), policy.opp_table.end(), *policy.min_freq);
        if (it != policy.opp_table.end()) {
            state.floor_index = static_cast<int>(std::distance(policy.opp_table.begin(), it));
        } else {
            auto lower = std::lower_bound(policy.opp_table.begin(), policy.opp_table.end(), *policy.min_freq);
            state.floor_index = static_cast<int>(std::distance(
                policy.opp_table.begin(), (lower != policy.opp_table.end()) ? lower : std::prev(policy.opp_table.end())
            ));
            LOGW_TAG(
                "UAS",
                "policy{}: original min_freq {} not in OPP table, using nearest OPP index {} ({})",
                policy.policy_id,
                *policy.min_freq,
                state.floor_index,
                policy.opp_table[state.floor_index]
            );
        }

        LOGD_TAG(
            "UAS",
            "policy{}: initial floor_index={} ({}kHz)",
            policy.policy_id,
            state.floor_index,
            policy.opp_table[state.floor_index]
        );

        clusters.push_back(std::move(state));
    }

    if (clusters.empty()) {
        throw std::runtime_error("no usable cpufreq clusters after filtering");
    }

    return clusters;
}

static float
compute_cluster_util(const ClusterState &state, const std::vector<CpuTicks> &prev, const std::vector<CpuTicks> &curr) {
    float util_sum = 0.0f;
    int count = 0;

    for (int cpu : state.cpus) {
        if (cpu >= static_cast<int>(prev.size()) || cpu >= static_cast<int>(curr.size())) {
            LOGW_TAG(
                "UAS",
                "policy{}: cpu{} out of range in tick snapshot (prev={} curr={})",
                state.policy_id,
                cpu,
                prev.size(),
                curr.size()
            );
            continue;
        }

        uint64_t busy_delta = curr[cpu].busy - prev[cpu].busy;
        uint64_t total_delta = curr[cpu].total - prev[cpu].total;

        if (total_delta == 0) continue;

        util_sum += static_cast<float>(busy_delta) / static_cast<float>(total_delta);
        count++;
    }

    return (count > 0) ? (util_sum / static_cast<float>(count)) : 0.0f;
}

Scheduler::Scheduler(Config config)
    : config_(std::move(config)) {
}

Scheduler::~Scheduler() {
    stop();
}

void Scheduler::start() {
    if (running_.exchange(true)) {
        LOGW_TAG("UAS", "start() called but scheduler is already running");
        return;
    }
    LOGD_TAG("UAS", "Starting scheduler thread");
    thread_ = std::thread(&Scheduler::run, this);
}

void Scheduler::stop() {
    if (!running_.exchange(false)) return;
    LOGD_TAG("UAS", "Stopping scheduler thread");
    if (thread_.joinable()) thread_.join();
    LOGD_TAG("UAS", "Scheduler thread stopped");
}

void Scheduler::run() {
    LOGD_TAG("UAS", "Scheduler thread started");

    std::vector<ClusterState> clusters;

    try {
        clusters = detect_clusters(config_);
    } catch (const std::exception &e) {
        LOGE_TAG("UAS", "detect_clusters failed: {}", e.what());
        running_.store(false);
        return;
    }

    LOGD_TAG(
        "UAS", "Configured {} cluster(s), entering poll loop (interval={}ms)", clusters.size(), config_.poll_interval.count()
    );

    auto prev_ticks = read_cpu_ticks();
    if (prev_ticks.empty()) {
        LOGE_TAG("UAS", "Initial read of /proc/stat returned empty, aborting");
        running_.store(false);
        return;
    }

    while (running_.load()) {
        std::this_thread::sleep_for(config_.poll_interval);

        auto curr_ticks = read_cpu_ticks();
        if (curr_ticks.empty()) {
            LOGW_TAG("UAS", "read_cpu_ticks returned empty, skipping tick");
            continue;
        }

        for (auto &cluster : clusters) {
            float util = compute_cluster_util(cluster, prev_ticks, curr_ticks);
            int new_floor = update_cluster(cluster, util, config_);

            LOGD_TAG(
                "UAS",
                "policy{}: util={:.3f} fast_ema={:.3f} slow_ema={:.3f} floor_index={} in_burst={}",
                cluster.policy_id,
                util,
                cluster.fast_ema.value,
                cluster.slow_ema.value,
                cluster.floor_index,
                cluster.in_burst
            );

            if (new_floor >= 0) {
                uint32_t freq = cluster.opp_table[new_floor];
                LOGD_TAG(
                    "UAS",
                    "policy{}: floor change -> index={} freq={}kHz, writing to {}",
                    cluster.policy_id,
                    new_floor,
                    freq,
                    cluster.policy_path
                );

                bool ok = write_policy_min_freq(cluster.cluster_index, cluster.policy_path, freq);
                if (!ok) {
                    LOGE_TAG(
                        "UAS",
                        "policy{}: write_policy_min_freq failed for {}kHz at {}",
                        cluster.policy_id,
                        freq,
                        cluster.policy_path
                    );
                } else {
                    LOGD_TAG("UAS", "policy{}: write OK", cluster.policy_id);
                }
            }
        }

        prev_ticks = std::move(curr_ticks);
    }

    LOGD_TAG("UAS", "Poll loop exited, restoring original min freqs");

    for (const auto &cluster : clusters) {
        bool ok = write_policy_min_freq(cluster.cluster_index, cluster.policy_path, cluster.original_min_freq);
        LOGD_TAG(
            "UAS",
            "policy{}: restore original_min_freq={}kHz {}",
            cluster.policy_id,
            cluster.original_min_freq,
            ok ? "OK" : "FAILED"
        );
    }
}

} // namespace UtilityAwareScheduler
