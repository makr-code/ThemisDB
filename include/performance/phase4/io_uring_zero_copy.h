/**
 * @file io_uring_zero_copy.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// ThemisDB - DPDK / io_uring Zero-Copy I/O Path for Network Performance
// Research basis:
//   "io_uring: a new Linux I/O interface" – Jens Axboe (KernelConf 2019)
//   "The Linux io_uring interface" – LWN (2019)
//   "High-Performance I/O with io_uring" – Cloudflare Blog (2022)
//
// Key idea: Linux io_uring (kernel 5.1+) provides a shared ring-buffer
// interface between user-space and the kernel that eliminates the syscall
// overhead of traditional epoll/read/write paths.  When combined with
// pre-registered I/O buffers (io_uring_register_buffers) and SQ-polling
// (IORING_SETUP_SQPOLL), the zero-copy send path achieves:
//   • Sub-10 µs wake latency (vs. ~50 µs for epoll)
//   • Elimination of data copies between user and kernel space
//   • 10 Gbps+ sustained throughput on commodity NICs
//
// Two backends are provided:
//   1. IoUringZeroCopyIO  – Linux io_uring backend (requires kernel ≥ 5.1)
//   2. Fallback to standard blocking/non-blocking I/O on other platforms
//      or when io_uring is unavailable.
//
// Compile-time gate: THEMIS_ENABLE_IO_URING  (set by CMake option)
// Runtime gate:      Phase4FeatureFlags::instance().io_uring_enabled()

#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace themis {
namespace performance {
namespace phase4 {

// ---------------------------------------------------------------------------
// IoUringStats – counters exposed by IoUringZeroCopyIO
// ---------------------------------------------------------------------------

/// Operational statistics for the io_uring zero-copy I/O path.
struct IoUringStats {
    uint64_t sq_entries_submitted{0};   ///< Submission queue entries enqueued
    uint64_t cq_entries_completed{0};   ///< Completion queue entries consumed
    uint64_t bytes_sent{0};             ///< Total bytes sent (zero-copy path)
    uint64_t bytes_received{0};         ///< Total bytes received
    uint64_t registered_buffers{0};     ///< Number of currently registered iovec buffers
    uint64_t fallback_sends{0};         ///< Sends that fell back to standard write()
    uint64_t fallback_recvs{0};         ///< Receives that fell back to standard read()
    bool     io_uring_available{false}; ///< false if io_uring_setup(2) is unavailable
};

// ---------------------------------------------------------------------------
// ZeroCopyBuffer – RAII buffer registered with the io_uring kernel interface
// ---------------------------------------------------------------------------

/// A contiguous memory region that is pre-registered with the io_uring
/// kernel interface via io_uring_register(IORING_REGISTER_BUFFERS).
///
/// Registered buffers allow the kernel to pin pages once at registration
/// time instead of per-I/O, eliminating DMA-map overhead per send/recv.
///
/// Not copyable (the registered mapping is tied to the backing fd).
/// Movable.
class ZeroCopyBuffer {
public:
    /// Allocate a buffer of @p size bytes aligned to the system page size.
    /// The buffer is NOT registered at construction; call register_with_ring()
    /// after associating it with an IoUringZeroCopyIO instance.
    explicit ZeroCopyBuffer(size_t size);
    ~ZeroCopyBuffer() noexcept;

    ZeroCopyBuffer(const ZeroCopyBuffer&) = delete;
    ZeroCopyBuffer& operator=(const ZeroCopyBuffer&) = delete;

    ZeroCopyBuffer(ZeroCopyBuffer&& other) noexcept;
    ZeroCopyBuffer& operator=(ZeroCopyBuffer&& other) noexcept;

    /// Pointer to the start of the buffer.
    void*  data()  const noexcept { return data_; }
    /// Usable capacity in bytes.
    size_t size()  const noexcept { return size_; }
    /// Returns true if the buffer was successfully allocated.
    bool   valid() const noexcept { return data_ != nullptr; }

private:
    void*  data_{nullptr};
    size_t size_{0};
};

// ---------------------------------------------------------------------------
// IoUringConfig – configuration for the io_uring zero-copy I/O backend
// ---------------------------------------------------------------------------

/// Configuration parameters for IoUringZeroCopyIO.
struct IoUringConfig {
    uint32_t ring_size{4096};           ///< SQ/CQ ring depth (power of 2)
    bool     sq_poll{false};            ///< Enable IORING_SETUP_SQPOLL (requires CAP_SYS_NICE)
    int      sq_thread_cpu{-1};         ///< CPU affinity for SQ poll thread (-1 = any)
    uint32_t sq_thread_idle_ms{1000};   ///< Idle timeout for SQ poll thread (ms)
    size_t   buffer_size{65536};        ///< Default send/recv buffer size (bytes)
    uint32_t num_buffers{16};           ///< Number of pre-registered I/O buffers
    bool     fixed_files{false};        ///< Register file descriptors for reduced overhead
};

// ---------------------------------------------------------------------------
// IoUringZeroCopyIO – io_uring async zero-copy I/O engine
// ---------------------------------------------------------------------------

/// Provides a zero-copy I/O path for network sockets using Linux io_uring.
///
/// On Linux (kernel ≥ 5.1):
///   - Sets up an io_uring instance with the supplied IoUringConfig.
///   - Pre-registers send/recv buffers to avoid per-I/O page pinning.
///   - Supports SQ-polling mode for polling-based submission without
///     syscalls (requires CAP_SYS_NICE and kernel ≥ 5.11).
///   - Exposes send_zerocopy() and recv_zerocopy() using registered buffers.
///
/// On non-Linux platforms or when io_uring_setup(2) fails:
///   - All operations fall back to standard send()/recv() syscalls.
///   - is_available() returns false; IoUringStats::io_uring_available is false.
///
/// Thread-safety:
///   - A single IoUringZeroCopyIO instance is NOT thread-safe for concurrent
///     send/recv from multiple threads.  Create one instance per thread or
///     protect access externally.
///
/// Typical usage:
/// @code
///   IoUringConfig cfg;
///   cfg.ring_size   = 4096;
///   cfg.sq_poll     = false;
///   cfg.num_buffers = 8;
///
///   IoUringZeroCopyIO io(cfg);
///   if (!io.is_available()) { /* fallback path */ }
///
///   int fd = /* connected socket */;
///   io.register_fd(fd);
///
///   auto& buf = io.get_buffer(0);
///   std::memcpy(buf.data(), payload, len);
///   io.send_zerocopy(fd, 0 /* buf_idx */, len);
///   io.wait_completions(1);
/// @endcode
class IoUringZeroCopyIO {
public:
    /// Initialise the io_uring instance with the given configuration.
    /// If io_uring is unavailable, the object remains valid but
    /// is_available() returns false.
    explicit IoUringZeroCopyIO(const IoUringConfig& config = IoUringConfig{});
    ~IoUringZeroCopyIO() noexcept;

