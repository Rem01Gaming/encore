/*
 * Copyright (C) 2026 Rem01Gaming
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

#include <cstdio>
#include <fstream>
#include <regex>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include "DeviceMitigationStore.hpp"
#include <DeviceInfo.hpp>
#include <EncoreUtility.hpp>

bool DeviceMitigationStore::load_config(const std::string &config_path) {
    std::lock_guard<std::mutex> lock(mutex_);

    FILE *fp = fopen(config_path.c_str(), "rb");
    if (!fp) {
        LOGE_TAG("DeviceMitigationStore", "Device mitigation file not found: {}", config_path);
        return false;
    }

    char readBuffer[65536];
    rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));

    rapidjson::Document doc;
    doc.ParseStream(is);
    fclose(fp);

    if (doc.HasParseError()) {
        LOGE_TAG(
            "DeviceMitigationStore",
            "Parse error: {} (Offset: {})",
            rapidjson::GetParseError_En(doc.GetParseError()),
            doc.GetErrorOffset()
        );
        return false;
    }

    if (!doc.IsObject()) {
        LOGE_TAG("DeviceMitigationStore", "Root is not an object");
        return false;
    }

    return parse_config(doc);
}

std::unordered_set<std::string> DeviceMitigationStore::get_mitigation_items(bool use_device_mitigation) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::unordered_set<std::string> items;

    // Always apply device-specific rules if they match
    LOGT_TAG("DeviceMitigationStore", "Checking device-specific rules...");
    for (const auto &[rule_name, rule] : data_.device_rules) {
        LOGT_TAG("DeviceMitigationStore", "Evaluating rule: {}", rule_name);

        if (matches_rule(rule)) {
            LOGI_TAG("DeviceMitigationStore", "Applying device rule: {}", rule_name);
            items.insert(rule.items.begin(), rule.items.end());
        } else {
            LOGT_TAG("DeviceMitigationStore", "Rule '{}' did not match", rule_name);
        }
    }

    // Only add default items if use_device_mitigation is true
    if (use_device_mitigation) {
        LOGD_TAG("DeviceMitigationStore", "Adding default items");
        items.insert(data_.default_items.begin(), data_.default_items.end());
    }

    LOGD_TAG("DeviceMitigationStore", "Total mitigation items: {}", items.size());
    return items;
}

std::unordered_set<std::string> DeviceMitigationStore::get_cached_mitigation_items(bool use_device_mitigation) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::unordered_set<std::string> items;

    items.insert(data_.cached_mitigation_items.begin(), data_.cached_mitigation_items.end());

    if (use_device_mitigation) {
        LOGD_TAG("DeviceMitigationStore", "Adding default items");
        items.insert(data_.default_items.begin(), data_.default_items.end());
    }

    LOGD_TAG("DeviceMitigationStore", "Total cached mitigation items: {}", items.size());
    return items;
}

bool DeviceMitigationStore::matches_rule(const DeviceRule &rule) const {
    auto device_info = get_device_info();

    LOGT_TAG("DeviceMitigationStore", "Matching rule: {}", rule.name);
    LOGT_TAG("DeviceMitigationStore", "  Filter type: {}", rule.filter_type);
    LOGT_TAG(
        "DeviceMitigationStore",
        "  Device info - SOC: '{}', Model: '{}', Uname: '{}'",
        device_info.at("soc"),
        device_info.at("model"),
        device_info.at("uname")
    );

    bool all_match = (rule.filter_type == "all");
    bool any_match = (rule.filter_type == "any");

    if (!all_match && !any_match) {
        LOGE_TAG("DeviceMitigationStore", "Unknown filter_type: {}", rule.filter_type);
        return false;
    }

    int matched_conditions = 0;
    int total_conditions = 0;

    LOGT_TAG("DeviceMitigationStore", "  Filter conditions:");
    for (const auto &[condition_name, condition] : rule.filter_condition) {
        total_conditions++;

        LOGT_TAG("DeviceMitigationStore", "    Condition {}:", condition_name);
        LOGT_TAG("DeviceMitigationStore", "      Operator: '{}', Expected value: '{}'", condition.op, condition.value);

        auto it = device_info.find(condition_name);
        if (it != device_info.end()) {
            LOGT_TAG("DeviceMitigationStore", "      Device value: '{}'", it->second);

            if (check_condition(condition, it->second)) {
                matched_conditions++;
                LOGD_TAG("DeviceMitigationStore", "      -> MATCHED");
                if (any_match) {
                    LOGT_TAG("DeviceMitigationStore", "      'any' condition satisfied - rule matches");
                    return true;
                }
            } else {
                LOGT_TAG("DeviceMitigationStore", "      -> NOT MATCHED");
                if (all_match) {
                    LOGT_TAG("DeviceMitigationStore", "      'all' condition failed - rule does not match");
                    return false;
                }
            }
        } else {
            LOGW_TAG("DeviceMitigationStore", "    Condition {} not found in device info", condition_name);
            if (all_match) {
                LOGD_TAG("DeviceMitigationStore", "      'all' condition missing - rule does not match");
                return false;
            }
        }
    }

    bool result = false;
    if (all_match) {
        result = (matched_conditions == total_conditions);
        LOGT_TAG(
            "DeviceMitigationStore",
            "  'all' rule: matched {}/{} conditions, result: {}",
            matched_conditions,
            total_conditions,
            result
        );
    } else { // any_match
        result = (matched_conditions > 0);
        LOGT_TAG(
            "DeviceMitigationStore",
            "  'any' rule: matched {}/{} conditions, result: {}",
            matched_conditions,
            total_conditions,
            result
        );
    }

    return result;
}

std::unordered_map<std::string, std::string> DeviceMitigationStore::get_device_info() const {
    std::unordered_map<std::string, std::string> info;

    info["soc"] = DeviceInfo::get_soc_model();
    info["model"] = DeviceInfo::get_device_model();
    info["uname"] = DeviceInfo::get_kernel_uname();

    LOGT_TAG("DeviceMitigationStore", "Current device info:");
    LOGT_TAG("DeviceMitigationStore", "  SOC: '{}'", info["soc"]);
    LOGT_TAG("DeviceMitigationStore", "  Model: '{}'", info["model"]);
    LOGT_TAG("DeviceMitigationStore", "  Uname: '{}'", info["uname"]);

    return info;
}

bool DeviceMitigationStore::parse_config(const rapidjson::Document &doc) {
    DeviceMitigationData new_data;

    // Parse default items
    if (doc.HasMember("default") && doc["default"].IsObject()) {
        const rapidjson::Value &default_obj = doc["default"];
        if (default_obj.HasMember("items") && default_obj["items"].IsArray()) {
            for (const auto &item : default_obj["items"].GetArray()) {
                if (item.IsString()) {
                    new_data.default_items.insert(item.GetString());
                }
            }
        }
        LOGD_TAG("DeviceMitigationStore", "Loaded {} default items", new_data.default_items.size());
    }

    // Parse device rules
    if (doc.HasMember("device_rules") && doc["device_rules"].IsObject()) {
        const rapidjson::Value &rules_obj = doc["device_rules"];
        LOGD_TAG("DeviceMitigationStore", "Found {} device rules", rules_obj.MemberCount());

        for (auto it = rules_obj.MemberBegin(); it != rules_obj.MemberEnd(); ++it) {
            if (it->name.IsString() && it->value.IsObject()) {
                DeviceRule rule;
                const rapidjson::Value &rule_obj = it->value;

                // Parse rule name and description
                if (rule_obj.HasMember("name") && rule_obj["name"].IsString()) {
                    rule.name = rule_obj["name"].GetString();
                }
                if (rule_obj.HasMember("description") && rule_obj["description"].IsString()) {
                    rule.description = rule_obj["description"].GetString();
                }
                if (rule_obj.HasMember("filter_type") && rule_obj["filter_type"].IsString()) {
                    rule.filter_type = rule_obj["filter_type"].GetString();
                }

                // Parse items
                if (rule_obj.HasMember("items") && rule_obj["items"].IsArray()) {
                    for (const auto &item : rule_obj["items"].GetArray()) {
                        if (item.IsString()) {
                            rule.items.insert(item.GetString());
                        }
                    }
                }

                // Parse filter conditions
                if (rule_obj.HasMember("filter_condition") && rule_obj["filter_condition"].IsObject()) {
                    const rapidjson::Value &cond_obj = rule_obj["filter_condition"];
                    LOGD_TAG("DeviceMitigationStore", "  Rule '{}': {} filter conditions", rule.name, cond_obj.MemberCount());

                    for (auto cond_it = cond_obj.MemberBegin(); cond_it != cond_obj.MemberEnd(); ++cond_it) {
                        if (cond_it->name.IsString() && cond_it->value.IsObject()) {
                            Condition condition;
                            const rapidjson::Value &cond_val = cond_it->value;

                            if (cond_val.HasMember("operator") && cond_val["operator"].IsString()) {
                                condition.op = cond_val["operator"].GetString();
                            }
                            if (cond_val.HasMember("value") && cond_val["value"].IsString()) {
                                condition.value = cond_val["value"].GetString();
                            }

                            rule.filter_condition[cond_it->name.GetString()] = condition;

                            LOGD_TAG(
                                "DeviceMitigationStore",
                                "    {}: {} '{}'",
                                cond_it->name.GetString(),
                                condition.op,
                                condition.value
                            );
                        }
                    }
                }

                new_data.device_rules[it->name.GetString()] = rule;
                LOGD_TAG("DeviceMitigationStore", "  Added rule: {} with {} items", rule.name, rule.items.size());
            }
        }
    }

    std::unordered_set<std::string> cached_items;

    for (const auto &[rule_name, rule] : new_data.device_rules) {
        LOGT_TAG("DeviceMitigationStore", "Evaluating rule: {}", rule_name);

        if (matches_rule(rule)) {
            LOGI_TAG("DeviceMitigationStore", "Applying device rule: {}", rule_name);
            cached_items.insert(rule.items.begin(), rule.items.end());
        } else {
            LOGT_TAG("DeviceMitigationStore", "Rule '{}' did not match", rule_name);
        }
    }

    new_data.cached_mitigation_items = std::move(cached_items);
    data_ = new_data;
    return true;
}

bool DeviceMitigationStore::check_condition(const Condition &condition, const std::string &value) const {
    LOGT_TAG(
        "DeviceMitigationStore",
        "    Checking condition: operator='{}', expected='{}', actual='{}'",
        condition.op,
        condition.value,
        value
    );

    bool result = false;

    if (condition.op == "match") {
        result = (value == condition.value);
        LOGT_TAG("DeviceMitigationStore", "      Exact match: {}", result);
    } else if (condition.op == "contains") {
        result = value.contains(condition.value);
        LOGT_TAG("DeviceMitigationStore", "      Contains check: '{}' in '{}' = {}", condition.value, value, result);
    } else if (condition.op == "regex") {
        try {
            std::regex pattern(condition.value);
            result = std::regex_search(value, pattern);
            LOGT_TAG("DeviceMitigationStore", "      Regex match: {}", result);
        } catch (const std::regex_error &e) {
            LOGE_TAG("DeviceMitigationStore", "Invalid regex pattern: {} - {}", condition.value, e.what());
            return false;
        }
    } else {
        LOGW_TAG("DeviceMitigationStore", "Unknown operator: {}", condition.op);
        return false;
    }

    return result;
}
