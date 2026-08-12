/**
 * @file backpressure_protocol.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Adaptive Backpressure Handshake Protocol
 * 
 * Cassandra-inspired protocol for load-aware synchronization deferral.
 * Defers sync operations during high load and replays them (WAL-like)
 * during low-load periods.
 * 
 * Features:
 * - System load monitoring (CPU, Memory, I/O, Network)
 * - Adaptive sync strategies (IMMEDIATE, THROTTLED, DEFERRED)
 * - WAL-like persistent deferral queue
 * - Automatic replay during idle periods
 * - Integration with existing StreamSession
 * 
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <functional>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <queue>
#include <map>
#include <optional>
#include <fstream>

namespace themisdb {
namespace backpressure {

// ============================================================================
// Forward Declarations
// ============================================================================

class BackpressureCoordinator;
class AdaptiveHandshakeSession;
class DeferredSyncQueue;
class SystemLoadMonitor;

// ============================================================================
// Enums & Constants
// ============================================================================

/**
 * Handshake protocol opcodes
 * Extends StreamMessageType for backpressure-specific messages
 */
enum class HandshakeOpcode : uint8_t {
    // Connection Phase
    HELLO = 0x01,                    // Initial connection with capabilities
    HELLO_ACK = 0x02,                // Acknowledge HELLO
    
    // Load Reporting
    LOAD_REPORT = 0x05,              // Current system load metrics
    LOAD_REPORT_ACK = 0x06,          // Acknowledge load report
    
    // Sync Negotiation
    SYNC_REQUEST = 0x10,             // Request synchronization
    SYNC_ACCEPT = 0x11,              // Accept immediate sync
    SYNC_DEFER = 0x12,               // Defer sync (with ETA)
    SYNC_THROTTLE = 0x13,            // Accept with throttled rate
    SYNC_REJECT = 0x14,              // Reject sync entirely
    
    // WAL Operations
    WAL_APPEND = 0x20,               // Append operation to WAL
    WAL_APPEND_ACK = 0x21,           // Acknowledge WAL append
    WAL_REPLAY_START = 0x22,         // Start WAL replay
    WAL_REPLAY_CHUNK = 0x23,         // WAL replay data chunk
    WAL_REPLAY_COMPLETE = 0x24,      // WAL replay finished
    
    // Flow Control
    PAUSE = 0x30,                    // Pause sync temporarily
    RESUME = 0x31,                   // Resume sync
    BACKPRESSURE_ALERT = 0x32,       // Alert: backpressure threshold crossed
    
    // Completion
    SYNC_COMPLETE = 0x40,            // Synchronization completed
    SYNC_COMPLETE_ACK = 0x41,        // Acknowledge completion
    
    // Error
    ERROR = 0xFF
};

/**
 * Sync strategy based on system load
 */
enum class SyncStrategy {
    IMMEDIATE,      // Load < 50%: Immediate sync
    THROTTLED,      // 50% <= Load < 80%: Reduced rate
    DEFERRED,       // Load >= 80%: Queue for later
    CRITICAL        // Any load: Force immediate (critical operations)
};

/**
 * Operation priority for deferral queue
 */
enum class OperationPriority : uint8_t {
    CRITICAL = 0,   // Must sync immediately (data loss risk)
    HIGH = 1,       // Sync soon (user-facing)
    NORMAL = 2,     // Regular sync
    LOW = 3,        // Can wait (background tasks)
    BACKGROUND = 4  // Best-effort only
};

/**
 * Deferred operation type
 */
enum class DeferredOperationType : uint8_t {
    SYNC_DATA = 1,           // Regular data sync
    MIGRATE_RANGE = 2,       // Token range migration
    REPAIR = 3,              // Consistency repair
    REBALANCE = 4,           // Rebalancing operation
    INDEX_SYNC = 5,          // Index synchronization
    BLOB_TRANSFER = 6        // Large blob transfer
};

/**
 * Handshake session state
 */
