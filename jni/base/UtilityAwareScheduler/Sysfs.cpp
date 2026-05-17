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

#include "Sysfs.hpp"

#include <algorithm>
#include <cctype>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <unistd.h>

namespace UtilityAwareScheduler::internal {

std::optional<std::string> sysfs_read(const std::string &path) {
    std::ifstream f(path);
    if (!f.is_open()) return std::nullopt;
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    return content;
}

bool sysfs_write(const std::string &path, const std::string &value) {
    int fd = ::open(path.c_str(), O_WRONLY);
    if (fd < 0) return false;
    ssize_t written = ::write(fd, value.c_str(), value.size());
    ::close(fd);
    return written == static_cast<ssize_t>(value.size());
}

static std::vector<int> parse_cpu_list(const std::string &content) {
    std::vector<int> cpus;
    std::istringstream ss(content);
    int id;
    while (ss >> id) {
        cpus.push_back(id);
    }
    return cpus;
}

static std::vector<uint32_t> parse_opp_table(const std::string &content) {
    std::vector<uint32_t> freqs;
    std::istringstream ss(content);
    uint32_t f;
    while (ss >> f) {
        freqs.push_back(f);
    }
    std::sort(freqs.begin(), freqs.end());
    return freqs;
}

static std::optional<int> policy_id_from_name(const std::string &name) {
    if (name.substr(0, 6) != "policy") return std::nullopt;
    const std::string digits = name.substr(6);
    if (digits.empty()) return std::nullopt;
    for (char c : digits) {
        if (!std::isdigit(static_cast<unsigned char>(c))) return std::nullopt;
    }
    return std::stoi(digits);
}

std::vector<PolicyInfo> enumerate_policies() {
    const std::string base = "/sys/devices/system/cpu/cpufreq";
    std::vector<PolicyInfo> policies;

    std::error_code ec;
    if (!std::filesystem::exists(base, ec) || ec) return policies;

    for (const auto &entry : std::filesystem::directory_iterator(base, ec)) {
        if (ec) break;

        const std::string name = entry.path().filename().string();
        auto id = policy_id_from_name(name);
        if (!id) continue;

        const std::string policy_path = base + "/" + name;

        auto related_content = sysfs_read(policy_path + "/related_cpus");
        if (!related_content) continue;

        auto opp_content = sysfs_read(policy_path + "/scaling_available_frequencies");
        if (!opp_content) continue;

        auto min_content = sysfs_read(policy_path + "/scaling_min_freq");

        PolicyInfo info;
        info.policy_id = *id;
        info.path = policy_path;
        info.related_cpus = parse_cpu_list(*related_content);
        info.opp_table = parse_opp_table(*opp_content);

        if (min_content) {
            try {
                info.min_freq = static_cast<uint32_t>(std::stoul(*min_content));
            } catch (...) {}
        }

        if (info.related_cpus.empty() || info.opp_table.empty()) continue;

        policies.push_back(std::move(info));
    }

    std::sort(policies.begin(), policies.end(), [](const PolicyInfo &a, const PolicyInfo &b) {
        return a.policy_id < b.policy_id;
    });

    return policies;
}

bool write_policy_min_freq(int policy_id, const std::string &policy_path, uint32_t freq) {
    if (access("/proc/ppm", F_OK) == 0) {
        return sysfs_write(
            "/proc/ppm/policy/hard_userlimit_min_cpu_freq", std::to_string(policy_id) + " " + std::to_string(freq)
        );
    }

    return sysfs_write(policy_path + "/scaling_min_freq", std::to_string(freq));
}

} // namespace UtilityAwareScheduler::internal
