// ThemisDB Network QoS Manager
// Token Bucket rate limiting, Priority Queues, and Backpressure control
// Q1 2026 – see docs/network_roadmap.md

#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace themis {
namespace network {

// =============================================================================
// Priority levels for traffic classification
// =============================================================================

/**
 * @brief Traffic priority levels for QoS scheduling.
 *
 * CRITICAL  – Interactive / low-latency queries (served first)
 * HIGH      – Transactional OLTP operations
 * MEDIUM    – Analytical OLAP queries
 * LOW       – Batch operations, backups, replication (served last)
 */
enum class Priority : uint8_t {
    CRITICAL = 0,
    HIGH     = 1,
    MEDIUM   = 2,
    LOW      = 3,
};

// =============================================================================
// TokenBucket – per-connection token bucket rate limiter
// =============================================================================

/**
 * @brief Token bucket rate limiter.
 *
 * Tokens refill at `rate_bps` bits per second up to a maximum of `burst_bytes`
 * bytes.  A call to `consume(bytes)` returns immediately if enough tokens are
 * available, otherwise it blocks or (optionally) returns false.
 *
 * Thread-safe via an internal mutex.
 *
 * Algorithm:
 *   tokens += elapsed_seconds * rate_bps / 8   (byte-level refill)
 *   tokens  = min(tokens, burst_bytes)
 */
class TokenBucket {
public:
    /**
     * @param rate_bps   Sustained rate in bits per second.
     * @param burst_bytes Maximum burst size in bytes.
     */
    TokenBucket(uint64_t rate_bps, uint64_t burst_bytes);

    /**
     * @brief Attempt to consume `bytes` from the bucket without blocking.
     * @return true if tokens were available, false if not enough tokens.
     */
    bool tryConsume(uint64_t bytes);

    /**
     * @brief Block until `bytes` tokens are available, then consume them.
     *
     * The call sleeps in short intervals; the caller may provide a deadline
     * after which the function returns false without consuming.
     *
     * @param bytes   Number of bytes to consume.
     * @param timeout Maximum time to wait.
     * @return true if tokens consumed before timeout, false otherwise.
     */
    bool consume(uint64_t bytes,
                 std::chrono::milliseconds timeout = std::chrono::milliseconds(1000));

    /**
     * @brief Update the token bucket parameters at runtime.
     * @param rate_bps   New sustained rate in bits per second.
     * @param burst_bytes New maximum burst size in bytes.
     */
    void reconfigure(uint64_t rate_bps, uint64_t burst_bytes);

    /** @brief Current available tokens (bytes). */
    double availableBytes() const;

    /** @brief Configured rate in bps. */
    uint64_t rateBps() const;

    /** @brief Configured burst size in bytes. */
    uint64_t burstBytes() const;

private:
    /** Refill tokens based on elapsed time since last refill. */
    void refill();

    mutable std::mutex mutex_;
    double tokens_;          ///< Available tokens (bytes, fractional allowed)
    uint64_t rate_bps_;      ///< Bits per second sustained rate
    uint64_t burst_bytes_;   ///< Maximum burst size in bytes
    std::chrono::steady_clock::time_point last_refill_;
};

// =============================================================================
// QoSManager – per-connection rate limiting and priority scheduling
// =============================================================================

/**
 * @brief Network QoS Manager.
 *
 * Provides:
 * - Per-connection token bucket bandwidth limiting
 * - Four-level priority queue for outbound scheduling
 * - Fair-queuing across connections at the same priority
 * - Backpressure signalling (write-queue depth limit)
 * - Runtime reconfiguration (rate / burst / priority)
 * - Statistics per connection and per priority class
 *
 * Usage:
 * @code
 * QoSManager::Config cfg;
 * cfg.default_rate_bps   = 100'000'000;  // 100 Mbps default
 * cfg.default_burst_bytes = 1'000'000;   // 1 MB burst
 * cfg.max_queue_bytes    = 10'000'000;   // 10 MB write-queue cap
 *
 * QoSManager qos(cfg);
 * qos.registerConnection(conn_id, Priority::HIGH);
 *
 * // Before writing N bytes to a connection:
 * if (!qos.allowSend(conn_id, bytes)) {
 *     // Backpressure: drop or delay
 * }
 * qos.recordBytesSent(conn_id, bytes);
 * @endcode
 */
class QoSManager {
public:
    // -------------------------------------------------------------------------
    // Configuration
    // -------------------------------------------------------------------------

