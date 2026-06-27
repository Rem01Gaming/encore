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

#include <cstdint>
#include <functional>
#include <string>

/**
 * @brief Callbacks fired by the IProcessObserver binder hosted in ActivityManagerService.
 */
struct ProcessObserverCallbacks {
    std::function<void(int32_t pid, int32_t uid, bool foreground)> onForegroundActivitiesChanged;
    std::function<void(int32_t pid, int32_t uid, int32_t serviceTypes)> onForegroundServicesChanged;
    std::function<void(int32_t pid, int32_t uid)> onProcessDied;
    std::function<void(int32_t pid, int32_t processUid, int32_t packageUid, const std::string &packageName, const std::string &processName)> onProcessStarted;
};

using DisplayStateCallback = std::function<void(bool isInteractive)>;
using PowerSaveCallback = std::function<void(bool isPowerSave)>;

class BinderMonitor {
public:
    /**
     * @brief Returns the process-wide singleton.
     */
    static BinderMonitor &get();
    BinderMonitor(const BinderMonitor &) = delete;
    BinderMonitor &operator=(const BinderMonitor &) = delete;
    ~BinderMonitor();

    /**
     * @brief Resolves all required binder transaction codes at runtime.
     *
     * @return true if all required services and transaction codes were acquired successfully.
     */
    bool initialize();

    /**
     * @brief Sets the callbacks invoked on IProcessObserver events from ActivityManagerService.
     *
     * @param callbacks Struct with per-event handler functions.
     */
    void setProcessObserverCallbacks(ProcessObserverCallbacks callbacks);

    /**
     * @brief Sets the callback invoked when screen interactivity changes.
     *
     * @param callback Receives true when the screen becomes interactive, false when it goes off.
     */
    void setDisplayStateCallback(DisplayStateCallback callback);

    /**
     * @brief Sets the callback invoked when Battery Saver state changes.
     *
     * @param callback Receives true when Battery Saver becomes active, false when it turns off.
     */
    void setPowerSaveCallback(PowerSaveCallback callback);

    /**
     * @brief Queries Battery Saver state from PowerManagerService synchronously.
     *
     * @return true if Battery Saver is active, false otherwise or on binder failure.
     */
    bool isPowerSave();

    /**
     * @brief Queries zen mode from NotificationManagerService.
     *
     * @return Zen mode integer (0 = off, non-zero = active mode), or -1 on failure.
     */
    int32_t getZenMode();

    /**
     * @brief Blocks the calling thread by joining the binder thread pool.
     */
    void joinThreadPool();

private:
    BinderMonitor() = default;
};
