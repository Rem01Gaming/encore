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

#include <filesystem>
#include <fstream>
#include <map>
#include <stdexcept>
#include <vector>

struct ModuleProperties {
    std::string key;
    std::string value;
};

namespace ModuleProperty {

/**
 * @brief Reads all properties from module.prop file
 *
 * @param path The path to the module.prop file
 * @param result Vector of ModuleProperties, which will be filled with the key-value pairs from module.prop
 * @throws std::runtime_error If R/W operation failed
 */
inline void Get(const std::string &path, std::vector<ModuleProperties> &result) {
    result.clear();

    std::ifstream in(path);
    if (!in.is_open()) {
        throw std::system_error(
            errno, std::generic_category(), "Failed to open module.prop file: " + path);
    }

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;

        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            result.push_back({key, value});
        }
    }

    in.close();
}

/**
 * @brief Changes module.prop based on the provided data
 *
 * @param path The path to the module.prop file
 * @param data A vector of ModuleProperties containing the key-value pairs to be changed in module.prop
 * @throws std::runtime_error If R/W operation failed
 * @note This function will only change the values of the keys specified in the data vector, leaving others intact
 */
inline void Change(const std::string &path, const std::vector<ModuleProperties> &data) {
    namespace fs = std::filesystem;

    std::map<std::string, std::string> props;

    if (fs::exists(path)) {
        std::ifstream in(path);
        if (in.is_open()) {
            std::string line;
            while (std::getline(in, line)) {
                if (line.empty() || line[0] == '#') continue;
                if (size_t pos = line.find('='); pos != std::string::npos) {
                    props[line.substr(0, pos)] = line.substr(pos + 1);
                }
            }
            in.close();
        }
    }

    for (const auto &p : data) {
        props[p.key] = p.value;
    }

    std::ofstream out(path, std::ios::trunc);
    if (!out) {
        throw std::system_error(
            errno, std::generic_category(), "Failed to open module.prop for writing: " + path);
    }

    for (const auto &[k, v] : props) {
        out << k << "=" << v << '\n';
    }

    out.close();
    if (!out.good()) {
        throw std::runtime_error("Error writing to module.prop: " + path);
    }
}

} // namespace ModuleProperty
