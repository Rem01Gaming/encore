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
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <fcntl.h>
#include <poll.h>
#include <sys/inotify.h>
#include <unistd.h>

/**
 * @class InotifyWatcher
 * @brief A C++ wrapper for Linux's inotify API to monitor file system events.
 *
 * This class provides a simple interface to watch for events on specific files or entire directories.
 * It runs a dedicated thread to process inotify events and invokes user-defined callbacks
 * when events occur.
 */
class InotifyWatcher {
public:
    /**
     * @brief A callback function type for handling inotify events.
     * @param event A pointer to the inotify_event structure.
     * @param path The full path of the file or directory associated with the event.
     * @param context An integer context value provided by the user.
     * @param additional_data A void pointer for user-defined data.
     */
    using EventCallback = std::function<void(
        const struct inotify_event *event, const std::string &path, int context,
        void *additional_data)>;

    /**
     * @struct WatchReference
     * @brief A structure to hold all the information needed to add a new watch.
     */
    struct WatchReference {
        std::string path; /**< The full path to the file or directory to watch. */
        EventCallback callback_func; /**< The callback function to invoke on an event. */
        int context; /**< An integer context value to pass to the callback. */
        void *additional_data; /**< A pointer to additional data to pass to the callback. */
    };

private:
    /**
     * @struct FileWatch
     * @brief Internal structure to represent a watch on a specific file.
     */
    struct FileWatch {
        std::string name; /**< The name of the file. */
        int context; /**< User-provided context. */
        void *additional_data; /**< User-provided additional data. */
        EventCallback callback_func; /**< Callback for this specific file. */
    };

    /**
     * @struct DirectoryWatch
     * @brief Internal structure to represent a watch on a directory.
     *
     * This can include watches for specific files within the directory or a general
     * watch on the directory itself.
     */
    struct DirectoryWatch {
        std::vector<FileWatch> files; /**< List of specific files being watched in this directory. */
        std::string path; /**< The path of the directory. */
        int context; /**< User-provided context for the directory watch. */
        void *additional_data; /**< User-provided data for the directory watch. */
        EventCallback callback_func; /**< Callback for events in the directory not matching a specific file. */
        int inotify_watch_fd; /**< The inotify watch descriptor for this directory. */
    };

    // Constants for event buffer handling
    static constexpr size_t EVENT_SIZE = sizeof(struct inotify_event);
    static constexpr size_t BUF_LEN = 1024 * (EVENT_SIZE + 16);
    static constexpr char DIR_SEPARATOR = '/';

    std::vector<DirectoryWatch> directories_; /**< A list of all directories being watched. */
    pthread_t thread_; /**< The worker thread for processing events. */
    bool alive_; /**< A flag to control the life of the worker thread. */
    int inotify_fd_; /**< The file descriptor for the inotify instance. */

    /**
     * @brief The static entry point for the worker thread.
     * @param argument A pointer to the InotifyWatcher instance.
     * @return Always returns nullptr.
     */
    static void *threadProcess(void *argument) {
        InotifyWatcher *watcher = static_cast<InotifyWatcher *>(argument);
        pthread_setname_np(pthread_self(), "InotifyWatcher");
        watcher->processEvents();
        return nullptr;
    }

