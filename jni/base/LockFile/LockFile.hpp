#pragma once

#include <string>

/**
 * @brief POSIX advisory lock management via fcntl.
 * @note This class is not thread-safe, note that POSIX locks are process-wide.
 */
class LockFile {
public:
    enum class AcquireMode { Blocking, NonBlocking };
    enum class LockType { Exclusive, Shared };



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



    int internal_open() const noexcept;
    bool ensure_open() noexcept;
    void close_fd() noexcept;

    static struct flock make_flock(int l_type) noexcept;
};
