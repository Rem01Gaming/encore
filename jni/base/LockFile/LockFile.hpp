#pragma once

#include <atomic>
#include <functional>
#include <future>
#include <string>
#include <thread>

#ifndef LOCKFILE_CANCEL_SIGNAL
    #include <signal.h>
    #define LOCKFILE_CANCEL_SIGNAL SIGUSR1
#endif

/**
 * @brief POSIX advisory lock management via fcntl.
 * @note This class is not thread-safe, note that POSIX locks are process-wide.
 */
class LockFile {
public:
    enum class AcquireMode { Blocking, NonBlocking };
    enum class LockType { Exclusive, Shared };

    /**
     * @brief Callback for watch()
     * @param lock_became_free True if the lock was released by another process.
     */
    using WatchCallback = std::function<void(bool lock_became_free)>;

    explicit LockFile(std::string path);
    ~LockFile();

    // Prevent copying
    LockFile(const LockFile &) = delete;
    LockFile &operator=(const LockFile &) = delete;

    // Support moving
    LockFile(LockFile &&o) noexcept;
    LockFile &operator=(LockFile &&o) noexcept;

    /**
     * @brief Acquire the lock.
     * @return true if acquired, false otherwise.
     */
    bool acquire(AcquireMode mode = AcquireMode::Blocking, LockType type = LockType::Exclusive);

    /**
     * @brief Release the lock.
     * @return true if released, false if not held.
     */
    bool release();

    /**
     * @brief Checks if the file is currently locked by any process.
     */
    bool is_locked() const;

    /**
     * @brief Monitors the file and triggers the callback when the lock is released.
     */
    void watch(WatchCallback callback);

    /**
     * @brief Stops an active watcher thread.
     */
    void unwatch();

    const std::string &path() const noexcept {
        return path_;
    }
    bool holds_lock() const noexcept {
        return fd_ != -1 && locked_;
    }

private:
    std::string path_;
    int fd_ = -1;
    bool locked_ = false;

    // Watcher thread state
    std::thread watcher_;
    std::atomic<bool> stop_watch_{false};
    std::promise<pthread_t> tid_promise_;
    std::future<pthread_t> tid_future_;

    int internal_open() const noexcept;
    bool ensure_open() noexcept;
    void close_fd() noexcept;

    static struct flock make_flock(int l_type) noexcept;
};