    struct Config {
        /** Total aggregate bandwidth cap (bps).  0 = unlimited. */
        uint64_t max_bandwidth_bps = 0;

        /** Default per-connection sustained rate (bps).  0 = unlimited. */
        uint64_t default_rate_bps = 0;

        /** Default burst size per connection (bytes).  0 = unlimited. */
        uint64_t default_burst_bytes = 1'000'000;  // 1 MB

        /** Maximum bytes queued per connection before backpressure triggers. */
        uint64_t max_queue_bytes = 10'000'000;  // 10 MB

        /** Enable strict priority scheduling (CRITICAL starves LOW). */
        bool enable_priority_scheduling = true;

        /** Enable fair queuing (equal service at the same priority level). */
        bool enable_fair_queuing = true;
    };

    explicit QoSManager(const Config& config);
    QoSManager();
    ~QoSManager();

    // Non-copyable, non-movable
    QoSManager(const QoSManager&)            = delete;
    QoSManager& operator=(const QoSManager&) = delete;
    QoSManager(QoSManager&&)                 = delete;
    QoSManager& operator=(QoSManager&&)      = delete;

    // -------------------------------------------------------------------------
    // Connection lifecycle
    // -------------------------------------------------------------------------

    /**
     * @brief Register a new connection with an optional initial priority.
     * @param connection_id Unique connection identifier.
     * @param priority      Initial traffic priority.
     */
    void registerConnection(uint64_t connection_id,
                            Priority priority = Priority::MEDIUM);

    /**
     * @brief Deregister a connection and free its token bucket.
     * @param connection_id Connection to remove.
     */
    void unregisterConnection(uint64_t connection_id);

    // -------------------------------------------------------------------------
    // Per-connection controls (runtime-adjustable)
    // -------------------------------------------------------------------------

    /**
     * @brief Change the priority of a live connection.
     * @param connection_id Target connection.
     * @param priority      New priority.
     */
    void setPriority(uint64_t connection_id, Priority priority);

    /**
     * @brief Set a bandwidth limit via a new token bucket for the connection.
     *
     * This replaces any existing token bucket for the connection.
     *
     * @param connection_id Target connection.
     * @param rate_bps      Sustained rate in bits per second.
     * @param burst_bytes   Maximum burst in bytes (0 = same as default_burst_bytes).
     */
    void setTokenBucket(uint64_t connection_id,
                        uint64_t rate_bps,
                        uint64_t burst_bytes = 0);

    /**
     * @brief Remove the token bucket from a connection (unlimited bandwidth).
     * @param connection_id Target connection.
     */
    void clearTokenBucket(uint64_t connection_id);

    // -------------------------------------------------------------------------
    // Hot-path: send permission and accounting
    // -------------------------------------------------------------------------

