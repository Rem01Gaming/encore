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

#include <cstdio>
#include <cstdlib>
#include <thread>
#include <unordered_map>
#include <vector>

#include <Dumpsys.hpp>
#include <PIDTracker.hpp>
#include <ShellUtility.hpp>
#include <Encore.hpp>
#include <EncoreConfig.hpp>
#include <EncoreLog.hpp>
#include <EncoreUtility.hpp>

#define LOOP_INTERVAL_SEC 12

std::vector<EncoreGameList> gamelist;

void encore_main_daemon(void) {
   EncoreProfileMode cur_mode = PERFCOMMON;
   DumpsysWindowDisplays window_displays;

   EncoreGameList *active_game = nullptr;
   bool battery_saver_state = false;
   bool battery_saver_state_oops = false;
   bool need_profile_checkup = false;
   bool game_requested_dnd = false;

   PIDTracker pid_tracker;

   run_perfcommon();

   auto GetActiveGame = [&](const std::vector<RecentAppList> &recent_applist,
                            std::vector<EncoreGameList> &gamelist) -> EncoreGameList * {
       std::unordered_map<std::string, EncoreGameList *> game_packages;
       for (auto &game : gamelist) {
           game_packages[game.package_name] = &game;
       }

       for (const auto &recent : recent_applist) {
           if (!recent.visible) continue;

           auto it = game_packages.find(recent.package_name);
           if (it != game_packages.end()) {
               return it->second;
           }
       }

       return nullptr;
   };

   auto GetBatterySaverState = [&]() -> std::optional<bool> {
       static bool use_settings_method = true;

       if (use_settings_method) {
           auto pipe = popen_direct({"/system/bin/cmd", "settings", "get", "global", "low_power"});

           if (pipe) {
               char buffer[16];
               if (fgets(buffer, sizeof(buffer), pipe.get())) {
                   std::string result = buffer;
                   result.erase(
                       std::remove_if(
                           result.begin(), result.end(),
                           [](unsigned char c) { return std::isspace(c); }),
                       result.end());

                   if (result == "1") return true;
                   if (result == "0") return false;
               }
           }

           use_settings_method = false;
       }

       try {
           DumpsysPower dumpsys_power;
           Dumpsys::Power(dumpsys_power);
           return dumpsys_power.battery_saver;
       } catch (const std::runtime_error &e) {
           LOGE_TAG("DumpsysPower", "{}", e.what());
           return std::nullopt;
       }
   };

   auto pidof_game = [&](const std::string &package_name) -> pid_t {
       pid_t game_pid = 0;
       try {
           game_pid = Dumpsys::GetAppPID(package_name);
           return game_pid;
       } catch (const std::runtime_error& e) {
           std::string error_msg = e.what();
           LOGE_TAG("DumpsysGetAppPID", "{}", error_msg);
           return 0;
       }
   };

   while(true) {
       std::this_thread::sleep_for(std::chrono::milliseconds(LOOP_INTERVAL_SEC * 1000));

       if (access(MODULE_UPDATE, F_OK) == 0) [[unlikely]] {
           LOGI("Module update detected, exiting");
           notify("Please reboot your device to complete module update.");
           break;
       }

       try {
           Dumpsys::WindowDisplays(window_displays);
       } catch (const std::runtime_error &e) {
           std::string error_msg = e.what();
           LOGE_TAG("DumpsysWindowDisplays", "{}", error_msg);
           return;
       }

       if (active_game == nullptr) {
           active_game = GetActiveGame(window_displays.recent_app, gamelist);
       } else if (!pid_tracker.is_valid()) [[unlikely]] {
           LOGI("Game {} exited, resetting profile...", active_game->package_name);
           active_game = nullptr;
           pid_tracker.invalidate();

           // Check for new game session
           active_game = GetActiveGame(window_displays.recent_app, gamelist);
           need_profile_checkup = true;
       }

       if (active_game == nullptr && !battery_saver_state_oops) {
           auto bs_state = GetBatterySaverState();
           battery_saver_state_oops = !bs_state.has_value();
           battery_saver_state = bs_state.value_or(false);

           if (battery_saver_state_oops) {
               LOGE("GetBatterySaverState is out of order!");
           }
       }

       if (active_game != nullptr && window_displays.screen_awake) {
           // Bail out if we already on performance profile
           // However we will pass this if need_profile_checkup was true
           if (!need_profile_checkup && cur_mode == PERFORMANCE_PROFILE)
               continue;

           pid_t game_pid = pidof_game(active_game->package_name);
           if (game_pid == 0) {
               LOGE("Unable to fetch PID of {}", active_game->package_name);
               active_game = nullptr;
               pid_tracker.invalidate();
               continue;
           }

           need_profile_checkup = false;
           cur_mode = PERFORMANCE_PROFILE;

           LOGI("Applying performance profile for {} (PID: {})", active_game->package_name, game_pid);
           apply_performance_profile(active_game->lite_mode, active_game->package_name, game_pid);
           pid_tracker.set_pid(game_pid);

           if (active_game->enable_dnd) {
               game_requested_dnd = true;
               set_do_not_disturb(active_game->enable_dnd);
           } else {
               game_requested_dnd = false;
               set_do_not_disturb(false);
           }
       } else if (battery_saver_state) {
           // Bail out if we already on powersave profile
           if (cur_mode == POWERSAVE_PROFILE)
               continue;

           cur_mode = POWERSAVE_PROFILE;
           need_profile_checkup = false;
           LOGI("Applying powersave profile");
           apply_powersave_profile();

           if (game_requested_dnd) {
               set_do_not_disturb(false);
               game_requested_dnd = false;
           }
       } else {
           // Bail out if we already on normal profile
           if (cur_mode == BALANCE_PROFILE)
               continue;

           cur_mode = BALANCE_PROFILE;
           need_profile_checkup = false;
           LOGI("Applying balance profile");
           apply_balance_profile();

           if (game_requested_dnd) {
               set_do_not_disturb(false);
               game_requested_dnd = false;
           }
       }
   }

   return;
}

int main(void) {
   if (getuid() != 0) {
       fprintf(stderr, "\033[31mERROR:\033[0m Please run this program as root\n");
       return EXIT_FAILURE;
   }

   if (!create_lock_file()) {
       fprintf(stderr, "\033[31mERROR:\033[0m Another instance of Encore Daemon is already running!\n");
       return EXIT_FAILURE;
   }

   if (access(ENCORE_GAMELIST, F_OK) != 0) {
       fprintf(stderr, "\033[31mERROR:\033[0m %s is missing\n", ENCORE_GAMELIST);
       return EXIT_FAILURE;
   }

   if (!check_dumpsys_sanity()) {
       fprintf(stderr, "\033[31mERROR:\033[0m Dumpsys sanity check failed, it might be tampered by other module.\n");
       return EXIT_FAILURE;
   }

   if (!load_gamelist_from_json(ENCORE_GAMELIST, gamelist)) {
       fprintf(stderr, "\033[31mERROR:\033[0m Failed to load initial gamelist\n");
       return EXIT_FAILURE;
   }

   if (daemon(0, 0)) {
       LOGC("Failed to daemonize service");
       return EXIT_FAILURE;
   }

   if (!init_file_watcher()) {
       LOGC("Failed to initialize file watcher");
       return EXIT_FAILURE;
   }

   LOGI("Encore Tweaks daemon started");
   encore_main_daemon();

   if (json_watcher) {
       xWatcher_destroy(json_watcher);
   }
}
