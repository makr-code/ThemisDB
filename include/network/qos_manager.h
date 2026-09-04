/**
 * @file qos_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// ThemisDB Network QoS Manager
// Token Bucket rate limiting, Priority Queues, and Backpressure control
// Q1 2026 – see docs/network_roadmap.md

#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
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
// LeakyBucket – constant-rate traffic shaper
// =============================================================================

/**
 * @brief Leaky bucket traffic shaper.
 *
 * Models a bucket with a fixed capacity that drains at a constant rate
 * (`drain_rate_bps` bits per second).  Data is added to the bucket via
 * `add(bytes)`.  If the bucket is full, `add()` returns false (overflow).
 *
 * Unlike the TokenBucket, the LeakyBucket enforces a strict constant output
 * rate with no accumulation — burst traffic is smoothed out.
 *
 * Thread-safe via an internal mutex.
 */
class LeakyBucket {
public:
    /**
     * @param drain_rate_bps  Output drain rate in bits per second.
     * @param capacity_bytes  Maximum bucket fill in bytes.
     */
    LeakyBucket(uint64_t drain_rate_bps, uint64_t capacity_bytes);

    /**
     * @brief Add `bytes` of data to the bucket.
     *
     * Drains elapsed time from the bucket first, then adds the new data.
     * Returns false if the bucket overflows (capacity exceeded after adding).
     *
     * @param bytes Number of bytes to add.
     * @return true if the bytes were accepted; false if the bucket overflowed.
     */
    bool add(uint64_t bytes);

    /**
     * @brief Check whether `bytes` can be transmitted now without overflow.
     *
     * Does NOT consume tokens; use `add()` to actually commit the send.
     *
     * @return true if sending `bytes` would not overflow the bucket.
     */
    bool tryConform(uint64_t bytes) const;

    /**
     * @brief Update drain rate and capacity at runtime.
     * @param drain_rate_bps  New output rate in bits per second.
     * @param capacity_bytes  New bucket capacity in bytes.
     */
    void reconfigure(uint64_t drain_rate_bps, uint64_t capacity_bytes);

    /** @brief Current bucket fill in bytes. */
    double currentFill() const;

    /** @brief Configured bucket capacity in bytes. */
    uint64_t capacityBytes() const;

    /** @brief Configured drain rate in bps. */
    uint64_t drainRateBps() const;

private:
    /** Drain elapsed bytes from bucket based on elapsed time. */
    void drain();

    mutable std::mutex mutex_;
    double fill_;                    ///< Current bucket fill (bytes)
    uint64_t drain_rate_bps_;        ///< Drain rate in bits per second
    uint64_t capacity_bytes_;        ///< Maximum bucket capacity (bytes)
    std::chrono::steady_clock::time_point last_drain_;
};

// =============================================================================
// CongestionController – per-connection AIMD congestion control
// =============================================================================

/**
 * @brief Simple AIMD congestion controller for per-connection rate adaptation.
 *
 * Implements a TCP-like AIMD (Additive Increase / Multiplicative Decrease)
 * algorithm:
 *   - Slow start:  window doubles each RTT until `ssthresh_bytes` is reached.
 *   - Congestion avoidance: window increases by MSS per RTT.
 *   - On packet loss:  ssthresh = window / 2;  window = ssthresh.
 *
 * Thread-safe via an internal mutex.
 */
class CongestionController {
public:
    static constexpr uint64_t kDefaultMss         = 1'460;        ///< bytes
    static constexpr uint64_t kDefaultInitialCwnd = 10 * kDefaultMss;
    static constexpr uint64_t kMaxCwnd            = 64 * 1024 * 1024;  ///< 64 MB

    CongestionController();

    /**
     * @brief Record a successful ACK (bytes acknowledged, round-trip time).
     *
     * Grows the congestion window according to slow-start / congestion-avoidance.
     *
     * @param bytes_acked  Number of bytes acknowledged.
     * @param rtt          Measured round-trip time.
     */
    void recordAck(uint64_t bytes_acked, std::chrono::microseconds rtt);

    /**
     * @brief Record a packet loss event.
     *
     * Halves the congestion window (AIMD decrease) and sets the slow-start
     * threshold.
     */
    void recordLoss();

    /** @brief Current congestion window in bytes. */
    uint64_t cwnd() const;

    /** @brief Current slow-start threshold in bytes. */
    uint64_t ssthresh() const;

    /** @brief Smoothed RTT estimate. */
    std::chrono::microseconds smoothedRtt() const;

