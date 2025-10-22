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

#include <shared_mutex>
#include <unordered_map>
#include <vector>
#include <string>
#include <optional>

#include "Encore.hpp"
#include "EncoreLog.hpp"

class GameRegistry {
private:
    std::unordered_map<std::string, EncoreGameList> game_packages_;
    mutable std::shared_mutex mutex_;

public:
    /**
     * @brief Updates the game registry with new game list data
     * @param new_list The new list of games to register
     */
    void update_gamelist(const std::vector<EncoreGameList>& new_list) {
        std::unique_lock lock(mutex_);
        game_packages_.clear();

        for (const auto& game : new_list) {
            if (validate_game_entry(game)) {
                game_packages_[game.package_name] = game;
            }
        }

        LOGI_TAG("GameRegistry", "Updated registry with {} games", game_packages_.size());
    }

    /**
     * @brief Finds a game by package name
     * @param package_name The package name to search for
     * @return Optional containing the game if found, empty if not found
     */
    std::optional<EncoreGameList> find_game(const std::string& package_name) const {
        std::shared_lock lock(mutex_);
        auto it = game_packages_.find(package_name);
        if (it != game_packages_.end()) {
            return it->second;
        }

        return std::nullopt;
    }

    /**
     * @brief Finds a game by package name (pointer version for performance)
     * @param package_name The package name to search for
     * @return Pointer to the game if found, nullptr if not found
     */
    const EncoreGameList* find_game_ptr(const std::string& package_name) const {
        std::shared_lock lock(mutex_);
        auto it = game_packages_.find(package_name);
        if (it != game_packages_.end()) {
            return &it->second;
        }

        return nullptr;
    }

    /**
     * @brief Checks if a package is registered as a game
     * @param package_name The package name to check
     * @return True if the package is a registered game
     */
    bool is_game_registered(const std::string& package_name) const {
        std::shared_lock lock(mutex_);
        return game_packages_.find(package_name) != game_packages_.end();
    }

    /**
     * @brief Gets the number of registered games
     * @return The number of games in the registry
     */
    size_t size() const {
        std::shared_lock lock(mutex_);
        return game_packages_.size();
    }

    /**
     * @brief Gets all registered game package names
     * @return Vector of all package names
     */
    std::vector<std::string> get_all_package_names() const {
        std::shared_lock lock(mutex_);
        std::vector<std::string> packages;
        packages.reserve(game_packages_.size());

        for (const auto& [package_name, _] : game_packages_) {
            packages.push_back(package_name);
        }

        return packages;
    }

private:
    /**
     * @brief Validates a game entry before adding to registry
     * @param game The game entry to validate
     * @return True if the entry is valid
     */
    bool validate_game_entry(const EncoreGameList& game) const {
        if (game.package_name.empty()) {
            LOGW_TAG("GameRegistry", "Skipping game with empty package name");
            return false;
        }

        return true;
    }
};

extern GameRegistry game_registry;