enum class HandshakeState {
    DISCONNECTED,
    CONNECTING,
    HELLO_SENT,
    NEGOTIATING,
    READY,
    STREAMING,
    THROTTLED,
    DEFERRED,
    PAUSED,
    COMPLETING,
    COMPLETE,
    FAILED
};

// Constants
constexpr uint32_t DEFAULT_LOAD_CHECK_INTERVAL_MS = 1000;
constexpr float CPU_THRESHOLD_THROTTLE = 50.0f;
constexpr float CPU_THRESHOLD_DEFER = 80.0f;
constexpr uint32_t DEFAULT_MAX_DEFER_TIME_SEC = 300;
constexpr uint32_t DEFAULT_WAL_MAX_SIZE_MB = 1024;
constexpr uint32_t DEFAULT_REPLAY_BATCH_SIZE = 100;

// ============================================================================
// Load Metrics
// ============================================================================

/**
 * System load metrics for backpressure decisions
 */
struct SystemLoadMetrics {
    // Resource utilization (0-100%)
    float cpu_usage_percent = 0.0f;
    float memory_usage_percent = 0.0f;
    float disk_io_utilization = 0.0f;
    float network_utilization = 0.0f;
    
    // Queue depths
    uint32_t pending_requests = 0;
    uint32_t active_migrations = 0;
    uint32_t deferred_ops_count = 0;
    
    // Latency
    std::chrono::milliseconds avg_latency_ms{0};
    std::chrono::milliseconds p99_latency_ms{0};
    
    // Timestamp
    std::chrono::steady_clock::time_point timestamp;
    
    /**
     * Calculate composite load score (0-100)
     */
    float getCompositeScore() const {
        // Weighted average with higher weight on CPU and I/O
        return (cpu_usage_percent * 0.35f +
                memory_usage_percent * 0.20f +
                disk_io_utilization * 0.30f +
                network_utilization * 0.15f);
    }
    
    /**
     * Determine sync strategy based on load
     */
    SyncStrategy getSyncStrategy() const {
        float score = getCompositeScore();
        if (score < CPU_THRESHOLD_THROTTLE) {
            return SyncStrategy::IMMEDIATE;
        } else if (score < CPU_THRESHOLD_DEFER) {
            return SyncStrategy::THROTTLED;
        } else {
            return SyncStrategy::DEFERRED;
        }
    }
    
    /**
     * Calculate throttle rate (bytes/sec) based on load
     */
    uint64_t getThrottleRate(uint64_t max_rate) const {
        float score = getCompositeScore();
        if (score < CPU_THRESHOLD_THROTTLE) {
            return max_rate;  // Full speed
        }
        // Linear reduction between THROTTLE and DEFER thresholds
        float reduction = (score - CPU_THRESHOLD_THROTTLE) / 
                         (CPU_THRESHOLD_DEFER - CPU_THRESHOLD_THROTTLE);
        return static_cast<uint64_t>(max_rate * (1.0f - reduction * 0.7f));
    }
    
    std::vector<uint8_t> serialize() const;
    static std::optional<SystemLoadMetrics> deserialize(const std::vector<uint8_t>& data);
};

// ============================================================================
// Deferred Operation
// ============================================================================

/**
 * Operation deferred for later replay
 */
struct DeferredOperation {
    std::string operation_id;
    DeferredOperationType type = DeferredOperationType::SYNC_DATA;
    OperationPriority priority = OperationPriority::NORMAL;
    
    // Source/Target
    std::string source_shard_id;
    std::string target_shard_id;
    
    // Token range (for range-based operations)
    uint64_t token_range_start = 0;
    uint64_t token_range_end = UINT64_MAX;
    
    // Timing
    std::chrono::steady_clock::time_point queued_at;
    std::chrono::steady_clock::time_point max_defer_until;
    std::chrono::steady_clock::time_point last_attempt;
    
    // Payload
    std::vector<uint8_t> payload;
    uint64_t payload_size = 0;
    
    // Retry tracking
    uint32_t retry_count = 0;
    uint32_t max_retries = 3;
    
