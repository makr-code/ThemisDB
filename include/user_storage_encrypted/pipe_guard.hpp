/**
 * @file pipe_guard.hpp
 * @brief RAII wrapper for POSIX pipes with automatic cleanup.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Status: Phase 2.1.1 Timeout & I/O Safety
 *
 * Provides safe, exception-safe pipe management with automatic close on
 * destruction. Prevents file descriptor leaks and explicit close() calls.
 */

#pragma once

#include <array>
#include <cstdint>
#include <cerrno>
#include <unistd.h>
#include <string>

namespace themis {
namespace plugins {
namespace user_storage {

/**
 * @class PipeGuard
 * @brief RAII wrapper for a pair of file descriptors (pipe).
 *
 * Manages a standard POSIX pipe pair: [0] = read end, [1] = write end.
 * Automatically closes file descriptors on destruction.
 *
 * Features:
 * - Exception-safe cleanup (noexcept destructor)
 * - Selective closing (close read, keep write, etc.)
 * - Move semantics for ownership transfer
 * - No raw pointers
 * - Error handling via errno and return values
 *
 * Usage example:
 * @code
 *   PipeGuard pipe = PipeGuard::create();
 *   if (!pipe.isValid()) {
 *       return error("pipe() failed");
 *   }
 *
 *   // Use pipe[0] for read, pipe[1] for write
 *   write(pipe[1], "data", 4);
 *   char buf[256];
 *   read(pipe[0], buf, sizeof(buf));
 *
 *   // Auto-closes on destruction (or manual: pipe.closeRead(), pipe.closeWrite())
 * @endcode
 *
 * @note Does NOT own the file descriptors initially; only after create() succeeds.
 * @note Destructor is noexcept; failed closes are logged via errno but don't throw.
 */
class PipeGuard {
public:
    /**
     * @brief Default constructor: creates an invalid (empty) pipe.
     *
     * @note Call create() to initialize, or move-assign from another PipeGuard.
     */
    PipeGuard() noexcept
        : fds_({{-1, -1}})
    {}

    /**
     * @brief Destructor: closes all open file descriptors.
     *
     * Both read and write ends are closed. If close() fails, errno is set
     * but no exception is thrown.
     *
     * @note noexcept: safe to call from other destructors
     */
    ~PipeGuard() noexcept {
        closeRead();
        closeWrite();
    }

    // Movable, not copyable
    PipeGuard(const PipeGuard&) = delete;
    PipeGuard& operator=(const PipeGuard&) = delete;

    /**
     * @brief Move constructor: takes ownership of another pipe.
     *
     * @param other Pipe to move from (will be invalidated)
     */
    PipeGuard(PipeGuard&& other) noexcept
        : fds_(std::move(other.fds_))
    {
        other.fds_[0] = -1;
        other.fds_[1] = -1;
    }

    /**
     * @brief Move assignment: takes ownership of another pipe.
     *
     * Closes any existing file descriptors before taking ownership.
     *
     * @param other Pipe to move from (will be invalidated)
     * @return Reference to this
     */
    PipeGuard& operator=(PipeGuard&& other) noexcept {
        if (this != &other) {
            closeRead();
            closeWrite();
            fds_ = std::move(other.fds_);
            other.fds_[0] = -1;
            other.fds_[1] = -1;
        }
        return *this;
    }

    /**
     * @brief Create a new pipe.
     *
     * Calls pipe() to create a pipe pair. On failure, returns an invalid
     * PipeGuard (isValid() returns false) and errno is set.
     *
     * @return PipeGuard with newly created pipe, or invalid PipeGuard on error
     */
    static PipeGuard create() noexcept {
        PipeGuard p;
        int pfd[2] = {-1, -1};
        if (::pipe(pfd) != 0) {
            // errno is already set by pipe()
            return p;
        }
        p.fds_[0] = pfd[0];
        p.fds_[1] = pfd[1];
        return p;
    }

    /**
     * @brief Check if pipe is valid (both fds open).
     *
     * @return true if read and write ends are both open (fd >= 0)
     */
    bool isValid() const noexcept {
        return fds_[0] >= 0 && fds_[1] >= 0;
    }

    /**
     * @brief Check if read end is open.
     */
    bool isReadOpen() const noexcept {
        return fds_[0] >= 0;
    }

    /**
     * @brief Check if write end is open.
     */
    bool isWriteOpen() const noexcept {
        return fds_[1] >= 0;
    }

    /**
     * @brief Get read end file descriptor.
     *
     * @return File descriptor, or -1 if already closed
     */
    int readFd() const noexcept {
        return fds_[0];
    }

    /**
     * @brief Get write end file descriptor.
     *
     * @return File descriptor, or -1 if already closed
     */
    int writeFd() const noexcept {
        return fds_[1];
    }

    /**
     * @brief Access read end by index [0].
     *
     * @return File descriptor at index 0
     */
    int operator[](size_t index) const noexcept {
        if (index >= 2) {
          return -1;
        }
        return fds_[index];
    }

    /**
     * @brief Close the read end (index 0).
     *
     * If already closed (fd == -1), this is a no-op.
     * If close() fails, errno is set and false is returned.
     *
     * @return true if close succeeded or was already closed, false on close() error
     */
    bool closeRead() noexcept {
        if (fds_[0] < 0) {
            return true;  // Already closed
        }
        int fd = fds_[0];
        fds_[0] = -1;
        return ::close(fd) == 0;
    }

    /**
     * @brief Close the write end (index 1).
     *
     * If already closed (fd == -1), this is a no-op.
     * If close() fails, errno is set and false is returned.
     *
     * @return true if close succeeded or was already closed, false on close() error
     */
    bool closeWrite() noexcept {
        if (fds_[1] < 0) {
            return true;  // Already closed
        }
        int fd = fds_[1];
        fds_[1] = -1;
        return ::close(fd) == 0;
    }

    /**
     * @brief Close both ends.
     *
     * Closes both read and write ends. Errors are only reported for the
     * first failed close (others are attempted regardless).
     *
     * @return true if both closes succeeded or were already closed
     */
    bool closeAll() noexcept {
        bool read_ok = closeRead();
        bool write_ok = closeWrite();
        return read_ok && write_ok;
    }

    /**
     * @brief Manually detach (release) an end without closing.
     *
     * Useful for passing ownership of a pipe end to another process/owner.
     *
     * @param index 0 for read end, 1 for write end
     * @return File descriptor at that index, or -1 if already closed
     *
     * @note After calling this, the caller is responsible for closing the fd
     */
    int detach(size_t index) noexcept {
        if (index >= 2) {
          return -1;
        }
        int fd = fds_[index];
        fds_[index] = -1;
        return fd;
    }

    /**
     * @brief Get remaining owner of this pipe (for debug/logging).
     *
     * @return Status string like "rw" (both), "r" (read only), "w" (write only), "-" (closed)
     */
    std::string status() const noexcept {
        std::string s = {};
        if (fds_[0] >= 0) {
          s += 'r';
        }
        if (fds_[1] >= 0) {
          s += 'w';
        }
        if (s.empty()) {
          s = '-';
        }
        return s;
    }

private:
    std::array<int, 2> fds_;
};

}  // namespace user_storage
}  // namespace plugins
}  // namespace themis
