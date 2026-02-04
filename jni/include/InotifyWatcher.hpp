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

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <fcntl.h>
#include <linux/limits.h>
#include <poll.h>
#include <sys/inotify.h>
#include <unistd.h>

#include <EncoreLog.hpp>

/**
 * @class InotifyWatcher
 * @brief A C++ wrapper for Linux's inotify API to monitor file system events.
 *
 * @details This class provides a simple interface to watch for events on specific files or entire directories.
 * It runs a dedicated thread to process inotify events and invokes user-defined callbacks
 * when events occur.
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

    // Constants for event buffer handling
    static constexpr size_t EVENT_SIZE = sizeof(struct inotify_event);
    static constexpr size_t BUF_LEN = 1024 * (EVENT_SIZE + NAME_MAX + 1);
    static constexpr char DIR_SEPARATOR = '/';

    std::vector<DirectoryWatch> directories_; /// A list of all directories being watched
    pthread_t thread_;                        /// The worker thread for processing events
    std::atomic<bool> alive_;                 /// A flag to control the life of the worker thread
    std::atomic<bool> thread_started_;        /// Flag to track if thread was started
    int inotify_fd_;                          /// The file descriptor for the inotify instance
    std::mutex directories_mutex_;            /// Mutex to protect access to directories_

    /**
     * @brief The static entry point for the worker thread.
     * 
     * @param argument A pointer to the InotifyWatcher instance.
     */
    static void *threadProcess(void *argument) {
        InotifyWatcher *watcher = static_cast<InotifyWatcher *>(argument);
        pthread_setname_np(pthread_self(), "InotifyWatcher");
        watcher->processEvents();
        return nullptr;
    }

    /**
     * @brief The main event processing loop for the worker thread.
     */
    void processEvents() {
        char buffer[BUF_LEN];

        while (alive_.load(std::memory_order_acquire)) {
            struct pollfd pfd = {inotify_fd_, POLLIN, 0};
            int ret = poll(&pfd, 1, 50); // Use a timeout to periodically check the 'alive_' flag.

            if (ret < 0) {
                LOGE_TAG("InotifyWatcher", "Poll failed: {}", strerror(errno));
                break;
            } else if (ret == 0) {
                continue; // Timeout, loop again.
            }

            // Read events from the inotify file descriptor.
            ssize_t length = read(inotify_fd_, buffer, BUF_LEN);
            if (length < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    continue; // No data available
                }
                LOGE_TAG("InotifyWatcher", "Read failed: {}", strerror(errno));
                break;
            }

            // Process all events in the buffer.
            ssize_t i = 0;
            while (i < length) {
                struct inotify_event *event = reinterpret_cast<struct inotify_event *>(&buffer[i]);

                // Extract callback information under lock protection
                EventCallback callback_to_call;
                std::string callback_path;
                int callback_context = 0;
                void *callback_additional_data = nullptr;
                bool valid_event = false;

                {
                    std::unique_lock<std::mutex> lock(directories_mutex_);

                    // Find the directory corresponding to the watch descriptor.
                    auto dir_it = std::find_if(directories_.begin(), directories_.end(), [event](const DirectoryWatch &dir) {
                        return dir.inotify_watch_fd == event->wd;
                    });

                    if (dir_it == directories_.end()) {
                        // This can happen if a watch was removed but an event was already in flight.
                        // It's usually safe to ignore.
                        i += EVENT_SIZE + event->len;
                        continue;
                    }

                    DirectoryWatch &directory = *dir_it;
                    // The event->name is only valid if len > 0.
                    std::string event_name = (event->len > 0) ? std::string(event->name) : "";

                    // Check if the event matches a specific file watch.
                    auto file_it =
                        std::find_if(directory.files.begin(), directory.files.end(), [&event_name](const FileWatch &file) {
                            return file.name == event_name;
                        });

                    if (file_it != directory.files.end()) {
                        // Event is for a specific file, use its callback.
                        FileWatch &file = *file_it;
                        callback_path = directory.path + DIR_SEPARATOR + file.name;
                        callback_to_call = file.callback_func;
                        callback_context = file.context;
                        callback_additional_data = file.additional_data;
                        valid_event = true;
                    } else if (directory.callback_func) {
                        // Event is not for a specific file, use the general directory callback if available.
                        callback_to_call = directory.callback_func;
                        callback_path = directory.path;
                        if (!event_name.empty()) {
                            callback_path += DIR_SEPARATOR + event_name;
                        }
                        callback_context = directory.context;
                        callback_additional_data = directory.additional_data;
                        valid_event = true;
                    }
                } // Lock released here

                // Call callback if it exists and we found a valid watch
                if (valid_event && callback_to_call) {
                    try {
                        callback_to_call(event, callback_path, callback_context, callback_additional_data);
                    } catch (const std::exception &e) {
                        LOGE_TAG("InotifyWatcher", "Callback exception: {}", e.what());
                    } catch (...) {
                        LOGE_TAG("InotifyWatcher", "Unknown exception in callback");
                    }
                }

                // Move to the next event in the buffer.
                i += EVENT_SIZE + event->len;
            }
        }

        cleanup();
    }

    /**
     * @brief Cleans up inotify watches and closes the file descriptor.
     */
    void cleanup() {
        std::lock_guard<std::mutex> lock(directories_mutex_);
        for (auto &directory : directories_) {
            inotify_rm_watch(inotify_fd_, directory.inotify_watch_fd);
        }
        // The main file descriptor is closed in the destructor to avoid race conditions.
    }

