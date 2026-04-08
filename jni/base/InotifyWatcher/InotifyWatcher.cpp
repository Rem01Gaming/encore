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

#include "InotifyWatcher.hpp"

#include <algorithm>
#include <cstring>
#include <stdexcept>

#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include <EncoreLog.hpp>

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------

InotifyWatcher::InotifyWatcher()
    : alive_(false)
    , inotify_fd_(-1)
    , stop_fd_(-1) {
    inotify_fd_ = inotify_init1(O_NONBLOCK | O_CLOEXEC);
    if (inotify_fd_ < 0) {
        LOGE_TAG("InotifyWatcher", "Failed to initialize inotify: {}", strerror(errno));
        throw std::runtime_error("Failed to initialize inotify");
    }

    stop_fd_ = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    if (stop_fd_ < 0) {
        LOGE_TAG("InotifyWatcher", "Failed to initialize eventfd for shutdown: {}", strerror(errno));
        throw std::runtime_error("Failed to initialize eventfd");
    }
}

InotifyWatcher::~InotifyWatcher() {
    stop();

    if (inotify_fd_ >= 0) {
        close(inotify_fd_);
    }

    if (stop_fd_ >= 0) {
        close(stop_fd_);
    }
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void InotifyWatcher::processEvents() {
    pthread_setname_np(pthread_self(), "InotifyWatcher");

    char buffer[BUF_LEN];

    struct pollfd pfds[2];
    pfds[0].fd = inotify_fd_;
    pfds[0].events = POLLIN;
    pfds[1].fd = stop_fd_;
    pfds[1].events = POLLIN;

    while (alive_.load(std::memory_order_acquire)) {
        // Sleep until a file changes or stop() is called.
        int ret = poll(pfds, 2, -1);

        if (ret < 0) {
            if (errno == EINTR) continue;
            LOGE_TAG("InotifyWatcher", "Poll failed: {}", strerror(errno));
            break;
        }

        // Check if we were signaled to stop
        if (pfds[1].revents & POLLIN) {
            uint64_t val;
            ssize_t rd = read(stop_fd_, &val, sizeof(val));
            (void)rd;
            break; // Exit the loop cleanly
        }

        // Check if we have filesystem events
        if (pfds[0].revents & POLLIN) {
            ssize_t length = read(inotify_fd_, buffer, BUF_LEN);
            if (length < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    continue;
                }
                LOGE_TAG("InotifyWatcher", "Read failed: {}", strerror(errno));
                break;
            }

            ssize_t i = 0;
            while (i < length) {
                struct inotify_event *event = reinterpret_cast<struct inotify_event *>(&buffer[i]);

                // Capture everything needed to fire the callback while holding the lock,
                // then release the lock before actually calling it.
                EventCallback callback_to_call;
                std::string callback_path;
                int callback_context = 0;
                void *callback_additional_data = nullptr;
                bool valid_event = false;

                {
                    std::lock_guard<std::mutex> lock(directories_mutex_);

                    auto map_it = wd_to_index_.find(event->wd);
                    if (map_it == wd_to_index_.end()) {
                        // Watch was removed but an event was already buffered
                        i += EVENT_SIZE + event->len;
                        continue;
                    }

                    DirectoryWatch &directory = directories_[map_it->second];

                    // event->name is only valid when len > 0
                    std::string event_name = (event->len > 0) ? std::string(event->name) : "";

                    // Check whether this event targets a specific tracked file
                    auto file_it =
                        std::find_if(directory.files.begin(), directory.files.end(), [&event_name](const FileWatch &file) {
                            return file.name == event_name;
                        });

                    if (file_it != directory.files.end()) {
                        FileWatch &file = *file_it;
                        callback_path = directory.path + DIR_SEPARATOR + file.name;
                        callback_to_call = file.callback_func;
                        callback_context = file.context;
                        callback_additional_data = file.additional_data;
                        valid_event = true;
                    } else if (directory.callback_func) {
                        // Not a file-specific event, use the general directory callback
                        callback_to_call = directory.callback_func;
                        callback_path = directory.path;
                        if (!event_name.empty()) {
                            callback_path += DIR_SEPARATOR + event_name;
                        }
                        callback_context = directory.context;
                        callback_additional_data = directory.additional_data;
                        valid_event = true;
                    }
                } // lock released here

                if (valid_event && callback_to_call) {
                    try {
                        callback_to_call(event, callback_path, callback_context, callback_additional_data);
                    } catch (const std::exception &e) {
                        LOGE_TAG("InotifyWatcher", "Callback exception: {}", e.what());
                    } catch (...) {
                        LOGE_TAG("InotifyWatcher", "Unknown exception in callback");
                    }
                }

                i += EVENT_SIZE + event->len;
            }
        }
    }

    cleanup();
}

