/**
 * @file wire_protocol_batch.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// ThemisDB Wire Protocol – Batch Write Processing
//
// This module implements three complementary strategies to maximise TCP
// throughput and reduce per-message overhead:
//
//   1. WireProtocolBatcher – accumulate small outbound frames in an iovec
//      list and flush them with a single writev(2) call.  Coalescing writes
//      amortises syscall overhead and lets the kernel pack more data into
//      each TCP segment.
//
//   2. NagleController – per-socket helper that applies TCP_CORK (Linux) or
//      TCP_NOPUSH (BSD/macOS) to hold segments until the MTU is filled, then
//      releases immediately when flushing.  Also exposes TCP_NODELAY for
//      ultra-low-latency modes where latency beats throughput.
//
//   3. BatchStats – lightweight statistics for observability.
//
// Performance targets (v1.7.0):
//   - Coalesced batches reduce syscall count by ~10× for small-message workloads
//   - Nagle tuning achieves < 1 ms (p99) round-trip at 100 K ops/sec

#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <BaseTsd.h>
using ssize_t = SSIZE_T;
#ifndef THEMIS_IOVEC_DEFINED
#define THEMIS_IOVEC_DEFINED
struct iovec {
    void* iov_base;
    size_t iov_len;
};
#endif
#else
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>
#endif

namespace themis {
namespace network {

// =============================================================================
// NagleController – TCP_NODELAY / TCP_CORK management
// =============================================================================

/**
 * @brief Controls Nagle's algorithm and TCP_CORK on a single socket.
 *
 * Three modes are supported:
 *   - NODELAY     : TCP_NODELAY=1, TCP_CORK=0.  Every write is sent
 *                   immediately.  Best for request-response workloads
 *                   where latency matters.
 *   - CORK        : TCP_CORK=1, TCP_NODELAY=0.  Segments are held until
 *                   the MTU is filled or cork is released.  Best for bulk
 *                   streaming.
 *   - DEFAULT     : Neither flag is set.  The kernel's Nagle algorithm
 *                   applies (200 ms coalescing window).
 *
 * Usage with batch writes:
 * @code
 *   NagleController nagle(fd);
 *   nagle.cork();               // hold data
 *   // ... schedule multiple writes ...
 *   nagle.uncork();             // flush all at once
 * @endcode
 */
class NagleController {
public:
    enum class Mode {
        DEFAULT,  ///< Kernel Nagle algorithm (default TCP behaviour)
        NODELAY,  ///< TCP_NODELAY=1: disable Nagle, send immediately
        CORK,     ///< TCP_CORK=1: hold segments until MTU or uncork()
    };

    /**
     * @brief Attach the controller to an open TCP socket.
     * @param fd  File descriptor for the socket.
     */
    explicit NagleController(int fd) noexcept;

    /**
     * @brief Apply @p mode to the socket.
     *
     * @return true on success; false if setsockopt(2) failed (errno set).
     */
    bool setMode(Mode mode) noexcept;

    /**
     * @brief Enable TCP_NODELAY (disable Nagle's algorithm).
     * Equivalent to setMode(Mode::NODELAY).
     */
    bool setNodelay() noexcept { return setMode(Mode::NODELAY); }

    /**
     * @brief Enable TCP_CORK to hold sends until uncork() or MTU fill.
     * Equivalent to setMode(Mode::CORK).
     */
    bool cork() noexcept { return setMode(Mode::CORK); }

    /**
     * @brief Release TCP_CORK, flushing any buffered data immediately.
     * Equivalent to setMode(Mode::DEFAULT).
     */
    bool uncork() noexcept;

    /// Current active mode.
    Mode currentMode() const noexcept { return mode_; }

    /// File descriptor this controller is attached to.
    int fd() const noexcept { return fd_; }

private:
    int  fd_;
    Mode mode_ = Mode::DEFAULT;
};

// =============================================================================
// BatchStats
// =============================================================================

/**
 * @brief Aggregate statistics for a WireProtocolBatcher instance.
 */
