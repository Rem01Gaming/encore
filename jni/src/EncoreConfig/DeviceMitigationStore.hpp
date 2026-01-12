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

#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <rapidjson/document.h>

#include "Encore.hpp"
#include "EncoreLog.hpp"

class DeviceMitigationStore {
public:
    struct Condition {
        std::string op; // "regex", "contains", "match"
        std::string value;
    };

    struct DeviceRule {
        std::string name;
        std::string description;
        std::string filter_type; // "all" or "any"
        std::unordered_set<std::string> items;
        std::unordered_map<std::string, Condition> filter_condition;
    };

    struct DeviceMitigationData {
        std::unordered_set<std::string> default_items;
        std::unordered_map<std::string, DeviceRule> device_rules;
        std::unordered_set<std::string> cached_mitigation_items;
    };

    /**
     * @brief Get singleton instance
     */
    static DeviceMitigationStore &get_instance() {
        static DeviceMitigationStore instance;
        return instance;
    }

    /**
     * @brief Load device mitigation rules from file
     * @param config_path Path to device mitigation config file
     * @return true if loaded successfully, false otherwise
     */
    bool load_config(const std::string &config_path = DEVICE_MITIGATION_FILE);

    /**
     * @brief Get all device mitigation items that should be applied
     * @param use_device_mitigation Whether to include default items
     * @return Set of mitigation items to apply
     */
    std::unordered_set<std::string> get_mitigation_items(bool use_device_mitigation) const;

    /**
     * @brief Get cached mitigation items
     * @param use_device_mitigation Whether to include default items
     * @return Set of cached mitigation items to apply
     */
    std::unordered_set<std::string> get_cached_mitigation_items(bool use_device_mitigation) const;

    /**
     * @brief Check if a device rule matches current device
     * @param rule The device rule to check
     * @return true if rule matches, false otherwise
     */
    bool matches_rule(const DeviceRule &rule) const;

    /**
     * @brief Get device information for matching
     */
    std::unordered_map<std::string, std::string> get_device_info() const;

private:
    DeviceMitigationStore() = default;
    ~DeviceMitigationStore() = default;

    DeviceMitigationStore(const DeviceMitigationStore &) = delete;
    DeviceMitigationStore &operator=(const DeviceMitigationStore &) = delete;

    /**
     * @brief Parse JSON document into device mitigation structure
     */
    bool parse_config(const rapidjson::Document &doc);

    /**
     * @brief Check if a condition matches a value
     */
    bool check_condition(const Condition &condition, const std::string &value) const;

    mutable std::mutex mutex_;
    DeviceMitigationData data_;
};

#define device_mitigation_store DeviceMitigationStore::get_instance()