    IoUringZeroCopyIO(const IoUringZeroCopyIO&) = delete;
    IoUringZeroCopyIO& operator=(const IoUringZeroCopyIO&) = delete;

    // Non-movable: the ring contains raw pointers into mmap'd regions.
    IoUringZeroCopyIO(IoUringZeroCopyIO&&) = delete;
    IoUringZeroCopyIO& operator=(IoUringZeroCopyIO&&) = delete;

    /// Returns true if io_uring is available and the ring was set up
    /// successfully.
    bool is_available() const noexcept { return available_; }

    /// Register a socket file descriptor for fixed-file operations
    /// (reduces fd-lookup overhead per SQE).  Only effective when
    /// IoUringConfig::fixed_files is true and io_uring is available.
    /// Returns true on success.
    bool register_fd(int fd) noexcept;

    /// Enqueue a zero-copy send of @p len bytes from pre-registered buffer
    /// at index @p buf_index.  The caller must have written @p len bytes into
    /// get_buffer(buf_index).data() before calling this.
    ///
    /// On io_uring: submits an IORING_OP_SEND with IORING_RECVSEND_FIXED_BUF.
    /// On fallback: calls send() immediately and increments fallback_sends.
    ///
    /// @return 0 on success (io_uring submission or fallback), -errno on error.
    int send_zerocopy(int fd, uint32_t buf_index, size_t len) noexcept;

    /// Enqueue a receive into pre-registered buffer at index @p buf_index.
    ///
    /// On io_uring: submits an IORING_OP_RECV with IORING_RECVSEND_FIXED_BUF.
    /// On fallback: calls recv() immediately and increments fallback_recvs.
    ///
    /// @return number of bytes received on fallback, 0 on io_uring queue, -errno on error.
    int recv_zerocopy(int fd, uint32_t buf_index, size_t max_len) noexcept;

    /// Block until at least @p min_completions CQEs are available and drain
    /// the completion queue.  Returns number of completions processed.
    uint32_t wait_completions(uint32_t min_completions = 1) noexcept;

    /// Return a reference to pre-registered buffer @p index.
    /// @p index must be < IoUringConfig::num_buffers.
    ZeroCopyBuffer& get_buffer(uint32_t index);
    const ZeroCopyBuffer& get_buffer(uint32_t index) const;

    /// Return a snapshot of current operational statistics.
    IoUringStats get_stats() const noexcept;

    /// Static probe: returns true if io_uring_setup(2) is available on
    /// the running kernel.  Safe to call without constructing an instance.
    static bool io_uring_accessible() noexcept;

private:
    void setup_ring(const IoUringConfig& config) noexcept;
    void teardown_ring() noexcept;
    bool register_buffers() noexcept;
    int  submit_sqes() noexcept;

    IoUringConfig config_;
    bool          available_{false};

    // Pre-registered I/O buffers
    std::vector<ZeroCopyBuffer> buffers_;

    // io_uring kernel structures (opaque on non-Linux)
    struct RingImpl;
    std::unique_ptr<RingImpl> ring_;

    // Mutex to protect ring_ initialization and access
    mutable std::mutex ring_mutex_;

    // Stats (relaxed atomics – single-threaded by contract but
    // allows safe reads from a monitoring thread)
    mutable std::atomic<uint64_t> sq_submitted_{0};
    mutable std::atomic<uint64_t> cq_completed_{0};
    mutable std::atomic<uint64_t> bytes_sent_{0};
    mutable std::atomic<uint64_t> bytes_received_{0};
    mutable std::atomic<uint64_t> fallback_sends_{0};
    mutable std::atomic<uint64_t> fallback_recvs_{0};
};

// ---------------------------------------------------------------------------
// ScopedIoUringTimer – RAII helper for measuring io_uring operation latency
// ---------------------------------------------------------------------------

/// RAII timer that records start/end timestamps (via CLOCK_MONOTONIC) and
/// writes the elapsed nanoseconds to *output_ns on destruction.
class ScopedIoUringTimer {
public:
    explicit ScopedIoUringTimer(uint64_t* output_ns) noexcept;
    ~ScopedIoUringTimer() noexcept;

    ScopedIoUringTimer(const ScopedIoUringTimer&) = delete;
    ScopedIoUringTimer& operator=(const ScopedIoUringTimer&) = delete;

private:
    uint64_t* output_ns_;
    uint64_t  start_ns_;
};

} // namespace phase4
} // namespace performance
} // namespace themis
