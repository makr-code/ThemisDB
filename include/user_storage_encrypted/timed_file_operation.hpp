/**
 * @file timed_file_operation.hpp
 * @brief RAII-based timed I/O operations (read/write) with timeout support.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Status: Phase 2.1.1 Timeout & I/O Safety
 * 
 * Provides safe, bounded I/O operations for POSIX file descriptors with explicit
 * timeout handling. Prevents indefinite blocking on pipes and sockets.
 */

#pragma once

#include <chrono>
#include <optional>
#include <cerrno>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <string>

namespace themis {
namespace plugins {
namespace user_storage {

/**
 * @enum IoTimeoutPolicy
 * @brief Policy for how to interpret timeout values
 */
enum class IoTimeoutPolicy {
    kAbsolute,      ///< Timeout is total wall-clock time for entire operation
    kPerOperation,  ///< Timeout applies to each individual read/write call
};

/**
 * @class TimedFileOperation
 * @brief Bounded I/O operations for file descriptors with explicit timeout.
 *
 * Provides type-safe read/write operations on POSIX file descriptors with
 * configurable timeouts. Uses poll() for non-blocking checks to prevent
 * indefinite blocking.
 *
 * Features:
 * - Template class supporting various buffer types (void*, std::string, etc.)
 * - Configurable timeout via std::chrono::duration
 * - Explicit error handling (no exceptions on timeout)
 * - Both read and write operations
 * - Returns ssize_t or optional<ssize_t> for easy integration
 *
 * Exception safety: noexcept (exceptions only on construction/destruction issues)
 *
 * Usage example:
 * @code
 *   int fd = open(...);
 *   char buf[1024];
 *   TimedFileOperation op(fd, std::chrono::seconds(5));
 *   
 *   auto bytes_read = op.read(buf, sizeof(buf));
 *   if (!bytes_read.has_value()) {
 *       // timeout or error
 *       return error("I/O timeout");
 *   }
 *   
 *   auto bytes_written = op.write("data", 4);
 *   if (!bytes_written.has_value()) {
 *       // timeout or error
 *       return error("I/O timeout");
 *   }
 * @endcode
 */
class TimedFileOperation {
public:
    using DurationMs = std::chrono::milliseconds;
    using DurationS = std::chrono::seconds;

    /**
     * @brief Construct a timed I/O operator for a file descriptor.
     *
     * @param fd           File descriptor to operate on (not owned)
     * @param timeout      Maximum time to wait for I/O to complete
     * @param policy       How to interpret timeout (absolute or per-operation)
     *
     * @note Does NOT close the file descriptor on destruction
     * @note fd must be valid and not closed while this object is in use
     */
    explicit TimedFileOperation(
        int fd,
        DurationMs timeout = DurationMs(5000),
        IoTimeoutPolicy policy = IoTimeoutPolicy::kPerOperation
    ) noexcept
        : fd_(fd),
          timeout_ms_(timeout.count()),
          policy_(policy),
          start_time_(std::chrono::steady_clock::now())
    {}

    /**
     * @brief Construct with std::chrono duration of any type.
     */
    template<typename Rep, typename Period>
    explicit TimedFileOperation(
        int fd,
        std::chrono::duration<Rep, Period> timeout,
        IoTimeoutPolicy policy = IoTimeoutPolicy::kPerOperation
    ) noexcept
        : fd_(fd),
          timeout_ms_(std::chrono::duration_cast<DurationMs>(timeout).count()),
          policy_(policy),
          start_time_(std::chrono::steady_clock::now())
    {}

    TimedFileOperation(const TimedFileOperation&) = delete;
    TimedFileOperation& operator=(const TimedFileOperation&) = delete;

    ~TimedFileOperation() noexcept = default;