    /**
     * @brief The main event processing loop for the worker thread.
     *
     * This function blocks on poll(), reads events from the inotify file descriptor,
     * and dispatches them to the appropriate callbacks.
     */
    void processEvents() {
        char buffer[BUF_LEN];

        while (alive_) {
            struct pollfd pfd = {inotify_fd_, POLLIN, 0};
            int ret = poll(&pfd, 1, 50); // Use a timeout to periodically check the 'alive_' flag.

            if (ret < 0) {
                perror("poll failed");
                break;
            } else if (ret == 0) {
                continue; // Timeout, loop again.
            }

            // Read events from the inotify file descriptor.
            ssize_t length = read(inotify_fd_, buffer, BUF_LEN);
            if (length < 0) {
                perror("read");
                break;
            }

            // Process all events in the buffer.
            ssize_t i = 0;
            while (i < length) {
                struct inotify_event *event = reinterpret_cast<struct inotify_event *>(&buffer[i]);

                // Find the directory corresponding to the watch descriptor.
                auto dir_it = std::find_if(
                    directories_.begin(), directories_.end(), [event](const DirectoryWatch &dir) {
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
                auto file_it = std::find_if(
                    directory.files.begin(), directory.files.end(),
                    [&event_name](const FileWatch &file) { return file.name == event_name; });

                if (file_it != directory.files.end()) {
                    // Event is for a specific file, use its callback.
                    FileWatch &file = *file_it;
                    std::string full_path = directory.path + DIR_SEPARATOR + file.name;

                    if (file.callback_func) {
                        file.callback_func(event, full_path, file.context, file.additional_data);
                    }
                } else {
                    // Event is not for a specific file, use the general directory callback if available.
                    if (directory.callback_func) {
                        directory.callback_func(
                            event, directory.path, directory.context, directory.additional_data);
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
     *
     * This is called when the worker thread is about to exit.
     */
    void cleanup() {
        for (auto &directory : directories_) {
            inotify_rm_watch(inotify_fd_, directory.inotify_watch_fd);
        }
        // The main file descriptor is closed in the destructor to avoid race conditions.
    }

public:
    /**
     * @brief Constructs an InotifyWatcher object.
     *
     * Initializes the inotify instance.
     * @throws std::runtime_error if inotify initialization fails.
     */
    InotifyWatcher()
        : alive_(false)
        , inotify_fd_(-1) {
        inotify_fd_ = inotify_init1(O_NONBLOCK);
        if (inotify_fd_ < 0) {
            perror("inotify_init1");
            throw std::runtime_error("Failed to initialize inotify");
        }
    }

    /**
     * @brief Destroys the InotifyWatcher object.
     *
     * Stops the worker thread and closes the inotify file descriptor.
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
     * @return true on success, false on failure (e.g., invalid path, file already watched).
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
            return false; // No directory component found
        }

        std::string dir_path = path.substr(0, last_slash);
        std::string filename = path.substr(last_slash + 1);

        // Find or create directory
        auto dir_it = std::find_if(
            directories_.begin(), directories_.end(),
            [&dir_path](const DirectoryWatch &dir) { return dir.path == dir_path; });

        if (dir_it == directories_.end()) {
            DirectoryWatch new_dir;
            new_dir.path = dir_path;
            new_dir.callback_func = nullptr; // No directory callback for file watches
            new_dir.context = 0;
            new_dir.additional_data = nullptr;
            new_dir.inotify_watch_fd = -1;

            // Add inotify watch
            new_dir.inotify_watch_fd =
                inotify_add_watch(inotify_fd_, dir_path.c_str(), IN_ALL_EVENTS);

            if (new_dir.inotify_watch_fd == -1) {
                perror("inotify_add_watch");
                return false;
            }

            directories_.push_back(std::move(new_dir));
            dir_it = directories_.end() - 1;
        }

        DirectoryWatch &directory = *dir_it;

        // Check if file already exists
        auto file_exists = std::find_if(
            directory.files.begin(), directory.files.end(),
            [&filename](const FileWatch &file) { return file.name == filename; });

        if (file_exists != directory.files.end()) {
            return false; // File already being watched
        }

        // Add file watch
        FileWatch new_file;
        new_file.name = filename;
        new_file.context = reference.context;
        new_file.additional_data = reference.additional_data;
        new_file.callback_func = reference.callback_func;

        directory.files.push_back(std::move(new_file));
        return true;
    }

    /**
     * @brief Adds a watch for a directory.
     *
     * @param reference A WatchReference object containing the path, callback, and context.
     * @return true on success, false on failure (e.g., directory already has a generic watch).
     */
    bool addDirectory(const WatchReference &reference) {
        std::string path = reference.path;

        // Remove trailing slash if present
        if (!path.empty() && path.back() == DIR_SEPARATOR) {
            path.pop_back();
        }

        // Check if directory already exists
        auto dir_it = std::find_if(
            directories_.begin(), directories_.end(),
            [&path](const DirectoryWatch &dir) { return dir.path == path; });

        if (dir_it != directories_.end()) {
            // Directory exists, update callback if not set
            if (dir_it->callback_func) {
                return false; // Callback already set
            }
            dir_it->callback_func = reference.callback_func;
            dir_it->context = reference.context;
            dir_it->additional_data = reference.additional_data;
        } else {
            // Create new directory watch
            DirectoryWatch new_dir;
            new_dir.path = path;
            new_dir.callback_func = reference.callback_func;
            new_dir.context = reference.context;
            new_dir.additional_data = reference.additional_data;

            // Add inotify watch
            new_dir.inotify_watch_fd = inotify_add_watch(inotify_fd_, path.c_str(), IN_ALL_EVENTS);

            if (new_dir.inotify_watch_fd == -1) {
                perror("inotify_add_watch");
                return false;
            }

            directories_.push_back(std::move(new_dir));
        }

        return true;
    }

    /**
     * @brief Starts the event processing thread.
     *
     * The thread will begin watching for and dispatching events.
     * @return true if the thread was started successfully, false if it was already running or failed to start.
     */
    bool start() {
        if (alive_) {
            return false; // Already running
        }

        alive_ = true;
        int result = pthread_create(&thread_, nullptr, threadProcess, this);

        if (result != 0) {
            alive_ = false;
            return false;
        }

        return true;
    }

    /**
     * @brief Stops the event processing thread.
     *
     * Signals the thread to terminate and waits for it to join.
     */
    void stop() {
        if (alive_) {
            alive_ = false;
            pthread_join(thread_, nullptr);
        }
    }

    // Disable copying
    InotifyWatcher(const InotifyWatcher &) = delete;
    InotifyWatcher &operator=(const InotifyWatcher &) = delete;
};
