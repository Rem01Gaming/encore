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

#include <cstdio>
#include <cstdlib>
#include <string>
#include <unistd.h>

#include <Encore.hpp>
#include <EncoreLog.hpp>
#include <Write2File.hpp>

/**
 * @brief Creates and locks a file to ensure single instance execution.
 *
 * Attempts to create a lock file and acquire an exclusive, non-blocking lock on it.
 * @return true if the lock file was created and locked successfully, false otherwise.
 */
bool create_lock_file(void);

/**
 * @brief Checks if the /system/bin/dumpsys executable is sane.
 *
 * This function performs a basic sanity check on the dumpsys binary to ensure it has not been
 * tampered with (e.g., emptied by a kill logger module).
 * @return true if the dumpsys binary appears to be sane, false otherwise.
 */
bool check_dumpsys_sanity(void);

/**
 * @brief Retrieves the UID of a given package name by checking its data directory.
 *
 * @param package_name The package name of the application (e.g., "com.termux").
 * @return The UID of the package if found, otherwise 0.
 */
uid_t get_uid_by_package_name(const std::string& package_name);

/**
 * @brief Posts a notification via shell.
 *
 * @param message The message content of the notification.
 * @note It is only intended for use in an Android environment.
 */
void notify(const char* message);

/**
 * @brief Sets the do not disturb mode via shell.
 *
 * @param do_not_disturb True to enable DND mode, false to disable.
 * @note It is only intended for use in an Android environment.
 */
void set_do_not_disturb(bool do_not_disturb);

/**
 * @brief Verifies the module's integrity.
 *
 * This function checks the `module.prop` file for specific 'name' and 'author'
 * fields to ensure the module has not been modified or redistributed by
 * unauthorized third parties. If the check fails, it logs a critical error,
 * sends a notification, and terminates the program.
 */
void is_kanged(void);

void run_perfcommon(void);
void apply_performance_profile(bool lite_mode, std::string game_pkg, pid_t game_pid);
void apply_balance_profile();
void apply_powersave_profile();
