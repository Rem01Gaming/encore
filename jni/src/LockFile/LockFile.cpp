#include "LockFile.hpp"

#include <cerrno>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <utility>

namespace {

/** Helper to convert class enums to fcntl types */
int fcntl_type_for(LockFile::LockType type) noexcept {
    return (type == LockFile::LockType::Exclusive) ? F_WRLCK : F_RDLCK;
}

/** No-op signal handler to allow fcntl to return EINTR */
void install_cancel_signal_handler() {
    struct sigaction sa{};
    sa.sa_handler = [](int) {};
    sigemptyset(&sa.sa_mask);
    sigaction(LOCKFILE_CANCEL_SIGNAL, &sa, nullptr);
}

} // namespace

struct flock LockFile::make_flock(int l_type) noexcept {
    struct flock fl{};
    fl.l_type = static_cast<short>(l_type);
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0; // Lock entire file
    return fl;
}

LockFile::LockFile(std::string path)
    : path_(std::move(path))
    , tid_future_(tid_promise_.get_future()) {
}

LockFile::~LockFile() {
    unwatch();
    release();
    close_fd();
}

LockFile::LockFile(LockFile &&o) noexcept
    : path_(std::move(o.path_))
    , fd_(std::exchange(o.fd_, -1))
    , locked_(std::exchange(o.locked_, false)) {
    // Note: We do not move the watcher thread. The moved-from object
    // must be allowed to join its own thread during destruction.
}

LockFile &LockFile::operator=(LockFile &&o) noexcept {
    if (this != &o) {
        unwatch();
        release();
        close_fd();

        path_ = std::move(o.path_);
        fd_ = std::exchange(o.fd_, -1);
        locked_ = std::exchange(o.locked_, false);
    }
    return *this;
}

int LockFile::internal_open() const noexcept {
    return ::open(path_.c_str(), O_CREAT | O_RDWR | O_CLOEXEC, 0600);
}

bool LockFile::ensure_open() noexcept {
    if (fd_ != -1) return true;
    fd_ = internal_open();
    return fd_ != -1;
}

void LockFile::close_fd() noexcept {
    if (fd_ != -1) {
        ::close(fd_);
        fd_ = -1;
    }
}

bool LockFile::acquire(AcquireMode mode, LockType type) {
    if (!ensure_open()) return false;

    struct flock fl = make_flock(fcntl_type_for(type));
    const int cmd = (mode == AcquireMode::Blocking) ? F_SETLKW : F_SETLK;

    while (::fcntl(fd_, cmd, &fl) != 0) {
        if (errno == EINTR) continue;
        return false;
    }

    locked_ = true;
    return true;
}

bool LockFile::release() {
    if (!locked_ || fd_ == -1) return false;

    struct flock fl = make_flock(F_UNLCK);
    while (::fcntl(fd_, F_SETLK, &fl) != 0) {
        if (errno != EINTR) return false;
    }

    locked_ = false;
    return true;
}

bool LockFile::is_locked() const {
    if (locked_) return true;

    // Use existing FD if open, otherwise open a temporary probe
    int probe = (fd_ != -1) ? fd_ : internal_open();
    if (probe == -1) return false;

    struct flock fl = make_flock(F_WRLCK);
    int ret = ::fcntl(probe, F_GETLK, &fl);

    if (probe != fd_) ::close(probe);

    return (ret == 0) && (fl.l_type != F_UNLCK);
}

void LockFile::watch(WatchCallback callback) {
    unwatch();

    static std::once_flag once;
    std::call_once(once, install_cancel_signal_handler);

    stop_watch_.store(false, std::memory_order_release);
    tid_promise_ = std::promise<pthread_t>{};
    tid_future_ = tid_promise_.get_future();

    watcher_ = std::thread([this, cb = std::move(callback)] {
        pthread_setname_np(pthread_self(), "LockFileWatcher");

        tid_promise_.set_value(pthread_self());

        int probe = internal_open();
        if (probe == -1) {
            cb(false);
            return;
        }

        struct flock fl = make_flock(F_WRLCK);
        bool became_free = false;

        while (true) {
            if (::fcntl(probe, F_SETLKW, &fl) == 0) {
                // Instantly release: we just wanted to know if we COULD lock it
                struct flock un = make_flock(F_UNLCK);
                ::fcntl(probe, F_SETLK, &un);
                became_free = true;
                break;
            }

            if (errno == EINTR) {
                if (stop_watch_.load(std::memory_order_acquire)) break;
                continue;
            }
            break;
        }

        ::close(probe);
        cb(became_free);
    });
}

void LockFile::unwatch() {
    if (!watcher_.joinable()) return;

    stop_watch_.store(true, std::memory_order_release);

    // Get the thread ID and interrupt the fcntl syscall
    if (tid_future_.valid()) {
        pthread_t tid = tid_future_.get();
        pthread_kill(tid, LOCKFILE_CANCEL_SIGNAL);
    }

    watcher_.join();
}