    // Checksum
    uint32_t checksum = 0;
    
    /**
     * Check if operation has expired
     */
    bool isExpired() const {
        return std::chrono::steady_clock::now() > max_defer_until;
    }
    
    /**
     * Calculate age in seconds
     */
    uint32_t getAgeSeconds() const {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::seconds>(
            now - queued_at).count();
    }
    
    /**
     * Calculate effective priority (considers age)
     */
    int getEffectivePriority() const {
        int base = static_cast<int>(priority);
        // Boost priority for older operations
        int age_boost = getAgeSeconds() / 60;  // +1 per minute
        return std::max(0, base - age_boost);
    }
    
    std::vector<uint8_t> serialize() const;
    static std::optional<DeferredOperation> deserialize(const std::vector<uint8_t>& data);
};

/**
 * Comparator for priority queue
 */
struct DeferredOperationComparator {
    bool operator()(const DeferredOperation& a, const DeferredOperation& b) const {
        // Lower effective priority = higher priority
        return a.getEffectivePriority() > b.getEffectivePriority();
    }
};

// ============================================================================
// WAL Entry (for persistence)
// ============================================================================

/**
 * WAL entry header
 */
struct WALEntryHeader {
    uint32_t magic = 0x54484D57;  // "THMW"
    uint32_t version = 1;
    uint64_t sequence_number = 0;
    uint64_t timestamp_us = 0;
    uint32_t payload_length = 0;
    uint32_t checksum = 0;
    
    static constexpr size_t SIZE = 32;
    
    std::vector<uint8_t> serialize() const;
    static std::optional<WALEntryHeader> deserialize(const std::vector<uint8_t>& data);
};

// ============================================================================
// Handshake Message
// ============================================================================

/**
 * Handshake protocol message
 */
struct HandshakeMessage {
    HandshakeOpcode opcode = HandshakeOpcode::ERROR;
    uint32_t session_id = 0;
    uint64_t sequence_number = 0;
    uint32_t flags = 0;
    
    // Optional fields based on opcode
    std::optional<SystemLoadMetrics> load_metrics;
    std::optional<SyncStrategy> suggested_strategy;
    std::optional<uint64_t> throttle_rate;
    std::optional<std::chrono::milliseconds> defer_duration;
    std::optional<DeferredOperation> deferred_op;
    
    // Payload
    std::vector<uint8_t> payload;
    uint32_t checksum = 0;
    
    std::vector<uint8_t> serialize() const;
    static std::optional<HandshakeMessage> deserialize(const std::vector<uint8_t>& data);
};

// ============================================================================
// Configuration
// ============================================================================

/**
 * Backpressure configuration
 */
struct BackpressureConfig {
    bool enabled = true;
    
    // Load monitoring
    uint32_t load_check_interval_ms = DEFAULT_LOAD_CHECK_INTERVAL_MS;
    float cpu_threshold_throttle = CPU_THRESHOLD_THROTTLE;
    float cpu_threshold_defer = CPU_THRESHOLD_DEFER;
    float memory_threshold_critical = 95.0f;
    
    // Deferral
    uint32_t max_defer_time_sec = DEFAULT_MAX_DEFER_TIME_SEC;
    uint32_t max_deferred_ops = 10000;
    
    // WAL
    bool wal_enabled = true;
    std::string wal_path = "/var/lib/themisdb/deferred_sync.wal";
    uint64_t wal_max_size_bytes = DEFAULT_WAL_MAX_SIZE_MB * 1024 * 1024;
    uint32_t wal_fsync_interval_ms = 100;
    bool wal_compress = true;
    
    // Replay
    float replay_trigger_load_percent = 30.0f;
    uint32_t replay_batch_size = DEFAULT_REPLAY_BATCH_SIZE;
    uint64_t replay_rate_limit_per_sec = 1000;
    bool replay_during_maintenance_only = false;
    