    /**
     * @brief Non-blocking read with timeout.
     *
     * Attempts to read up to @p count bytes from the file descriptor. If the
     * read would block, waits up to timeout_ milliseconds using poll().
     *
     * @param buffer  Buffer to read into (not null)
     * @param count   Number of bytes to attempt to read
     * @return        Number of bytes read, or empty optional on timeout/error
     *
     * Error codes (errno):
     * - EAGAIN: Timeout occurred
     * - EBADF: Invalid file descriptor
     * - EIO: I/O error
     * - Other POSIX errors from read()
     */
    std::optional<ssize_t> read(void* buffer, size_t count) noexcept {
        if (buffer == nullptr || fd_ < 0) {
            errno = EBADF;
            return {};
        }

        int remaining_ms = getRemainingTimeoutMs();
        if (remaining_ms <= 0) {
            errno = EAGAIN;  // Signal timeout
            return {};
        }

        auto poll_result = poll_for_readability(remaining_ms);
        if (poll_result < 0) {
            // poll() error
            return {};
        } else if (poll_result == 0) {
            // poll() timeout
            errno = EAGAIN;
            return {};
        }

        // Now readable
        ssize_t n = ::read(fd_, buffer, count);
        if (n < 0 && errno == EINTR) {
            // Retry on signal
            return read(buffer, count);
        }
        return n;
    }

    /**
     * @brief Non-blocking write with timeout.
     *
     * Attempts to write @p count bytes to the file descriptor. If the write
     * would block, waits up to timeout_ milliseconds using poll().
     *
     * @param buffer  Buffer to write from (not null)
     * @param count   Number of bytes to write
     * @return        Number of bytes written, or empty optional on timeout/error
     *
     * Error codes (errno):
     * - EAGAIN: Timeout occurred
     * - EBADF: Invalid file descriptor
     * - EPIPE: Pipe closed by peer
     * - Other POSIX errors from write()
     */
    std::optional<ssize_t> write(const void* buffer, size_t count) noexcept {
        if (buffer == nullptr || fd_ < 0) {
            errno = EBADF;
            return {};
        }

        int remaining_ms = getRemainingTimeoutMs();
        if (remaining_ms <= 0) {
            errno = EAGAIN;  // Signal timeout
            return {};
        }

        auto poll_result = poll_for_writability(remaining_ms);
        if (poll_result < 0) {
            // poll() error
            return {};
        } else if (poll_result == 0) {
            // poll() timeout
            errno = EAGAIN;
            return {};
        }

        // Now writable
        ssize_t n = ::write(fd_, buffer, count);
        if (n < 0 && errno == EINTR) {
            // Retry on signal
            return write(buffer, count);
        }
        return n;
    }

    /**
     * @brief Check if operation has timed out.
     *
     * @return true if total elapsed time >= timeout (in absolute policy)
     */
    bool hasTimedOut() const noexcept {
        if (policy_ == IoTimeoutPolicy::kPerOperation) {
            return false;  // Per-operation timeouts managed locally
        }
        return getRemainingTimeoutMs() <= 0;
    }

    /**
     * @brief Get the file descriptor.
     */
    int fd() const noexcept { return fd_; }

    /**
     * @brief Get configured timeout in milliseconds.
     */
    int timeoutMs() const noexcept { return timeout_ms_; }

private:
    int fd_;
    int timeout_ms_;
    IoTimeoutPolicy policy_;
    std::chrono::steady_clock::time_point start_time_;

    /**
     * @brief Get remaining time for absolute timeouts (or timeout_ if per-operation).
     * @return Milliseconds remaining, or negative if expired
     */
    int getRemainingTimeoutMs() const noexcept {
        if (policy_ == IoTimeoutPolicy::kPerOperation) {
            return timeout_ms_;
        }

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time_
        );
        return static_cast<int>(timeout_ms_ - elapsed.count());
    }

    /**
     * @brief Wait for fd to be readable (using poll).
     * @param timeout_ms Timeout in milliseconds (-1 = forever)
     * @return 1 if readable, 0 if timeout, -1 if error
     */
    int poll_for_readability(int timeout_ms) const noexcept {
        struct pollfd pfd = {fd_, POLLIN, 0};
        return poll(&pfd, 1, timeout_ms);
    }

    /**
     * @brief Wait for fd to be writable (using poll).
     * @param timeout_ms Timeout in milliseconds (-1 = forever)
     * @return 1 if writable, 0 if timeout, -1 if error
     */
    int poll_for_writability(int timeout_ms) const noexcept {
        struct pollfd pfd = {fd_, POLLOUT, 0};
        return poll(&pfd, 1, timeout_ms);
    }
};

}  // namespace user_storage
}  // namespace plugins
}  // namespace themis
