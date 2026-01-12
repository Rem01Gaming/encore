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

#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "Encore.hpp"
#include "EncoreLog.hpp"

class GameRegistry {
private:
    std::unordered_map<std::string, EncoreGameList> game_packages_;
    mutable std::shared_mutex mutex_;

public:
    /**
     * @brief Loads game list from JSON file
     * @param filename Path to the JSON file
     * @return True if successful, false otherwise
     */
    bool load_from_json(const std::string &filename);

    /**
     * @brief Populates game list from base file and saves as JSON
     * @param gamelist Output JSON file path
     * @param baselist Input base file path
     * @return True if successful, false otherwise
     */
    static bool populate_from_base(const std::string &gamelist, const std::string &baselist);

    /**
     * @brief Updates the game registry with new game list data
     * @param new_list The new list of games to register
     */
    void update_gamelist(const std::vector<EncoreGameList> &new_list);

    /**
     * @brief Finds a game by package name
     * @param package_name The package name to search for
     * @return Optional containing the game if found, empty if not found
     */
    std::optional<EncoreGameList> find_game(const std::string &package_name) const;

    /**
     * @brief Finds a game by package name (pointer version for performance)
     * @param package_name The package name to search for
     * @return Pointer to the game if found, nullptr if not found
     */
    const EncoreGameList *find_game_ptr(const std::string &package_name) const;

    /**
     * @brief Checks if a package is registered as a game
     * @param package_name The package name to check
     * @return True if the package is a registered game
     */
    bool is_game_registered(const std::string &package_name) const;

    /**
     * @brief Gets the number of registered games
     * @return The number of games in the registry
     */
    size_t size() const;

    /**
     * @brief Gets all registered game package names
     * @return Vector of all package names
     */
    std::vector<std::string> get_all_package_names() const;

private:
    /**
     * @brief Validates a game entry before adding to registry
     * @param game The game entry to validate
     * @return True if the entry is valid
     */
    bool validate_game_entry(const EncoreGameList &game) const;
};

extern GameRegistry game_registry;
