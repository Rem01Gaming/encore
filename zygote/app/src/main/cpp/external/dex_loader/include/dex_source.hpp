#pragma once

#include <cstdint>
#include <cerrno>
#include <fcntl.h>
#include <string>
#include <unistd.h>
#include <utility>
#include <vector>

namespace dexloader {

    inline ssize_t ReadNoIntr(int fd, void* buffer, size_t count) {
        ssize_t result;
        do {
            result = ::read(fd, buffer, count);
        } while (result == -1 && errno == EINTR);
        return result;
    }

    inline bool ReadFileBytes(const char* path, std::vector<uint8_t>& out) {
        out.clear();

        if (path == nullptr) {
            return false;
        }

        const int fd = ::open(path, O_RDONLY | O_CLOEXEC);
        if (fd < 0) {
            return false;
        }

        std::vector<uint8_t> buffer(65536);
        bool ok = true;

        for (;;) {
            const ssize_t bytes = ReadNoIntr(fd, buffer.data(), buffer.size());

            if (bytes < 0) {
                ok = false;
                break;
            }

            if (bytes == 0) {
                break;
            }

            out.insert(out.end(), buffer.begin(), buffer.begin() + bytes);
        }

        ::close(fd);

        if (!ok) {
            out.clear();
        }

        return ok && !out.empty();
    }

/**
 * Interface for anything that can provide DEX bytes.
 */
    class DexSource {
    public:
        virtual ~DexSource() = default;

        virtual bool Load(std::vector<uint8_t>& out, std::string& error) const = 0;
    };

/**
 * Reads DEX bytes from a file path.
 */
    class FileDexSource final : public DexSource {
    public:
        explicit FileDexSource(std::string path) : path_(std::move(path)) {}

        bool Load(std::vector<uint8_t>& out, std::string& error) const override {
            if (!ReadFileBytes(path_.c_str(), out)) {
                error = "Failed to read DEX file: " + path_;
                return false;
            }

            return true;
        }

    private:
        std::string path_;
    };

/**
 * Uses already-loaded DEX bytes.
 */
    class MemoryDexSource final : public DexSource {
    public:
        explicit MemoryDexSource(std::vector<uint8_t> dexBytes)
                : dexBytes_(std::move(dexBytes)) {}

        bool Load(std::vector<uint8_t>& out, std::string& error) const override {
            if (dexBytes_.empty()) {
                error = "Memory DEX source is empty";
                return false;
            }

            out = dexBytes_;
            return true;
        }

    private:
        std::vector<uint8_t> dexBytes_;
    };

} // namespace dexloader
