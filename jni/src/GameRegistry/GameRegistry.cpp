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

#include <filesystem>
#include <fstream>
#include <sstream>
#include <unistd.h>

#include "GameRegistry.hpp"

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace fs = std::filesystem;

bool GameRegistry::load_from_json(const std::string &filename) {
    if (!fs::exists(filename)) {
        LOGE_TAG("GameRegistry", "{}: File not found", filename);
        return false;
    }

    std::ifstream ifs(filename, std::ios::in | std::ios::binary);
    if (!ifs.is_open()) {
        LOGE_TAG("GameRegistry", "{}: {}", filename, strerror(errno));
        return false;
    }

    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    ifs.close();

    rapidjson::Document doc;
    doc.Parse(content.c_str());

    if (doc.HasParseError()) {
        LOGE_TAG(
            "GameRegistry", "{} parse error: {} (Offset: {})", filename,
            rapidjson::GetParseError_En(doc.GetParseError()), doc.GetErrorOffset());
        return false;
    }

    if (!doc.IsObject()) {
        LOGE_TAG("GameRegistry", "{} root is not an object", filename);
        return false;
    }

    std::vector<EncoreGameList> new_list;

    for (auto it = doc.MemberBegin(); it != doc.MemberEnd(); ++it) {
        if (!it->name.IsString()) {
            LOGE_TAG("GameRegistry", "Skipping non-string key in gamelist");
            continue;
        }

        if (!it->value.IsObject()) {
            LOGE_TAG(
                "GameRegistry", "Value for {} is not an object, skipping", it->name.GetString());
            continue;
        }

        const rapidjson::Value &game_obj = it->value;
        EncoreGameList game;
        game.package_name = it->name.GetString();

        // Validate package name
        if (game.package_name.empty()) {
            LOGE_TAG("GameRegistry", "Empty package name found, skipping");
            continue;
        }

        // Parse lite_mode with default to false
        if (game_obj.HasMember("lite_mode") && game_obj["lite_mode"].IsBool()) {
            game.lite_mode = game_obj["lite_mode"].GetBool();
        } else {
            game.lite_mode = false;
        }

        // Parse enable_dnd with default to false
        if (game_obj.HasMember("enable_dnd") && game_obj["enable_dnd"].IsBool()) {
            game.enable_dnd = game_obj["enable_dnd"].GetBool();
        } else {
            game.enable_dnd = false;
        }

        new_list.push_back(game);
    }

    update_gamelist(new_list);
    LOGI_TAG("GameRegistry", "Loaded {} games from {}", new_list.size(), filename);
    return true;
}

bool GameRegistry::populate_from_base(const std::string &gamelist, const std::string &baselist) {
    auto IsAppInstalled = [](const std::string &package_name) -> bool {
        return (access(("/data/data/" + package_name).c_str(), F_OK) == 0);
    };

    std::ifstream base_file(baselist);
    if (!base_file.is_open()) {
        LOGE_TAG("GameRegistry", "Failed to open base gamelist: {}", baselist);
        return false;
    }

    std::vector<EncoreGameList> game_list;
    std::string package_name;

    while (std::getline(base_file, package_name)) {
        package_name.erase(package_name.find_last_not_of(" \t\r\n") + 1);
        if (package_name.empty()) continue;

        if (IsAppInstalled(package_name)) {
            EncoreGameList game;
            game.package_name = package_name;
            game.lite_mode = false;
            game.enable_dnd = false;
            game_list.push_back(game);
        }
    }

    base_file.close();

    rapidjson::Document doc;
    doc.SetObject();
    rapidjson::Document::AllocatorType &allocator = doc.GetAllocator();

    for (const auto &game : game_list) {
        rapidjson::Value game_obj(rapidjson::kObjectType);
        game_obj.AddMember("lite_mode", game.lite_mode, allocator);
        game_obj.AddMember("enable_dnd", game.enable_dnd, allocator);

        rapidjson::Value key(game.package_name.c_str(), allocator);
        doc.AddMember(key, game_obj, allocator);
    }

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    std::ofstream output_file(gamelist);
    if (!output_file.is_open()) {
        LOGE_TAG("GameRegistry", "Failed to create gamelist: {}", gamelist);
        return false;
    }

    output_file << buffer.GetString();
    output_file.close();

    LOGI_TAG("GameRegistry", "Populated gamelist JSON with {} games", game_list.size());

    return true;
}

void GameRegistry::update_gamelist(const std::vector<EncoreGameList> &new_list) {
    std::unique_lock lock(mutex_);
    game_packages_.clear();

    for (const auto &game : new_list) {
        if (validate_game_entry(game)) {
            game_packages_[game.package_name] = game;
        }
    }

    LOGI_TAG("GameRegistry", "Updated registry with {} games", game_packages_.size());
}

std::optional<EncoreGameList> GameRegistry::find_game(const std::string &package_name) const {
    std::shared_lock lock(mutex_);
    auto it = game_packages_.find(package_name);
    if (it != game_packages_.end()) {
        return it->second;
    }

    return std::nullopt;
}

const EncoreGameList *GameRegistry::find_game_ptr(const std::string &package_name) const {
    std::shared_lock lock(mutex_);
    auto it = game_packages_.find(package_name);
    if (it != game_packages_.end()) {
        return &it->second;
    }

    return nullptr;
}

bool GameRegistry::is_game_registered(const std::string &package_name) const {
    std::shared_lock lock(mutex_);
    return game_packages_.find(package_name) != game_packages_.end();
}

size_t GameRegistry::size() const {
    std::shared_lock lock(mutex_);
    return game_packages_.size();
}

std::vector<std::string> GameRegistry::get_all_package_names() const {
    std::shared_lock lock(mutex_);
    std::vector<std::string> packages;
    packages.reserve(game_packages_.size());

    for (const auto &[package_name, _] : game_packages_) {
        packages.push_back(package_name);
    }

    return packages;
}

bool GameRegistry::validate_game_entry(const EncoreGameList &game) const {
    if (game.package_name.empty()) {
        LOGW_TAG("GameRegistry", "Skipping game with empty package name");
        return false;
    }

    return true;
}