    /** @brief Reset to initial state. */
    void reset();

private:
    mutable std::mutex mutex_;
    uint64_t cwnd_;             ///< Congestion window (bytes)
    uint64_t ssthresh_;         ///< Slow-start threshold (bytes)
    std::chrono::microseconds srtt_;  ///< Smoothed RTT
    bool in_slow_start_;
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

        /** Convenience: total aggregate bandwidth cap in Mbps.
         *  When non-zero, overrides max_bandwidth_bps (1 Mbps = 1,000,000 bps). */
        uint64_t max_bandwidth_mbps = 0;

        /** Default per-connection sustained rate (bps).  0 = unlimited. */
        uint64_t default_rate_bps = 0;

        /** Convenience: default per-connection limit in Mbps.
         *  When non-zero, overrides default_rate_bps. */
        uint64_t per_connection_limit_mbps = 0;

        /** Default burst size per connection (bytes).  0 = unlimited. */
        uint64_t default_burst_bytes = 1'000'000;  // 1 MB

        /** Maximum bytes queued per connection before backpressure triggers. */
        uint64_t max_queue_bytes = 10'000'000;  // 10 MB

        /** Enable strict priority scheduling (CRITICAL starves LOW). */
        bool enable_priority_scheduling = true;

        /** Enable priority queuing (alias for enable_priority_scheduling). */
        bool enable_priority_queuing = true;

        /** Enable fair queuing (equal service at the same priority level). */
        bool enable_fair_queuing = true;

        /** Maximum consecutive dequeues from higher priority before
         *  a lower-priority connection is forcibly served (starvation guard). */
        uint32_t starvation_guard_threshold = 16;
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
    // Snake-case API (issue requirement: consistent with FUTURE_ENHANCEMENTS spec)
    // -------------------------------------------------------------------------

    /**
     * @brief Change the priority of a live connection (snake_case alias).
     * @param connection_id Target connection.
     * @param priority      New priority.
     */
    void set_priority(uint64_t connection_id, Priority priority) {
        setPriority(connection_id, priority);
    }

    /**
     * @brief Set a per-connection bandwidth limit in bytes per second.
     *
     * Internally creates a token bucket with a burst equal to one second of
     * sustained throughput.  Use `setTokenBucket()` for explicit burst control.
     *
     * @param connection_id   Target connection.
     * @param bytes_per_second Sustained bandwidth limit in bytes per second.
     */
    void set_bandwidth_limit(uint64_t connection_id, uint64_t bytes_per_second);

    /**
     * @brief Set a token bucket for traffic shaping (snake_case alias).
     *
     * @param connection_id Target connection.
     * @param rate_bps      Sustained rate in bits per second.
     * @param burst_bytes   Maximum burst size in bytes.
     */
    void set_token_bucket(uint64_t connection_id,
                          uint64_t rate_bps,
                          uint64_t burst_bytes) {
        setTokenBucket(connection_id, rate_bps, burst_bytes);
    }

    // -------------------------------------------------------------------------
    // Leaky bucket shaping
    // -------------------------------------------------------------------------

    /**
     * @brief Attach a leaky bucket shaper to a connection.
     *
     * The leaky bucket enforces a strict constant drain rate with no burst
     * allowance.  Data that overflows the bucket is considered "non-conformant"
     * and `allowSend()` will return false for it.
     *
     * @param connection_id    Target connection.
     * @param drain_rate_bps   Constant drain rate in bits per second.
     * @param capacity_bytes   Maximum bucket capacity in bytes.
     */
    void setLeakyBucket(uint64_t connection_id,
                        uint64_t drain_rate_bps,
                        uint64_t capacity_bytes);

    /**
     * @brief Remove the leaky bucket shaper from a connection.
     * @param connection_id Target connection.
     */
    void clearLeakyBucket(uint64_t connection_id);

    // -------------------------------------------------------------------------
    // Priority queue scheduling
    // -------------------------------------------------------------------------

    /**
     * @brief Pending send descriptor for priority queue scheduling.
     */
    struct PendingSend {
        uint64_t connection_id = 0;
        uint64_t bytes         = 0;
        Priority priority      = Priority::MEDIUM;
    };

    /**
     * @brief Enqueue a pending send into the priority scheduler.
     *
     * The send is placed into the priority queue corresponding to the
     * connection's current priority level.  The scheduler does NOT
     * immediately consume token bucket tokens — that happens in `allowSend()`.
     *
     * @param connection_id Source connection.
     * @param bytes         Number of bytes to send.
     * @return true if the item was accepted; false if the connection is unknown.
     */
    bool enqueueSend(uint64_t connection_id, uint64_t bytes);

