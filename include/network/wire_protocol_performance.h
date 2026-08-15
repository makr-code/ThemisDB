/**
 * @file wire_protocol_performance.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// ThemisDB Wire Protocol v1 Performance – Metrics & Buffer Pool
//
// Phase 2 additions:
//   • WireProtocolMetrics  – latency histograms, throughput, error-rate tracking
//   • PayloadBufferPool    – fixed-size slab allocator for payload reuse
//   • CompressionAdvisor   – selects LZ4 compression level based on payload size
//
// These components are wired into WireProtocolServer (server-side) and
// WireProtocolConnectionPool (client-side).  They are optional helpers;
// existing code continues to work without them.

#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <vector>

namespace themis {
namespace network {

// =============================================================================
// WireProtocolMetrics – latency / throughput / error statistics
// =============================================================================

/**
 * @brief Thread-safe metrics collector for the wire protocol.
 *
 * Call `recordLatency()` on every completed request, `recordBytes()` for each
 * transferred payload, and `recordError()` on failures.  The rolling window is
 * bounded: older samples are dropped once `max_samples` is reached.
 *
 * Typical usage:
 * @code
 *   WireProtocolMetrics metrics;
 *   // inside session handler:
 *   auto t0 = std::chrono::steady_clock::now();
 *   // … handle request …
 *   metrics.recordLatency(t0);
 *   metrics.recordBytes(bytes_in, bytes_out);
 *
 *   // periodic reporting:
 *   auto snap = metrics.snapshot();
 *   logger->info("p99={} ms, rps={}", snap.latency.p99_ms, snap.throughput.requests_per_second);
 * @endcode
 */
class WireProtocolMetrics {
public:
    // ── Latency percentile snapshot ─────────────────────────────────────
    struct LatencyStats {
        double p50_ms  = 0.0;
        double p75_ms  = 0.0;
        double p95_ms  = 0.0;
        double p99_ms  = 0.0;
        double p999_ms = 0.0;
        double max_ms  = 0.0;
        double mean_ms = 0.0;
        uint64_t sample_count = 0;
    };

    // ── Throughput snapshot ──────────────────────────────────────────────
    struct ThroughputStats {
        uint64_t bytes_received_total   = 0;
        uint64_t bytes_sent_total       = 0;
        uint64_t requests_total         = 0;
        uint64_t compressed_payloads    = 0;
        double   compression_ratio      = 1.0; ///< < 1 means net saving
    };

    // ── Error snapshot ───────────────────────────────────────────────────
    struct ErrorStats {
        uint64_t connection_errors = 0;
        uint64_t timeout_errors    = 0;
        uint64_t parse_errors      = 0;
        uint64_t auth_errors       = 0;
        double   error_rate        = 0.0; ///< errors / (requests + errors)
    };

    // ── Full snapshot (all three categories + histogram) ─────────────────
    struct Snapshot {
        LatencyStats    latency;
        ThroughputStats throughput;
        ErrorStats      errors;
        std::map<uint64_t /*bucket_ms*/, uint64_t /*count*/> latency_histogram;
        std::chrono::steady_clock::time_point captured_at;
    };

    // ── Configuration ────────────────────────────────────────────────────
    struct Config {
        size_t max_samples = 10'000; ///< Sliding-window size for latency samples
        static Config defaults() { return {}; }
    };

    explicit WireProtocolMetrics(const Config& cfg = Config::defaults());

    // ── Record helpers ────────────────────────────────────────────────────

    /**
     * @brief Record a completed request.
     * @param started  Time-point when request handling began.
     */
    void recordLatency(std::chrono::steady_clock::time_point started);

    /**
     * @brief Record raw latency in milliseconds.
     */
    void recordLatencyMs(double ms);

    /**
     * @brief Accumulate byte counters.
     * @param received  Bytes read from the wire (payload).
     * @param sent      Bytes written to the wire (response payload).
     */
    void recordBytes(uint64_t received, uint64_t sent);

