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

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace UtilityAwareScheduler::internal {

std::optional<std::string> sysfs_read(const std::string &path);
bool sysfs_write(const std::string &path, const std::string &value);

struct PolicyInfo {
    int policy_id;
    std::string path;
    std::vector<int> related_cpus;
    std::vector<uint32_t> opp_table;
    std::optional<uint32_t> min_freq;
};

std::vector<PolicyInfo> enumerate_policies();
bool write_policy_min_freq(int policy_id, const std::string &policy_path, uint32_t freq);

} // namespace UtilityAwareScheduler::internal