    /**
     * @brief Dequeue the next pending send selected by priority + fair-queuing.
     *
     * Selects the highest-priority non-empty queue.  Within a priority level,
     * connections are served in round-robin order (fair queuing).
     *
     * A starvation guard ensures that lower-priority items are occasionally
     * served even when higher-priority queues are always non-empty (see
     * `Config::starvation_guard_threshold`).
     *
     * @return The next PendingSend, or std::nullopt if all queues are empty.
     */
    std::optional<PendingSend> dequeueForSend();

    /**
     * @brief Return the number of pending sends in a specific priority queue.
     * @param priority Queue to query.
     */
    size_t getPendingQueueDepth(Priority priority) const;

    // -------------------------------------------------------------------------
    // Congestion control
    // -------------------------------------------------------------------------

    /**
     * @brief Record a successful ACK for congestion window adaptation.
     *
     * @param connection_id Connection that received the ACK.
     * @param bytes_acked   Number of bytes acknowledged.
     * @param rtt           Measured round-trip time for this segment.
     */
    void recordAck(uint64_t connection_id,
                   uint64_t bytes_acked,
                   std::chrono::microseconds rtt);

    /**
     * @brief Record a packet loss event for congestion window reduction.
     * @param connection_id Connection that experienced the loss.
     */
    void recordLoss(uint64_t connection_id);

    /**
     * @brief Return the current congestion window for a connection (bytes).
     *
     * Returns UINT64_MAX if no congestion controller is registered for the
     * connection (unlimited window).
     *
     * @param connection_id Target connection.
     */
    uint64_t getCongestionWindow(uint64_t connection_id) const;

    // -------------------------------------------------------------------------
    // Linux tc integration
    // -------------------------------------------------------------------------

    /**
     * @brief Configuration for Linux Traffic Control (tc) integration.
     */
    struct TcConfig {
        /** Network interface to configure (e.g. "eth0"). */
        std::string interface_name;

        /** Enable tc integration (Linux only; no-op on other platforms). */
        bool enabled = false;

        /** Total bandwidth allocated on the interface (bps). */
        uint64_t total_rate_bps = 0;
    };

    /**
     * @brief Apply an HTB qdisc configuration on a Linux network interface.
     *
     * Issues `tc qdisc` and `tc class` commands to set up a Hierarchical Token
     * Bucket on the specified interface.  On non-Linux platforms this function
     * returns false without doing anything.
     *
     * Requires the process to have sufficient privileges (CAP_NET_ADMIN).
     *
     * @param tc_config  tc configuration parameters.
     * @return true if tc commands succeeded; false otherwise.
     */
    bool configureTc(const TcConfig& tc_config);

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
        // Congestion control fields (UINT64_MAX = no CC active)
        uint64_t congestion_window = UINT64_MAX;         ///< Current cwnd in bytes
        uint64_t congestion_ssthresh_bytes = UINT64_MAX; ///< Slow-start threshold (bytes)
        uint64_t smoothed_rtt_us = 0;                    ///< Smoothed RTT in microseconds
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
    // Per-tenant bandwidth quota management
    // -------------------------------------------------------------------------

    /**
     * @brief Register a bandwidth quota for a tenant.
     *
     * All connections assigned to this tenant share a single token bucket that
     * enforces the aggregate bandwidth limit.  If a quota already exists for
     * the tenant, its parameters are updated in-place.
     *
     * @param tenant_id   Unique tenant identifier.
     * @param rate_bps    Sustained aggregate bandwidth limit in bits per second
     *                    (0 = unlimited).
     * @param burst_bytes Maximum burst size in bytes
     *                    (0 = 1 second of sustained rate, or unlimited when
     *                     rate_bps is also 0).
     */
    void registerTenantQuota(const std::string& tenant_id,
                              uint64_t rate_bps,
                              uint64_t burst_bytes = 0);

    /**
     * @brief Remove the bandwidth quota for a tenant.
     *
     * Existing connections assigned to this tenant become unquoted after this
     * call.  The connection-to-tenant assignment entries are NOT removed; they
     * simply have no tenant bucket to check against.
     *
     * @param tenant_id Tenant identifier.
     */
    void unregisterTenantQuota(const std::string& tenant_id);

    /**
     * @brief Update the bandwidth quota for a tenant at runtime.
     *
     * Equivalent to calling registerTenantQuota with the new parameters.
     * Creates the tenant entry if it does not already exist.
     *
     * @param tenant_id   Tenant identifier.
     * @param rate_bps    New sustained rate in bits per second (0 = unlimited).
     * @param burst_bytes New maximum burst size in bytes (0 = auto-derive).
     */
    void setTenantQuota(const std::string& tenant_id,
                        uint64_t rate_bps,
                        uint64_t burst_bytes = 0);