    // Network
    uint32_t handshake_timeout_ms = 5000;
    uint32_t heartbeat_interval_ms = 1000;
    uint32_t load_report_interval_ms = 5000;
};

// ============================================================================
// System Load Monitor
// ============================================================================

/**
 * Monitors system resources for backpressure decisions
 */
class SystemLoadMonitor {
public:
    explicit SystemLoadMonitor(uint32_t check_interval_ms = DEFAULT_LOAD_CHECK_INTERVAL_MS);
    ~SystemLoadMonitor();
    
    /**
     * Start monitoring
     */
    void start();
    
    /**
     * Stop monitoring
     */
    void stop();
    
    /**
     * Get current load metrics
     */
    SystemLoadMetrics getCurrentLoad() const;
    
    /**
     * Get load history (for trending)
     */
    std::vector<SystemLoadMetrics> getLoadHistory(
        std::chrono::seconds duration = std::chrono::seconds(60)) const;
    
    /**
     * Register callback for load threshold crossings
     */
    using LoadThresholdCallback = std::function<void(const SystemLoadMetrics&, SyncStrategy)>;
    void setThresholdCallback(LoadThresholdCallback callback);
    
    /**
     * Check if system is under pressure
     */
    bool isUnderPressure() const {
        return getCurrentLoad().getSyncStrategy() == SyncStrategy::DEFERRED;
    }
    
    /**
     * Check if system is idle (suitable for replay)
     */
    bool isIdle(float threshold = 30.0f) const {
        return getCurrentLoad().getCompositeScore() < threshold;
    }

private:
    uint32_t check_interval_ms_;
    std::atomic<bool> running_{false};
    std::thread monitor_thread_;
    mutable std::mutex mutex_;
    
    SystemLoadMetrics current_load_;
    std::deque<SystemLoadMetrics> load_history_;
    LoadThresholdCallback threshold_callback_;
    
    void monitorLoop();
    SystemLoadMetrics measureLoad() const;
};

// ============================================================================
// Deferred Sync Queue
// ============================================================================

/**
 * WAL-like persistent queue for deferred operations
 */
class DeferredSyncQueue {
public:
    explicit DeferredSyncQueue(const BackpressureConfig& config);
    ~DeferredSyncQueue();
    
    /**
     * Initialize queue (load from WAL if exists)
     */
    bool initialize();
    
    /**
     * Shutdown queue (flush to WAL)
     */
    void shutdown();
    
    /**
     * Enqueue operation for deferred execution
     */
    bool enqueue(DeferredOperation op);
    
    /**
     * Dequeue next operation (by priority)
     */
    std::optional<DeferredOperation> dequeue();
    
    /**
     * Peek at next operation without removing
     */
    std::optional<DeferredOperation> peek() const;
    
    /**
     * Get all operations for a target shard
     */
    std::vector<DeferredOperation> getOperationsForShard(
        const std::string& shard_id) const;
    
    /**
     * Remove operation by ID
     */
    bool remove(const std::string& operation_id);
    
    /**
     * Get queue size
     */
    size_t size() const { return queue_.size(); }
    
    /**
     * Check if queue is empty
     */
    bool empty() const { return queue_.empty(); }
    
    /**
     * Get expired operations count
     */
    size_t getExpiredCount() const;
    
    /**
     * Prune expired operations
     */
    size_t pruneExpired();
    
    /**
     * Flush to WAL
     */
    bool flushToWAL();
    
    /**
     * Get WAL stats
     */
    struct WALStats {
        uint64_t entries_written = 0;
        uint64_t bytes_written = 0;
        uint64_t entries_replayed = 0;
        uint64_t wal_file_size = 0;
        std::chrono::steady_clock::time_point last_flush;
    };
    WALStats getWALStats() const;

private:
    BackpressureConfig config_;
    
    // In-memory priority queue
    std::priority_queue<DeferredOperation, 
                       std::vector<DeferredOperation>,
                       DeferredOperationComparator> queue_;
    
    // Index by operation ID
    std::map<std::string, std::reference_wrapper<const DeferredOperation>> op_index_;
    