    /**
     * @brief Check whether `bytes` may be sent now (token bucket + queue check).
     *
     * If a token bucket is configured and there are insufficient tokens, the
     * function may block up to `timeout` to wait for refill.  If the write
     * queue depth would exceed `max_queue_bytes`, it returns false immediately
     * (backpressure signal).
     *
     * @param connection_id Target connection.
     * @param bytes         Number of bytes the caller wants to send.
     * @param timeout       Maximum time to wait for tokens.
     * @return true if the send is permitted, false if backpressure applies.
     */
    bool allowSend(uint64_t connection_id, uint64_t bytes,
                   std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    /**
     * @brief Record that `bytes` were successfully sent on a connection.
     *
     * Call this after the OS write call to update accounting.
     *
     * @param connection_id Source connection.
     * @param bytes         Bytes actually written.
     */
    void recordBytesSent(uint64_t connection_id, uint64_t bytes);

    /**
     * @brief Record that `bytes` were received on a connection.
     * @param connection_id Source connection.
     * @param bytes         Bytes received.
     */
    void recordBytesReceived(uint64_t connection_id, uint64_t bytes);

    // -------------------------------------------------------------------------
    // Statistics
    // -------------------------------------------------------------------------

    /**
     * @brief Per-connection statistics snapshot.
     */
    struct ConnectionStats {
        uint64_t connection_id   = 0;
        Priority priority        = Priority::MEDIUM;
        uint64_t bytes_sent      = 0;   ///< Total bytes sent on this connection
        uint64_t bytes_received  = 0;   ///< Total bytes received
        uint64_t bytes_shaped    = 0;   ///< Bytes delayed/dropped due to shaping
        uint64_t queue_depth     = 0;   ///< Current write-queue depth (bytes)
        uint64_t backpressure_events = 0; ///< Times backpressure was triggered
        bool     has_token_bucket = false;
        uint64_t token_bucket_rate_bps   = 0;
        uint64_t token_bucket_burst_bytes = 0;
    };

    /**
     * @brief Aggregate statistics across all connections.
     */
    struct Stats {
        uint64_t total_bytes_sent    = 0;
        uint64_t total_bytes_received = 0;
        uint64_t total_bytes_shaped  = 0;   ///< Bytes delayed due to QoS
        uint64_t backpressure_events = 0;   ///< Total backpressure events
        uint64_t active_connections  = 0;
        std::map<Priority, uint64_t> bytes_per_priority;
    };

    /**
     * @brief Retrieve aggregate statistics.
     */
    Stats getStats() const;

    /**
     * @brief Retrieve per-connection statistics for a specific connection.
     * @param connection_id Target connection.
     * @return Stats snapshot, or default-constructed if not found.
     */
    ConnectionStats getConnectionStats(uint64_t connection_id) const;

    /**
     * @brief Retrieve statistics for all registered connections.
     */
    std::vector<ConnectionStats> getAllConnectionStats() const;

    // -------------------------------------------------------------------------
    // Callbacks
    // -------------------------------------------------------------------------

    /**
     * @brief Set a callback invoked when backpressure is triggered.
     *
     * The callback receives the connection_id and the number of bytes that
     * could not be sent.  It is invoked while holding an internal callback
     * mutex, so it must be fast and non-blocking.
     */
    void setBackpressureCallback(
        std::function<void(uint64_t /*connection_id*/, uint64_t /*bytes*/)> cb);

private:
    // Per-connection state
    struct ConnectionState {
        uint64_t connection_id;
        std::atomic<uint8_t> priority{static_cast<uint8_t>(Priority::MEDIUM)};

        mutable std::mutex token_bucket_mutex;  // protects token_bucket
        std::shared_ptr<TokenBucket> token_bucket;  // nullptr = unlimited

        std::atomic<uint64_t> bytes_sent{0};
        std::atomic<uint64_t> bytes_received{0};
        std::atomic<uint64_t> bytes_shaped{0};
        std::atomic<uint64_t> queue_depth{0};
        std::atomic<uint64_t> backpressure_events{0};
    };

    std::shared_ptr<ConnectionState> findConnection(uint64_t id) const;

    Config config_;

    mutable std::mutex connections_mutex_;
    std::unordered_map<uint64_t, std::shared_ptr<ConnectionState>> connections_;

    // Aggregate counters (updated via relaxed atomics)
    std::atomic<uint64_t> total_bytes_sent_{0};
    std::atomic<uint64_t> total_bytes_received_{0};
    std::atomic<uint64_t> total_bytes_shaped_{0};
    std::atomic<uint64_t> total_backpressure_events_{0};

    // Per-priority byte counters
    mutable std::mutex priority_stats_mutex_;
    std::map<Priority, uint64_t> bytes_per_priority_;

    // Optional backpressure callback
    mutable std::mutex callback_mutex_;
    std::function<void(uint64_t, uint64_t)> backpressure_cb_;
};

}  // namespace network
}  // namespace themis
