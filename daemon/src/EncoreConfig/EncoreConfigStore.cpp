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

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <memory>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include "EncoreConfigStore.hpp"

bool EncoreConfigStore::load_config(const std::string &config_path) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        config_path_ = config_path;
    }

    FILE *fp = fopen(config_path.c_str(), "rb");
    if (!fp) {
        LOGW_TAG("EncoreConfigStore", "Config file not found, creating default: {}", config_path);
        return create_default_config();
    }

    char readBuffer[65536];
    rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));

    rapidjson::Document doc;
    doc.ParseStream(is);
    fclose(fp);

    if (doc.HasParseError()) {
        LOGE_TAG(
            "EncoreConfigStore", "Parse error: {} (Offset: {})",
            rapidjson::GetParseError_En(doc.GetParseError()), doc.GetErrorOffset());
        return create_default_config();
    }

    if (!doc.IsObject()) {
        LOGE_TAG("EncoreConfigStore", "Root is not an object");
        return false;
    }

    return parse_config(doc);
}

bool EncoreConfigStore::save_config(const std::string &config_path) {
    std::lock_guard<std::mutex> lock(mutex_);

    rapidjson::Document doc;
    doc.SetObject();
    rapidjson::Document::AllocatorType &allocator = doc.GetAllocator();

    // Serialize preferences
    rapidjson::Value prefs_obj(rapidjson::kObjectType);
    prefs_obj.AddMember("enforce_lite_mode", config_.preferences.enforce_lite_mode, allocator);
    prefs_obj.AddMember("log_level", config_.preferences.log_level, allocator);
    doc.AddMember("preferences", prefs_obj, allocator);

    // Serialize device mitigation
    rapidjson::Value device_mit_obj(rapidjson::kObjectType);
    device_mit_obj.AddMember("enable", config_.device_mitigation.enable, allocator);

    rapidjson::Value items_array(rapidjson::kArrayType);
    for (const auto &item : config_.device_mitigation.items) {
        items_array.PushBack(rapidjson::Value(item.c_str(), allocator).Move(), allocator);
    }
    device_mit_obj.AddMember("items", items_array, allocator);
    doc.AddMember("device_mitigation", device_mit_obj, allocator);

    // Serialize CPU governor
    rapidjson::Value cpu_gov_obj(rapidjson::kObjectType);
    cpu_gov_obj.AddMember(
        "balance", rapidjson::Value(config_.cpu_governor.balance.c_str(), allocator).Move(),
        allocator);
    cpu_gov_obj.AddMember(
        "powersave", rapidjson::Value(config_.cpu_governor.powersave.c_str(), allocator).Move(),
        allocator);
    doc.AddMember("cpu_governor", cpu_gov_obj, allocator);

    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    writer.SetIndent(' ', 2);
    doc.Accept(writer);

    FILE *fp = fopen(config_path.c_str(), "wb");
    if (!fp) {
        LOGE_TAG("EncoreConfigStore", "Failed to open config file for writing: {}", config_path);
        return false;
    }

    fwrite(buffer.GetString(), 1, buffer.GetSize(), fp);
    fclose(fp);

    LOGI_TAG("EncoreConfigStore", "Configuration saved to {}", config_path);
    return true;
}

EncoreConfigStore::ConfigData EncoreConfigStore::get_config() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return config_;
}

EncoreConfigStore::Preferences EncoreConfigStore::get_preferences() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return config_.preferences;
}

EncoreConfigStore::DeviceMitigation EncoreConfigStore::get_device_mitigation() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return config_.device_mitigation;
}

EncoreConfigStore::CPUGovernor EncoreConfigStore::get_cpu_governor() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return config_.cpu_governor;
}

bool EncoreConfigStore::is_mitigation_enabled(const std::string &item) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!config_.device_mitigation.enable) return false;
    return config_.device_mitigation.items.find(item) != config_.device_mitigation.items.end();
}

void EncoreConfigStore::set_preferences(const Preferences &prefs) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_.preferences = prefs;
}

void EncoreConfigStore::set_device_mitigation(const DeviceMitigation &mitigation) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_.device_mitigation = mitigation;
}

void EncoreConfigStore::set_cpu_governor(const CPUGovernor &governor) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_.cpu_governor = governor;
}

std::string EncoreConfigStore::get_config_path() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return config_path_;
}

bool EncoreConfigStore::reload() {
    return load_config(config_path_);
}

std::string EncoreConfigStore::read_default_cpu_governor() const {
    // Fallback governor
    std::string default_governor = "schedutil";

    std::ifstream file(DEFAULT_CPU_GOV);
    if (!file.is_open()) {
        LOGW_TAG(
            "EncoreConfigStore", "Default CPU governor file not found, using fallback: {}",
            default_governor);
    }

    if (std::getline(file, default_governor)) {
        LOGD_TAG("EncoreConfigStore", "Read default CPU governor from file: {}", default_governor);
    } else {
        LOGW_TAG(
            "EncoreConfigStore", "Default CPU governor file is empty, using fallback: {}",
            default_governor);
    }

    file.close();
    return default_governor;
}

bool EncoreConfigStore::create_default_config() {
    std::string default_governor = read_default_cpu_governor();

    // clang-format off
    ConfigData default_config = ConfigData{
        .preferences = {
            .enforce_lite_mode = false,
            .log_level = 3
        },
        .device_mitigation = {
            .enable = false,
            .items = {}
        },
        .cpu_governor = {
            .balance = default_governor,
            .powersave = default_governor
        }
    };
    // clang-format on

    {
        std::lock_guard<std::mutex> lock(mutex_);
        config_ = default_config;
    }

    return save_config(config_path_);
}

bool EncoreConfigStore::parse_config(const rapidjson::Document &doc) {
    ConfigData new_config;

    // Parse preferences
    if (doc.HasMember("preferences") && doc["preferences"].IsObject()) {
        const rapidjson::Value &prefs = doc["preferences"];

        if (prefs.HasMember("enforce_lite_mode") && prefs["enforce_lite_mode"].IsBool()) {
            new_config.preferences.enforce_lite_mode = prefs["enforce_lite_mode"].GetBool();
        }

        if (prefs.HasMember("log_level") && prefs["log_level"].IsInt()) {
            new_config.preferences.log_level = prefs["log_level"].GetInt();
        }
    }

    // Parse device mitigation
    if (doc.HasMember("device_mitigation") && doc["device_mitigation"].IsObject()) {
        const rapidjson::Value &mit = doc["device_mitigation"];

        if (mit.HasMember("enable") && mit["enable"].IsBool()) {
            new_config.device_mitigation.enable = mit["enable"].GetBool();
        }

        if (mit.HasMember("items") && mit["items"].IsArray()) {
            for (const auto &item : mit["items"].GetArray()) {
                if (item.IsString()) {
                    new_config.device_mitigation.items.insert(item.GetString());
                }
            }
        }
    }

    // Parse CPU governor
    if (doc.HasMember("cpu_governor") && doc["cpu_governor"].IsObject()) {
        const rapidjson::Value &gov = doc["cpu_governor"];

        if (gov.HasMember("balance") && gov["balance"].IsString()) {
            new_config.cpu_governor.balance = gov["balance"].GetString();
        }

        if (gov.HasMember("powersave") && gov["powersave"].IsString()) {
            new_config.cpu_governor.powersave = gov["powersave"].GetString();
        }
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        config_ = new_config;
    }

    LOGI_TAG("EncoreConfigStore", "Configuration loaded from {}", config_path_);
    return true;
}