    // WAL file
    std::fstream wal_file_;
    uint64_t wal_sequence_ = 0;
    WALStats wal_stats_;
    
    mutable std::mutex mutex_;
    
    bool openWAL();
    bool writeWALEntry(const DeferredOperation& op);
    bool loadFromWAL();
    void truncateWAL();
};

// ============================================================================
// Adaptive Handshake Session
// ============================================================================

/**
 * Manages a backpressure-aware synchronization session
 */
class AdaptiveHandshakeSession {
public:
    AdaptiveHandshakeSession(
        const std::string& local_shard_id,
        const std::string& remote_shard_id,
        const BackpressureConfig& config,
        std::shared_ptr<DeferredSyncQueue> deferred_queue);
    
    ~AdaptiveHandshakeSession();
    
    /**
     * Initiate handshake with remote shard
     */
    bool connect(const std::string& remote_endpoint);
    
    /**
     * Accept incoming handshake
     */
    bool accept();
    
    /**
     * Request sync operation
     */
    struct SyncRequest {
        DeferredOperationType type;
        OperationPriority priority;
        uint64_t token_range_start;
        uint64_t token_range_end;
        std::vector<uint8_t> payload;
    };
    
    enum class SyncResult {
        ACCEPTED,           // Sync will proceed immediately
        THROTTLED,          // Sync will proceed at reduced rate
        DEFERRED,           // Sync was deferred (added to queue)
        REJECTED,           // Sync was rejected
        ERROR               // Error occurred
    };
    
    SyncResult requestSync(const SyncRequest& request);
    
    /**
     * Report local load to remote
     */
    void reportLoad(const SystemLoadMetrics& metrics);
    
    /**
     * Get negotiated sync strategy
     */
    SyncStrategy getNegotiatedStrategy() const { return strategy_.load(); }
    
    /**
     * Get current throttle rate (bytes/sec)
     */
    uint64_t getThrottleRate() const { return throttle_rate_.load(); }
    
    /**
     * Get session state
     */
    HandshakeState getState() const { return state_.load(); }
    
    /**
     * Get session ID
     */
    uint32_t getSessionId() const { return session_id_; }
    
    /**
     * Pause sync operations
     */
    void pause();
    
    /**
     * Resume sync operations
     */
    void resume();
    
    /**
     * Close session
     */
    void close();
    
    /**
     * Check if session is active
     */
    bool isActive() const;
    
    /**
     * Set callback for state changes
     */
    using StateChangeCallback = std::function<void(HandshakeState, HandshakeState)>;
    void setStateChangeCallback(StateChangeCallback callback);

private:
    std::string local_shard_id_;
    std::string remote_shard_id_;
    BackpressureConfig config_;
    std::shared_ptr<DeferredSyncQueue> deferred_queue_;
    
    uint32_t session_id_ = 0;
    std::atomic<HandshakeState> state_{HandshakeState::DISCONNECTED};
    std::atomic<SyncStrategy> strategy_{SyncStrategy::IMMEDIATE};
    std::atomic<uint64_t> throttle_rate_{0};
    
    SystemLoadMetrics local_load_;
    SystemLoadMetrics remote_load_;
    
    StateChangeCallback state_callback_;
    
    std::atomic<bool> running_{false};
    std::thread heartbeat_thread_;
    std::thread load_report_thread_;
    std::mutex mutex_;
    std::condition_variable cv_;
    
    // Network (placeholder for actual mTLS connection)
    // std::shared_ptr<MTLSClient> connection_;
    
    void transitionState(HandshakeState new_state);
    bool sendMessage(const HandshakeMessage& msg);
    std::optional<HandshakeMessage> receiveMessage();
    void heartbeatLoop();
    void loadReportLoop();
    
    SyncStrategy negotiateStrategy(const SystemLoadMetrics& remote);
};

// ============================================================================
// Backpressure Coordinator
// ============================================================================

/**
 * Global coordinator for backpressure-aware synchronization
 */