void InotifyWatcher::cleanup() {
    std::lock_guard<std::mutex> lock(directories_mutex_);
    for (auto &directory : directories_) {
        inotify_rm_watch(inotify_fd_, directory.inotify_watch_fd);
    }
}

void InotifyWatcher::push_directory(DirectoryWatch &&dir) {
    size_t idx = directories_.size();
    wd_to_index_[dir.inotify_watch_fd] = idx;
    directories_.push_back(std::move(dir));
}

void InotifyWatcher::erase_directory(std::vector<DirectoryWatch>::iterator pos) {
    size_t idx = static_cast<size_t>(pos - directories_.begin());
    wd_to_index_.erase(pos->inotify_watch_fd);

    size_t last = directories_.size() - 1;
    if (idx != last) {
        // Swap the last element into the gap to avoid shifting the whole vector
        directories_[idx] = std::move(directories_[last]);
        wd_to_index_[directories_[idx].inotify_watch_fd] = idx;
    }
    directories_.pop_back();
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

bool InotifyWatcher::addFile(const WatchReference &reference) {
    std::string path = reference.path;

    if (!path.empty() && path.back() == DIR_SEPARATOR) {
        path.pop_back();
    }

    size_t last_slash = path.find_last_of(DIR_SEPARATOR);
    if (last_slash == std::string::npos) {
        LOGE_TAG("InotifyWatcher", "Invalid file path: {}", path);
        return false;
    }

    std::string dir_path = path.substr(0, last_slash);
    std::string filename = path.substr(last_slash + 1);

    if (filename.empty()) {
        LOGE_TAG("InotifyWatcher", "Empty filename in path: {}", path);
        return false;
    }

    std::lock_guard<std::mutex> lock(directories_mutex_);

    // Find or create directory watch
    auto dir_it = std::find_if(directories_.begin(), directories_.end(), [&dir_path](const DirectoryWatch &dir) {
        return dir.path == dir_path;
    });

    if (dir_it == directories_.end()) {
        DirectoryWatch new_dir;
        new_dir.path = dir_path;
        new_dir.callback_func = nullptr;
        new_dir.context = 0;
        new_dir.additional_data = nullptr;
        new_dir.inotify_watch_fd =
            inotify_add_watch(inotify_fd_, dir_path.c_str(), IN_MODIFY | IN_CLOSE_WRITE | IN_DELETE | IN_MOVE | IN_CREATE);

        if (new_dir.inotify_watch_fd == -1) {
            LOGE_TAG("InotifyWatcher", "Cannot add file watch for directory '{}': {}", dir_path, strerror(errno));
            return false;
        }

        push_directory(std::move(new_dir));
        dir_it = directories_.end() - 1;
    }

    DirectoryWatch &directory = *dir_it;

    if (std::find_if(directory.files.begin(), directory.files.end(), [&filename](const FileWatch &f) {
            return f.name == filename;
        }) != directory.files.end()) {
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

bool InotifyWatcher::addDirectory(const WatchReference &reference) {
    std::string path = reference.path;

    if (!path.empty() && path.back() == DIR_SEPARATOR) {
        path.pop_back();
    }

    std::lock_guard<std::mutex> lock(directories_mutex_);

    auto dir_it = std::find_if(directories_.begin(), directories_.end(), [&path](const DirectoryWatch &dir) {
        return dir.path == path;
    });

    if (dir_it != directories_.end()) {
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

    DirectoryWatch new_dir;
    new_dir.path = path;
    new_dir.callback_func = reference.callback_func;
    new_dir.context = reference.context;
    new_dir.additional_data = reference.additional_data;
    new_dir.inotify_watch_fd = inotify_add_watch(
        inotify_fd_, path.c_str(), IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVED_FROM | IN_MOVED_TO | IN_CLOSE_WRITE
    );

    if (new_dir.inotify_watch_fd == -1) {
        LOGE_TAG("InotifyWatcher", "Cannot add directory watch for '{}': {}", path, strerror(errno));
        return false;
    }

    push_directory(std::move(new_dir));
    LOGD_TAG("InotifyWatcher", "Added directory watch for '{}'", path);
    return true;
}

bool InotifyWatcher::start() {
    bool expected = false;
    if (!alive_.compare_exchange_strong(expected, true)) {
        LOGE_TAG("InotifyWatcher", "Already running!");
        return false;
    }

    try {
        thread_ = std::thread(&InotifyWatcher::processEvents, this);
    } catch (const std::system_error &e) {
        alive_.store(false, std::memory_order_release);
        LOGE_TAG("InotifyWatcher", "Failed to create thread: {}", e.what());
        return false;
    }

    LOGD_TAG("InotifyWatcher", "Started monitoring thread");
    return true;
}

void InotifyWatcher::stop() {
    bool expected = true;
    if (alive_.compare_exchange_strong(expected, false)) {
        // Signal the eventfd so poll() returns immediately
        if (stop_fd_ >= 0) {
            uint64_t val = 1;
            ssize_t ret = write(stop_fd_, &val, sizeof(val));
            (void)ret; // Suppress unused warning
        }

        if (thread_.joinable()) {
            thread_.join();
        }
    }
}

bool InotifyWatcher::isRunning() const {
    return alive_.load(std::memory_order_acquire);
}

bool InotifyWatcher::removeFile(const std::string &path) {
    size_t last_slash = path.find_last_of(DIR_SEPARATOR);
    if (last_slash == std::string::npos) {
        return false;
    }

    std::string dir_path = path.substr(0, last_slash);
    std::string filename = path.substr(last_slash + 1);

    std::lock_guard<std::mutex> lock(directories_mutex_);

    auto dir_it = std::find_if(directories_.begin(), directories_.end(), [&dir_path](const DirectoryWatch &dir) {
        return dir.path == dir_path;
    });

    if (dir_it == directories_.end()) {
        return false;
    }

    auto &files = dir_it->files;
    auto file_it = std::find_if(files.begin(), files.end(), [&filename](const FileWatch &file) {
        return file.name == filename;
    });

    if (file_it == files.end()) {
        return false;
    }

    files.erase(file_it);
    LOGD_TAG("InotifyWatcher", "Removed file watch for '{}'", path);

    // If the directory has no remaining watches, tear it down
    if (files.empty() && !dir_it->callback_func) {
        inotify_rm_watch(inotify_fd_, dir_it->inotify_watch_fd);
        LOGD_TAG("InotifyWatcher", "Removed empty directory watch for '{}'", dir_path);
        erase_directory(dir_it);
    }

    return true;
}

bool InotifyWatcher::removeDirectory(const std::string &path) {
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

    inotify_rm_watch(inotify_fd_, dir_it->inotify_watch_fd);
    LOGD_TAG("InotifyWatcher", "Removed directory watch for '{}'", clean_path);
    erase_directory(dir_it);
    return true;
}
