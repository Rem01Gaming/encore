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

#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <linux/limits.h>
#include <sys/inotify.h>

/**
 * @class InotifyWatcher
 * @brief A C++ wrapper for Linux's inotify API to monitor file system events.
 */
class InotifyWatcher {
public:
    /**
     * @brief A callback function type for handling inotify events.
     *
     * @param event A pointer to the inotify_event structure.
     * @param path The full path of the file or directory associated with the event.
     * @param context An integer context value provided by the user.
     * @param additional_data A void pointer for user-defined data.
     */
    using EventCallback =
        std::function<void(const struct inotify_event *event, const std::string &path, int context, void *additional_data)>;

    /**
     * @struct WatchReference
     * @brief A structure to hold all the information needed to add a new watch.
     */
    struct WatchReference {
        std::string path;            /// The full path to the file or directory to watch
        EventCallback callback_func; /// The callback function to invoke on an event
        int context;                 /// An integer context value to pass to the callback
        void *additional_data;       /// A pointer to additional data to pass to the callback
    };

    /**
     * @brief Constructs an InotifyWatcher object.
     *
     * @throws std::runtime_error if inotify or eventfd initialization fails.
     */
    InotifyWatcher();

    /**
     * @brief Destroys the InotifyWatcher object, ensuring the watcher thread is stopped cleanly.
     */
    ~InotifyWatcher();

    /**
     * @brief Adds a watch for a specific file.
     *
     * @param reference A WatchReference object containing the path, callback, and context.
     * @return true on success, otherwise false (e.g., invalid path, file already watched).
     */
    bool addFile(const WatchReference &reference);

    /**
     * @brief Adds a watch for a directory.
     *
     * @param reference A WatchReference object containing the path, callback, and context.
     * @return true on success, otherwise false.
     */
    bool addDirectory(const WatchReference &reference);

    /**
     * @brief Starts the event processing thread.
     *
     * @return true if the thread was started successfully, otherwise false.
     */
    bool start();

    /**
     * @brief Stops the event processing thread and immediately wakes it up if blocking.
     */
    void stop();

    /**
     * @brief Checks if the watcher is running.
     *
     * @return true if the watcher is active, false otherwise.
     */
    bool isRunning() const;

    /**
     * @brief Removes a file watch.
     *
     * @param path The full path to the file to stop watching.
     * @return true if the file was being watched and was removed, false otherwise.
     */
    bool removeFile(const std::string &path);

    /**
     * @brief Removes a directory watch.
     *
     * @param path The path to the directory to stop watching.
     * @return true if the directory was being watched and was removed, false otherwise.
     */
    bool removeDirectory(const std::string &path);

    // Disable copying
    InotifyWatcher(const InotifyWatcher &) = delete;
    InotifyWatcher &operator=(const InotifyWatcher &) = delete;

private:
    /**
     * @struct FileWatch
     * @brief Internal structure to represent a watch on a specific file.
     */
    struct FileWatch {
        std::string name;            /// The name of the file
        int context;                 /// User-provided context
        void *additional_data;       /// User-provided additional data
        EventCallback callback_func; /// Callback for this specific file
    };

    /**
     * @struct DirectoryWatch
     * @brief Internal structure to represent a watch on a directory.
     */
    struct DirectoryWatch {
        std::vector<FileWatch> files; /// List of specific files being watched in this directory
        std::string path;             /// The path of the directory
        int context;                  /// User-provided context for the directory watch
        void *additional_data;        /// User-provided data for the directory watch
        EventCallback callback_func;  /// Callback for events in the directory not matching a specific file
        int inotify_watch_fd;         /// The inotify watch descriptor for this directory
    };

    // Each inotify_event is at least sizeof(inotify_event) bytes, plus a
    // null-terminated filename of up to NAME_MAX (255) bytes. Sizing the buffer
    // for 1024 events at maximum filename length gives ~275 KB well within
    // typical stack limits and large enough to drain a burst without looping.
    static constexpr size_t EVENT_SIZE = sizeof(struct inotify_event);
    static constexpr size_t BUF_LEN = 1024 * (EVENT_SIZE + NAME_MAX + 1);
    static constexpr char DIR_SEPARATOR = '/';

    std::vector<DirectoryWatch> directories_;
    std::unordered_map<int, size_t> wd_to_index_;

    std::thread thread_;           /// Worker thread that drives the inotify event loop
    std::atomic<bool> alive_;      /// Signals the worker thread to keep running
    int inotify_fd_;               /// File descriptor for the inotify instance
    int stop_fd_;                  /// Eventfd used to instantly wake up the poll loop for shutdown
    std::mutex directories_mutex_; /// Protects directories_ and wd_to_index_

    /**
     * @brief The main event processing loop run by the worker thread.
     */
    void processEvents();

    /**
     * @brief Removes all inotify watches.
     *
     * @note The inotify fd is closed in the destructor.
     */
    void cleanup();

    /**
     * @brief Registers a new DirectoryWatch and updates wd_to_index_.
     *
     * @pre directories_mutex_ must be held by the caller.
     */
    void push_directory(DirectoryWatch &&dir);

    /**
     * @brief Removes the DirectoryWatch at @p pos and keeps wd_to_index_ consistent.
     *
     * @pre directories_mutex_ must be held by the caller.
     */
    void erase_directory(std::vector<DirectoryWatch>::iterator pos);
};