class BackpressureCoordinator {
public:
    static BackpressureCoordinator& getInstance();
    
    /**
     * Initialize coordinator
     */
    void initialize(const BackpressureConfig& config);
    
    /**
     * Shutdown coordinator
     */
    void shutdown();
    
    /**
     * Check if initialized
     */
    bool isInitialized() const { return initialized_.load(); }
    
    /**
     * Create new handshake session
     */
    std::shared_ptr<AdaptiveHandshakeSession> createSession(
        const std::string& local_shard_id,
        const std::string& remote_shard_id);
    
    /**
     * Get active sessions
     */
    std::vector<std::shared_ptr<AdaptiveHandshakeSession>> getActiveSessions() const;
    
    /**
     * Get current system load
     */
    SystemLoadMetrics getCurrentLoad() const;
    
    /**
     * Get recommended sync strategy
     */
    SyncStrategy getRecommendedStrategy() const;
    
    /**
     * Get deferred queue
     */
    std::shared_ptr<DeferredSyncQueue> getDeferredQueue() const {
        return deferred_queue_;
    }
    
    /**
     * Trigger manual replay
     */
    void triggerReplay();
    
    /**
     * Get statistics
     */
    struct Stats {
        uint64_t sessions_total = 0;
        uint64_t sessions_active = 0;
        uint64_t syncs_immediate = 0;
        uint64_t syncs_throttled = 0;
        uint64_t syncs_deferred = 0;
        uint64_t syncs_rejected = 0;
        uint64_t replays_total = 0;
        uint64_t replays_successful = 0;
        uint64_t bytes_transferred = 0;
        std::chrono::steady_clock::time_point last_replay;
    };
    Stats getStats() const;
    
    /**
     * Export Prometheus metrics
     */
    std::string toPrometheusFormat() const;

private:
    BackpressureCoordinator() = default;
    ~BackpressureCoordinator() = default;
    
    BackpressureConfig config_;
    std::unique_ptr<SystemLoadMonitor> load_monitor_;
    std::shared_ptr<DeferredSyncQueue> deferred_queue_;
    
    std::vector<std::weak_ptr<AdaptiveHandshakeSession>> sessions_;
    Stats stats_;
    
    std::atomic<bool> initialized_{false};
    std::atomic<bool> running_{false};
    std::thread replay_thread_;
    mutable std::mutex mutex_;
    std::condition_variable replay_cv_;
    
    void replayLoop();
    bool shouldReplay() const;
    void processReplayBatch();
};

// ============================================================================
// Utility Functions
// ============================================================================

/**
 * Convert SyncStrategy to string
 */
inline const char* syncStrategyToString(SyncStrategy strategy) {
    switch (strategy) {
        case SyncStrategy::IMMEDIATE: return "IMMEDIATE";
        case SyncStrategy::THROTTLED: return "THROTTLED";
        case SyncStrategy::DEFERRED: return "DEFERRED";
        case SyncStrategy::CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

/**
 * Convert HandshakeState to string
 */
inline const char* handshakeStateToString(HandshakeState state) {
    switch (state) {
        case HandshakeState::DISCONNECTED: return "DISCONNECTED";
        case HandshakeState::CONNECTING: return "CONNECTING";
        case HandshakeState::HELLO_SENT: return "HELLO_SENT";
        case HandshakeState::NEGOTIATING: return "NEGOTIATING";
        case HandshakeState::READY: return "READY";
        case HandshakeState::STREAMING: return "STREAMING";
        case HandshakeState::THROTTLED: return "THROTTLED";
        case HandshakeState::DEFERRED: return "DEFERRED";
        case HandshakeState::PAUSED: return "PAUSED";
        case HandshakeState::COMPLETING: return "COMPLETING";
        case HandshakeState::COMPLETE: return "COMPLETE";
        case HandshakeState::FAILED: return "FAILED";
        default: return "UNKNOWN";
    }
}

} // namespace backpressure
} // namespace themisdb