    /**
     * @brief Record a compressed payload.
     * @param original_bytes  Size before compression.
     * @param compressed_bytes  Size after compression.
     */
    void recordCompression(uint64_t original_bytes, uint64_t compressed_bytes);

    /**
     * @brief Increment an error counter.
     * @param kind  One of "connection", "timeout", "parse", "auth".
     */
    void recordError(const char* kind);

    // ── Snapshot ──────────────────────────────────────────────────────────

    /**
     * @brief Compute a consistent point-in-time snapshot.
     *
     * Latency percentiles are computed from the current sample window.
     * This involves a sort of the window copy, so it is O(n log n) where
     * n ≤ max_samples.
     */
    Snapshot snapshot() const;

    /**
     * @brief Reset all counters and the sample window.
     */
    void reset();

    // ── Individual getters (lock-free for hot paths) ──────────────────────
    uint64_t totalRequests()  const noexcept {
        return requests_total_.load(std::memory_order_relaxed);
    }
    uint64_t totalErrors() const noexcept {
        return errors_total_.load(std::memory_order_relaxed);
    }

private:
    Config cfg_;

    // Latency sample ring-buffer (protected by latency_mutex_)
    mutable std::mutex            latency_mutex_;
    std::vector<double>           latency_samples_; ///< milliseconds
    size_t                        write_pos_ = 0;
    bool                          buffer_full_ = false;

    // Throughput (atomic for hot-path updates)
    std::atomic<uint64_t> bytes_received_total_{0};
    std::atomic<uint64_t> bytes_sent_total_{0};
    std::atomic<uint64_t> requests_total_{0};
    std::atomic<uint64_t> compressed_payloads_{0};
    std::atomic<uint64_t> original_bytes_total_{0};
    std::atomic<uint64_t> compressed_bytes_total_{0};

    // Errors (atomic)
    std::atomic<uint64_t> errors_total_{0};
    std::atomic<uint64_t> connection_errors_{0};
    std::atomic<uint64_t> timeout_errors_{0};
    std::atomic<uint64_t> parse_errors_{0};
    std::atomic<uint64_t> auth_errors_{0};

    // Compute percentile from a sorted vector
    static double percentile(const std::vector<double>& sorted, double p) noexcept;
};

// =============================================================================
// PayloadBufferPool – slab allocator for wire-protocol payload buffers
// =============================================================================

/**
 * @brief Lock-based slab-allocator that reuses `std::vector<uint8_t>` payloads.
 *
 * Allocating a fresh `std::vector` per request is ~2 µs due to heap allocation.
 * Reusing a pre-warmed slab eliminates that cost for the common case where
 * payload sizes cluster around a known maximum (configured via `slab_size`).
 *
 * Usage:
 * @code
 *   PayloadBufferPool pool(4096, 64); // 64 slabs of 4 KiB each
 *
 *   {
 *       auto buf = pool.acquire();  // checked-out buffer
 *       buf->resize(payload_size);
 *       // … fill and send buf … //
 *   } // destructor returns buf to pool automatically
 * @endcode
 */
class PayloadBufferPool {
public:
    using Buffer = std::vector<uint8_t>;

    /**
     * @brief RAII handle that returns the buffer to the pool on destruction.
     */
    class Handle {
    public:
        Handle() = default;
        Handle(Handle&&) noexcept;
        Handle& operator=(Handle&&) noexcept;
        ~Handle();

        Buffer* operator->() noexcept { return buf_.get(); }
        Buffer& operator*()  noexcept { return *buf_;       }
        const Buffer* operator->() const noexcept { return buf_.get(); }
        const Buffer& operator*()  const noexcept { return *buf_;      }

        explicit operator bool() const noexcept { return buf_ != nullptr; }

        // Return ownership back to pool (called by destructor)
        void release() noexcept;

    private:
        friend class PayloadBufferPool;
        Handle(std::unique_ptr<Buffer> buf, PayloadBufferPool* pool) noexcept;