    /**
     * @brief Assign a registered connection to a tenant.
     *
     * After assignment, `allowSend` will also check the tenant-level token
     * bucket in addition to the per-connection bucket.  A connection can only
     * belong to one tenant; calling this again re-assigns it.
     *
     * Typically called after the client authenticates and the tenant is known.
     *
     * @param connection_id Connection to assign.
     * @param tenant_id     Target tenant identifier.
     */
    void assignTenant(uint64_t connection_id, const std::string& tenant_id);

    /**
     * @brief Statistics snapshot for a single tenant quota.
     */
    struct TenantQuotaStats {
        std::string tenant_id;
        uint64_t rate_bps          = 0;   ///< Configured sustained rate (bps)
        uint64_t burst_bytes       = 0;   ///< Configured burst size (bytes)
        uint64_t bytes_sent        = 0;   ///< Total bytes sent by all tenant connections
        uint64_t bytes_shaped      = 0;   ///< Bytes delayed/rejected by tenant quota
        uint64_t active_connections = 0;  ///< Currently assigned connections
    };

    /**
     * @brief Retrieve statistics for a specific tenant quota.
     * @param tenant_id Tenant identifier.
     * @return Stats snapshot, or default-constructed if tenant not found.
     */
    TenantQuotaStats getTenantStats(const std::string& tenant_id) const;

    /**
     * @brief Retrieve statistics for all registered tenant quotas.
     */
    std::vector<TenantQuotaStats> getAllTenantStats() const;

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
        uint64_t connection_id = 0;
        std::atomic<uint8_t> priority{static_cast<uint8_t>(Priority::MEDIUM)};

        mutable std::mutex token_bucket_mutex;  // protects token_bucket
        std::shared_ptr<TokenBucket> token_bucket;  // nullptr = unlimited

        mutable std::mutex leaky_bucket_mutex;  // protects leaky_bucket
        std::shared_ptr<LeakyBucket> leaky_bucket;  // nullptr = no leaky shaping

        mutable std::mutex congestion_mutex;    // protects congestion_ctrl
        std::shared_ptr<CongestionController> congestion_ctrl;  // nullptr = no CC

        std::atomic<uint64_t> bytes_sent{0};
        std::atomic<uint64_t> bytes_received{0};
        std::atomic<uint64_t> bytes_shaped{0};
        std::atomic<uint64_t> queue_depth{0};
        std::atomic<uint64_t> backpressure_events{0};
    };

    std::shared_ptr<ConnectionState> findConnection(uint64_t id) const;

    // Resolve effective max_bandwidth_bps from config (handles mbps override)
    uint64_t effectiveMaxBandwidthBps() const;
    // Resolve effective default_rate_bps from config (handles per_connection_limit_mbps)
    uint64_t effectiveDefaultRateBps() const;

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

    // -------------------------------------------------------------------------
    // Priority queue scheduling state
    // -------------------------------------------------------------------------

    // One deque per priority level; each entry is a PendingSend
    mutable std::mutex pq_mutex_;
    std::deque<PendingSend> pq_critical_;
    std::deque<PendingSend> pq_high_;
    std::deque<PendingSend> pq_medium_;
    std::deque<PendingSend> pq_low_;

    // Starvation guard: tracks how many consecutive high-priority sends have
    // been served so that low-priority connections are not starved indefinitely.
    uint32_t pq_consecutive_high_serves_{0};

    // -------------------------------------------------------------------------
    // Per-tenant bandwidth quota state
    // -------------------------------------------------------------------------

    struct TenantState {
        std::string tenant_id;

        mutable std::mutex token_bucket_mutex;          // protects token_bucket
        std::shared_ptr<TokenBucket> token_bucket;      // nullptr = unlimited

        std::atomic<uint64_t> bytes_sent{0};
        std::atomic<uint64_t> bytes_shaped{0};
        std::atomic<uint64_t> active_connections{0};
    };

    std::shared_ptr<TenantState> findTenant(const std::string& id) const;

    mutable std::mutex tenants_mutex_;
    std::unordered_map<std::string, std::shared_ptr<TenantState>> tenants_;

    // Maps connection_id -> tenant_id for quota enforcement
    mutable std::mutex tenant_assignments_mutex_;
    std::unordered_map<uint64_t, std::string> tenant_assignments_;
};

}  // namespace network
}  // namespace themis
