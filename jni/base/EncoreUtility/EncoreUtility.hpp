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

#pragma once

#include <cstdio>
#include <cstdlib>
#include <string>
#include <unistd.h>

#include <Encore.hpp>
#include <EncoreLog.hpp>

/**
 * @brief  Fetches PID of a process.
 * @param  target_name: The package name.
 * @param  strict: Strict matching mode. If true, "com.mobile.legends" will NOT match "com.mobile.legends:UnityKillsMe"
 * @return The PID of the package if found, otherwise 0.
 */
[[nodiscard]] pid_t pidof(std::string_view target_name, bool strict = true);

/**
 * @brief Retrieves the UID of a given package name by checking its data directory.
 *
 * @param package_name The package name of the application (e.g., "com.termux").
 * @return The UID of the package if found, otherwise 0.
 */
[[nodiscard]] uid_t get_uid_by_package_name(const std::string &package_name);

/**
 * @brief Posts a notification via shell.
 *
 * @param message The message content of the notification.
 * @note It is only intended for use in an Android environment.
 */
void notify(const char *message);

/**
 * @brief Whether a package currently has the resumed (on-screen) activity.
 */
enum class ResumedState {
    Resumed,    ///< The package has a resumed activity.
    NotResumed, ///< Another app is resumed and the package isn't.
    Unknown,    ///< Can't tell (dumpsys failed, output format unknown, lock screen...).
};

/**
 * @brief Parses the output of `dumpsys activity activities` looking at the
 *        "*ResumedActivity" lines.
 *
 * Only reports NotResumed when a different *app* is resumed. System UI and the
 * "android" package (lock screen, shade) do not count, so a lock screen drawn over
 * the game never looks like "the user left the game".
 *
 * @param stream The dumpsys output.
 * @param package The package name to look for.
 */
ResumedState parse_resumed_activities(FILE *stream, const std::string &package);

/**
 * @brief Runs dumpsys and tells whether the package is on screen right now.
 * @note Takes a few hundred milliseconds, don't call it in a hot path.
 *       Returns Unknown on any failure so callers can keep their old behaviour.
 */
ResumedState query_package_resumed(const std::string &package);

/**
 * @brief Shows a toast message (best effort).
 *
 * Android has no shell command for toast, so this relies on the helper app
 * "bellavita.toast". If it is not installed this function does nothing and
 * returns quietly, the notification posted by notify() still works.
 *
 * @param message The message to show.
 * @note It is only intended for use in an Android environment.
 */
void toast(const char *message);

/**
 * @brief Sets the do not disturb mode via shell.
 *
 * @param do_not_disturb True to enable DND mode, false to disable.
 * @note It is only intended for use in an Android environment.
 */
void set_do_not_disturb(bool do_not_disturb);