public:
    /**
     * @brief Constructs an InotifyWatcher object.
     * 
     * @throws std::runtime_error if inotify initialization fails.
     */
    InotifyWatcher()
        : alive_(false)
        , thread_started_(false)
        , inotify_fd_(-1) {
        inotify_fd_ = inotify_init1(O_NONBLOCK | O_CLOEXEC);
        if (inotify_fd_ < 0) {
            LOGE_TAG("InotifyWatcher", "Failed to initialize inotify: {}", strerror(errno));
            throw std::runtime_error("Failed to initialize inotify");
        }
    }

    /**
     * @brief Destroys the InotifyWatcher object.
     */
    ~InotifyWatcher() {
        stop();
        if (inotify_fd_ >= 0) {
            close(inotify_fd_);
        }
    }

    /**
     * @brief Adds a watch for a specific file.
     *
     * @param reference A WatchReference object containing the path, callback, and context.
     * @return true on success, otherwise false (e.g., invalid path, file already watched).
     */
    bool addFile(const WatchReference &reference) {
        std::string path = reference.path;

        // Remove trailing slash if present
        if (!path.empty() && path.back() == DIR_SEPARATOR) {
            path.pop_back();
        }

        // Extract directory and filename
        size_t last_slash = path.find_last_of(DIR_SEPARATOR);
        if (last_slash == std::string::npos) {
            LOGE_TAG("InotifyWatcher", "Invalid file path: {}", path);
            return false; // No directory component found
        }

        std::string dir_path = path.substr(0, last_slash);
        std::string filename = path.substr(last_slash + 1);

        if (filename.empty()) {
            LOGE_TAG("InotifyWatcher", "Empty filename in path: {}", path);
            return false;
        }

        std::lock_guard<std::mutex> lock(directories_mutex_);

        // Find or create directory
        auto dir_it = std::find_if(directories_.begin(), directories_.end(), [&dir_path](const DirectoryWatch &dir) {
            return dir.path == dir_path;
        });

        if (dir_it == directories_.end()) {
            DirectoryWatch new_dir;
            new_dir.path = dir_path;
            new_dir.callback_func = nullptr; // No directory callback for file watches
            new_dir.context = 0;
            new_dir.additional_data = nullptr;
            new_dir.inotify_watch_fd = -1;

            // Add inotify watch for common file events
            new_dir.inotify_watch_fd =
                inotify_add_watch(inotify_fd_, dir_path.c_str(), IN_MODIFY | IN_CLOSE_WRITE | IN_DELETE | IN_MOVE | IN_CREATE);

            if (new_dir.inotify_watch_fd == -1) {
                LOGE_TAG("InotifyWatcher", "Cannot add file watch for directory '{}': {}", dir_path, strerror(errno));
                return false;
            }

            directories_.push_back(std::move(new_dir));
            dir_it = directories_.end() - 1;
        }

        DirectoryWatch &directory = *dir_it;

        auto file_exists = std::find_if(directory.files.begin(), directory.files.end(), [&filename](const FileWatch &file) {
            return file.name == filename;
        });

        // File already being watched
        if (file_exists != directory.files.end()) {
            LOGE_TAG("InotifyWatcher", "File '{}' already being watched in directory '{}'", filename, dir_path);
            return false;
        }

        FileWatch new_file;
        new_file.name = filename;
        new_file.context = reference.context;
        new_file.additional_data = reference.additional_data;
        new_file.callback_func = reference.callback_func;

        directory.files.push_back(std::move(new_file));
        LOGD_TAG("InotifyWatcher", "Added file watch for '{}' in directory '{}'", filename, dir_path);
        return true;
    }

    /**
     * @brief Adds a watch for a directory.
     *
     * @param reference A WatchReference object containing the path, callback, and context.
     * @return true on success, otherwise false.
     */
    bool addDirectory(const WatchReference &reference) {
        std::string path = reference.path;

        // Remove trailing slash if present
        if (!path.empty() && path.back() == DIR_SEPARATOR) {
            path.pop_back();
        }

        std::lock_guard<std::mutex> lock(directories_mutex_);

        // Check if directory already exists
        auto dir_it = std::find_if(directories_.begin(), directories_.end(), [&path](const DirectoryWatch &dir) {
            return dir.path == path;
        });

        if (dir_it != directories_.end()) {
            // Directory exists, update callback if not set
            if (dir_it->callback_func) {
                LOGE_TAG("InotifyWatcher", "Directory '{}' already has a callback", path);
                return false;
            }

            dir_it->callback_func = reference.callback_func;
            dir_it->context = reference.context;
            dir_it->additional_data = reference.additional_data;
            LOGD_TAG("InotifyWatcher", "Updated callback for directory '{}'", path);
            return true;
        }

        // Create new directory watch
        DirectoryWatch new_dir;
        new_dir.path = path;
        new_dir.callback_func = reference.callback_func;
        new_dir.context = reference.context;
        new_dir.additional_data = reference.additional_data;

        // Add inotify watch for directory events
        new_dir.inotify_watch_fd = inotify_add_watch(
            inotify_fd_, path.c_str(), IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVED_FROM | IN_MOVED_TO | IN_CLOSE_WRITE
        );

        if (new_dir.inotify_watch_fd == -1) {
            LOGE_TAG("InotifyWatcher", "Cannot add directory watch for '{}': {}", path, strerror(errno));
            return false;
        }

        directories_.push_back(std::move(new_dir));
        LOGD_TAG("InotifyWatcher", "Added directory watch for '{}'", path);
        return true;
    }

    /**
     * @brief Starts the event processing thread.
     * 
     * @return true if the thread was started successfully, otherwise false.
     */
    bool start() {
        bool expected = false;
        if (!alive_.compare_exchange_strong(expected, true)) {
            LOGE_TAG("InotifyWatcher", "Already running!");
            return false;
        }

        int result = pthread_create(&thread_, nullptr, threadProcess, this);

        if (result != 0) {
            alive_.store(false, std::memory_order_release);
            LOGE_TAG("InotifyWatcher", "Failed to create thread: {}", strerror(result));
            return false;
        }

        thread_started_.store(true, std::memory_order_release);
        LOGD_TAG("InotifyWatcher", "Started monitoring thread");
        return true;
    }

    /**
     * @brief Stops the event processing thread.
     */
    void stop() {
        bool expected = true;
        if (alive_.compare_exchange_strong(expected, false)) {
            if (thread_started_.load(std::memory_order_acquire)) {
                pthread_join(thread_, nullptr);
                thread_started_.store(false, std::memory_order_release);
            }
        }
    }

    /**
     * @brief Checks if the watcher is running.
     * 
     * @return true if the watcher is active, false otherwise.
     */
    bool isRunning() const {
        return alive_.load(std::memory_order_acquire);
    }

    /**
     * @brief Removes a file watch.
     * * @param path The full path to the file to stop watching.
     * @return true if the file was being watched and was removed, false otherwise.
     */
    bool removeFile(const std::string &path) {
        // Extract directory and filename
        size_t last_slash = path.find_last_of(DIR_SEPARATOR);
        if (last_slash == std::string::npos) {
            return false;
        }

        std::string dir_path = path.substr(0, last_slash);
        std::string filename = path.substr(last_slash + 1);

        std::lock_guard<std::mutex> lock(directories_mutex_);

        // Find directory
        auto dir_it = std::find_if(directories_.begin(), directories_.end(), [&dir_path](const DirectoryWatch &dir) {
            return dir.path == dir_path;
        });

        if (dir_it == directories_.end()) {
            return false;
        }

        // Remove file from directory's watch list
        auto &files = dir_it->files;
        auto file_it =
            std::find_if(files.begin(), files.end(), [&filename](const FileWatch &file) { return file.name == filename; });

        if (file_it == files.end()) {
            return false;
        }

        files.erase(file_it);
        LOGD_TAG("InotifyWatcher", "Removed file watch for '{}'", path);

        // If no more files and no directory callback, remove the directory watch
        if (dir_it->files.empty() && dir_it->callback_func == nullptr) {
            inotify_rm_watch(inotify_fd_, dir_it->inotify_watch_fd);
            directories_.erase(dir_it);
            LOGD_TAG("InotifyWatcher", "Removed empty directory watch for '{}'", dir_path);
        }

        return true;
    }

    /**
     * @brief Removes a directory watch.
     * 
     * @param path The path to the directory to stop watching.
     * @return true if the directory was being watched and was removed, false otherwise.
     */
    bool removeDirectory(const std::string &path) {
        std::string clean_path = path;
        if (!clean_path.empty() && clean_path.back() == DIR_SEPARATOR) {
            clean_path.pop_back();
        }

        std::lock_guard<std::mutex> lock(directories_mutex_);

        auto dir_it = std::find_if(directories_.begin(), directories_.end(), [&clean_path](const DirectoryWatch &dir) {
            return dir.path == clean_path;
        });

        if (dir_it == directories_.end()) {
            return false;
        }

        // Remove the inotify watch
        inotify_rm_watch(inotify_fd_, dir_it->inotify_watch_fd);

        // Remove from list
        directories_.erase(dir_it);
        LOGD_TAG("InotifyWatcher", "Removed directory watch for '{}'", clean_path);
        return true;
    }

    // Disable copying
    InotifyWatcher(const InotifyWatcher &) = delete;
    InotifyWatcher &operator=(const InotifyWatcher &) = delete;
};
