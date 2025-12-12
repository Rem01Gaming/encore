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

#include <fstream>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "Encore.hpp"
#include "EncoreLog.hpp"

class EncoreConfigStore {
public:
    struct Preferences {
        bool enforce_lite_mode = false;
        int log_level = 3;
    };

    struct DeviceMitigation {
        bool enable = false;
        std::unordered_set<std::string> items;
    };

    struct CPUGovernor {
        std::string performance;
        std::string balance;
        std::string powersave;
    };

    struct ConfigData {
        Preferences preferences;
        DeviceMitigation device_mitigation;
        CPUGovernor cpu_governor;
    };

    /**
     * @brief Get singleton instance
     */
    static EncoreConfigStore &get_instance() {
        static EncoreConfigStore instance;
        return instance;
    }

    /**
     * @brief Load configuration from file
     * @param config_path Path to config file (defaults to CONFIG_FILE)
     * @return true if loaded successfully, false otherwise
     */
    bool load_config(const std::string &config_path = CONFIG_FILE);

    /**
     * @brief Save current configuration to file
     * @param config_path Optional custom path, uses loaded path by default
     * @return true if saved successfully, false otherwise
     */
    bool save_config(const std::string &config_path = CONFIG_FILE);

    /**
     * @brief Get current configuration
     */
    ConfigData get_config() const;

    /**
     * @brief Get preferences
     */
    Preferences get_preferences() const;

    /**
     * @brief Get device mitigation settings
     */
    DeviceMitigation get_device_mitigation() const;

    /**
     * @brief Get CPU governor settings
     */
    CPUGovernor get_cpu_governor() const;

    /**
     * @brief Check if a specific mitigation item is enabled
     */
    bool is_mitigation_enabled(const std::string &item) const;

    /**
     * @brief Update preferences
     */
    void set_preferences(const Preferences &prefs);

    /**
     * @brief Update device mitigation
     */
    void set_device_mitigation(const DeviceMitigation &mitigation);

    /**
     * @brief Update CPU governor settings
     */
    void set_cpu_governor(const CPUGovernor &governor);

    /**
     * @brief Reload configuration from disk
     */
    std::string get_config_path() const;

    /**
     * @brief Reload configuration from disk
     */
    bool reload();

private:
    EncoreConfigStore() = default;
    ~EncoreConfigStore() = default;

    EncoreConfigStore(const EncoreConfigStore &) = delete;
    EncoreConfigStore &operator=(const EncoreConfigStore &) = delete;

    /**
     * @brief Read default CPU governor from file
     */
    std::string read_default_cpu_governor() const;

    /**
     * @brief Create default configuration file
     */
    bool create_default_config();

    /**
     * @brief Parse JSON document into config structure
     */
    bool parse_config(const rapidjson::Document &doc);

    mutable std::mutex mutex_;
    ConfigData config_;
    std::string config_path_ = CONFIG_FILE;
};

#define CONFIG_STORE EncoreConfigStore::get_instance()
