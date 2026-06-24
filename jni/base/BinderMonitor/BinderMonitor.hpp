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

class BinderMonitor {
public:
    /**
     * @brief Returns the process-wide singleton.
     */
    static BinderMonitor &get();

    BinderMonitor(const BinderMonitor &) = delete;

    BinderMonitor &operator=(const BinderMonitor &) = delete;

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
     * @brief Queries Battery Saver state from PowerManagerService.
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