struct BatchStats {
    uint64_t messages_queued   = 0; ///< Total messages added to the batcher
    uint64_t batches_flushed   = 0; ///< Total flush() calls that sent data
    uint64_t bytes_flushed     = 0; ///< Total bytes sent via writev
    uint64_t flush_errors      = 0; ///< writev(2) failures
    uint64_t forced_flushes    = 0; ///< Flushes triggered by size/byte limits

    /// Average messages per batch (0 when no batches have been flushed).
    double avgBatchSize() const noexcept {
        return batches_flushed > 0
            ? static_cast<double>(messages_queued) / batches_flushed
            : 0.0;
    }
};

// =============================================================================
// WireProtocolBatcher
// =============================================================================

/**
 * @brief Coalesce multiple outbound frames into a single writev(2) call.
 *
 * The batcher collects (pointer, length) pairs from callers and writes them
 * all at once when:
 *   - the message count reaches `max_messages_per_batch`, or
 *   - the total byte count reaches `max_bytes_per_batch`, or
 *   - flush() is called explicitly.
 *
 * Ownership: the caller retains ownership of the data pointed to by each
 * iov entry.  The data must remain valid until flush() returns.
 *
 * Thread safety: a single batcher instance should be used from one thread
 * at a time.  Use one batcher per connection.
 *
 * Example:
 * @code
 *   WireProtocolBatcher batcher(socket_fd);
 *   batcher.add(header.data(), header.size());
 *   batcher.add(payload.data(), payload.size());
 *   // ... add more frames ...
 *   batcher.flush(); // single writev call
 * @endcode
 */
class WireProtocolBatcher {
public:
    /// Maximum number of iov entries per writev(2) call (IOV_MAX on Linux is
    /// 1024; we cap at 256 for stack-allocation safety).
    static constexpr size_t MAX_IOV = 256;

    struct Config {
        size_t max_messages_per_batch = 64;         ///< Flush after N messages
        size_t max_bytes_per_batch    = 64 * 1024;  ///< Flush after N bytes
        bool   auto_flush_on_limit    = true;        ///< Flush when limit hit inside add()
        static Config defaults() { return {}; }
    };

    /**
     * @brief Create a batcher for @p fd.
     * @param fd    Open, writable file descriptor (TCP socket).
     * @param cfg   Tunable configuration.
     */
    explicit WireProtocolBatcher(int fd, const Config& cfg = Config::defaults());

    /**
     * @brief Queue a data region for inclusion in the next batch.
     *
     * If `auto_flush_on_limit` is true and the limits are exceeded, the
     * current batch is flushed before adding the new entry.
     *
     * @param data  Pointer to the bytes to include.  Must remain valid until
     *              the next flush() call.
     * @param size  Number of bytes.
     * @return false if the iov list is full (MAX_IOV entries) and auto-flush
     *              failed; true otherwise.
     */
    bool add(const void* data, size_t size);

    /**
     * @brief Flush all pending entries with a single writev(2) call.
     *
     * Clears the iov list on return regardless of success.
     *
     * @return Total bytes written, or -1 on writev error (errno is set).
     */
    ssize_t flush();

    /**
     * @brief Return true if there is at least one pending entry.
     */
    bool pending() const noexcept { return iov_count_ > 0; }

    /**
     * @brief Number of pending iov entries (messages).
     */
    size_t pendingCount() const noexcept { return iov_count_; }

    /**
     * @brief Total pending bytes across all iov entries.
     */
    size_t pendingBytes() const noexcept { return pending_bytes_; }

    /// Read-only statistics snapshot.
    const BatchStats& stats() const noexcept { return stats_; }

    /// File descriptor this batcher is attached to.
    int fd() const noexcept { return fd_; }

private:
    struct BatchIOVec {
        void*  iov_base;
        size_t iov_len;
    };

    int    fd_;
    Config cfg_;

    BatchIOVec iov_[MAX_IOV];
    size_t     iov_count_    = 0;
    size_t       pending_bytes_ = 0;

    BatchStats stats_;
};

} // namespace network
} // namespace themis