        std::unique_ptr<Buffer> buf_;
        PayloadBufferPool*      pool_ = nullptr;
    };

    /**
     * @brief Construct a buffer pool.
     * @param slab_size    Reserved capacity for each slab buffer (bytes).
     * @param pool_depth   Maximum number of idle slabs kept in the pool.
     */
    explicit PayloadBufferPool(size_t slab_size = 64 * 1024,
                               size_t pool_depth = 128);

    ~PayloadBufferPool();

    /**
     * @brief Acquire a buffer.
     *
     * Returns a pooled slab if one is available, or allocates a new one.
     * The caller may resize the buffer to any size; it is reset to
     * `slab_size` capacity (but zero length) before being returned to pool.
     *
     * @return RAII handle owning the buffer.
     */
    Handle acquire();

    // ── Statistics ────────────────────────────────────────────────────────
    size_t poolDepth()   const noexcept;   ///< Current idle slab count
    size_t slabSize()    const noexcept;   ///< Configured slab capacity
    uint64_t hitCount()  const noexcept;   ///< Acquisitions from pool
    uint64_t missCount() const noexcept;   ///< Fresh allocations
    double   hitRate()   const noexcept;   ///< hits / (hits + misses)

private:
    friend class Handle;
    void returnBuffer(std::unique_ptr<Buffer> buf) noexcept;

    const size_t slab_size_;
    const size_t pool_depth_;

    // R16: Use timed_mutex to support try_lock_for() timeout enforcement
    mutable std::timed_mutex                 pool_mutex_;
    std::vector<std::unique_ptr<Buffer>>    idle_slabs_;

    std::atomic<uint64_t> hit_count_{0};
    std::atomic<uint64_t> miss_count_{0};
};

// =============================================================================
// CompressionAdvisor – select LZ4 compression level for wire payloads
// =============================================================================

/**
 * @brief Decides whether (and how) to compress a wire-protocol payload.
 *
 * The advisor implements an adaptive policy:
 * - Skip compression for small payloads (overhead exceeds benefit).
 * - Use fast LZ4 for medium payloads (speed > ratio).
 * - Use LZ4-HC for large payloads (ratio > speed).
 *
 * The thresholds and acceleration factors are tunable.
 */
class CompressionAdvisor {
public:
    enum class Decision {
        SKIP,       ///< Do not compress (payload too small or incompressible)
        LZ4_FAST,   ///< LZ4 with default acceleration (1)
        LZ4_FAST_X, ///< LZ4 with high acceleration (3) – ultra-low latency
        LZ4_HC,     ///< LZ4-HC (high compression) for large payloads
    };

    struct Config {
        size_t min_compressible_bytes  = 512;    ///< Below this: always SKIP
        size_t lz4_fast_threshold      = 64 * 1024; ///< Above this: LZ4_HC
        int    lz4_fast_acceleration   = 1;      ///< LZ4_compress_fast accel
        int    lz4_fast_x_acceleration = 3;      ///< used when latency is critical
        int    lz4_hc_level            = 4;      ///< LZ4HC_CLEVEL_DEFAULT
        bool   prefer_speed            = false;  ///< Prefer LZ4_FAST_X over LZ4_HC
        static Config defaults() { return {}; }
    };

    explicit CompressionAdvisor(const Config& cfg = Config::defaults());

    /**
     * @brief Advise on compression for a payload of @p size bytes.
     */
    Decision advise(size_t payload_size) const noexcept;

    /**
     * @brief Return the LZ4 acceleration parameter for a given decision.
     * @return 0 for HC decisions (use lz4_hc_level instead), else acceleration int.
     */
    int lz4Acceleration(Decision d) const noexcept;

    /**
     * @brief Return the LZ4-HC compression level for HC decisions.
     */
    int lz4HcLevel() const noexcept { return cfg_.lz4_hc_level; }

    const Config& config() const noexcept { return cfg_; }

private:
    Config cfg_;
};

} // namespace network
} // namespace themis
