/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            stream_protocol.h                                  ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:09:35                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     819                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * ThemisDB Inter-Shard Streaming Protocol
 * 
 * Cassandra-inspired streaming architecture for efficient data transfer
 * between shards during rebalancing, repair, and bootstrap operations.
 * 
 * Features:
 * - Chunk-based transfer with checksums
 * - LZ4/Zstd compression
 * - Bandwidth throttling
 * - Resume on interruption
 * - Multi-stream parallelization
 * - Progress tracking
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
#include <set>
#include <optional>

namespace themisdb {
namespace streaming {

// ============================================================================
// Forward Declarations
// ============================================================================

class StreamSession;
class StreamPlan;
class StreamCoordinator;
class StreamTransferTask;
class StreamReceiveTask;

// ============================================================================
// Enums & Constants
// ============================================================================

/**
 * Streaming message types (protocol opcodes)
 */
enum class StreamMessageType : uint8_t {
    // Session Management
    PREPARE_REQUEST = 0x01,
    PREPARE_ACK = 0x02,
    PREPARE_NACK = 0x03,
    
    // Data Transfer
    FILE_HEADER = 0x10,
    DATA_CHUNK = 0x11,
    DATA_CHUNK_ACK = 0x12,
    FILE_COMPLETE = 0x13,
    
    // Control
    RETRY_REQUEST = 0x20,
    ABORT = 0x21,
    SESSION_COMPLETE = 0x22,
    SESSION_COMPLETE_ACK = 0x23,
    
    // Keepalive
    HEARTBEAT = 0x30,
    HEARTBEAT_ACK = 0x31,
    
    // Error
    ERROR = 0xFF
};

/**
 * Compression algorithms
 */
enum class CompressionAlgorithm : uint8_t {
    NONE = 0,
    LZ4 = 1,
    ZSTD = 2,
    SNAPPY = 3
};

/**
 * Stream session state machine
 */
enum class StreamSessionState {
    INITIALIZED,      // Session created
    PREPARING,        // Exchanging metadata
    STREAMING,        // Actively transferring data
    COMPLETE,         // Successfully completed
    FAILED,           // Failed (retryable)
    ABORTED           // Aborted (non-retryable)
};

/**
 * Stream direction
 */
enum class StreamDirection {
    OUTGOING,         // This node is sending
    INCOMING          // This node is receiving
};

// Constants
constexpr uint32_t DEFAULT_CHUNK_SIZE = 64 * 1024;           // 64KB chunks
constexpr uint32_t MAX_CHUNK_SIZE = 1024 * 1024;             // 1MB max
constexpr uint32_t DEFAULT_WINDOW_SIZE = 16;                  // Outstanding chunks
constexpr uint32_t DEFAULT_TIMEOUT_MS = 30000;               // 30s timeout
constexpr uint32_t HEARTBEAT_INTERVAL_MS = 5000;             // 5s heartbeat

// ============================================================================
// Configuration Structures
// ============================================================================

/**
 * Bandwidth throttling configuration
 */
struct StreamThrottleConfig {
    uint64_t max_bytes_per_second = 0;           // 0 = unlimited
    uint64_t inter_dc_max_bytes_per_second = 0;  // For cross-DC streams
    bool prioritize_local_dc = true;
    bool adaptive_throttling = true;             // Adjust based on load
};

/**
 * Stream session configuration
 */
struct StreamSessionConfig {
    std::string local_endpoint;
    std::string remote_endpoint;
    std::string local_shard_id;
    std::string remote_shard_id;
    StreamDirection direction;
    
    // Transfer settings
    uint32_t chunk_size = DEFAULT_CHUNK_SIZE;
    uint32_t window_size = DEFAULT_WINDOW_SIZE;
    uint32_t timeout_ms = DEFAULT_TIMEOUT_MS;
    
    // Compression
    CompressionAlgorithm compression = CompressionAlgorithm::LZ4;
    int compression_level = 1;  // 1-9, higher = more compression
    
    // Security
    std::string cert_path;
    std::string key_path;
    std::string ca_cert_path;
    bool require_mtls = true;
    
    // Throttling
    StreamThrottleConfig throttle;
};

/**
 * Stream plan configuration
 */
struct StreamPlanConfig {
    std::string plan_id;
    std::string description;
    
    // Parallelization
    uint32_t max_concurrent_sessions = 4;
    uint32_t max_concurrent_transfers_per_session = 2;
    
    // Retry policy
    uint32_t max_retries = 3;
    uint32_t retry_delay_ms = 5000;
    bool retry_on_timeout = true;
    
    // Callbacks
    bool notify_on_progress = true;
    uint32_t progress_report_interval_ms = 1000;
};

// ============================================================================
// Message Structures
// ============================================================================

/**
 * Stream message header (binary protocol)
 */
struct StreamMessageHeader {
    uint8_t version = 1;
    StreamMessageType type;
    uint32_t session_id;
    uint64_t sequence_number;
    uint32_t payload_length;
    uint32_t flags;              // Compressed, encrypted, etc.
    uint32_t checksum;           // CRC32 of payload
    
    static constexpr size_t SIZE = 26;
    
    std::vector<uint8_t> serialize() const;
    static std::optional<StreamMessageHeader> deserialize(const std::vector<uint8_t>& data);
};

/**
 * File/Collection metadata for transfer
 */
struct StreamFileInfo {
    std::string collection_name;
    std::string file_id;
    // Optional: absolute/relative paths for sender/receiver
    // These are used by local transfer tasks; RPC implementations may ignore them
    std::string source_path;   // path to read from on sender
    std::string target_path;   // path to write to on receiver
    uint64_t file_size;
    uint64_t num_documents;
    uint64_t token_range_start;
    uint64_t token_range_end;
    std::string content_hash;     // SHA-256 of entire file
    CompressionAlgorithm compression;
    
    std::vector<uint8_t> serialize() const;
    static std::optional<StreamFileInfo> deserialize(const std::vector<uint8_t>& data);
};

/**
 * Data chunk for transfer
 */
struct StreamChunk {
    uint64_t file_offset;
    uint32_t chunk_index;
    uint32_t uncompressed_size;
    uint32_t compressed_size;
    std::vector<uint8_t> data;
    uint32_t checksum;            // CRC32 of uncompressed data
    
    bool verify() const;
    std::vector<uint8_t> serialize() const;
    static std::optional<StreamChunk> deserialize(const std::vector<uint8_t>& data);
};

// ============================================================================
// Progress & Statistics
// ============================================================================

/**
 * Progress for a single file transfer
 */
struct StreamFileProgress {
    std::string file_id;
    uint64_t bytes_transferred = 0;
    uint64_t total_bytes = 0;
    uint32_t chunks_transferred = 0;
    uint32_t total_chunks = 0;
    uint32_t retry_count = 0;
    double compression_ratio = 1.0;
    std::chrono::steady_clock::time_point start_time;
    std::chrono::steady_clock::time_point last_activity;
    
    double getProgressPercent() const {
        return total_bytes > 0 ? (100.0 * bytes_transferred / total_bytes) : 0.0;
    }
    
    double getThroughputBytesPerSecond() const;
};

/**
 * Progress for an entire session
 */
struct StreamSessionProgress {
    uint32_t session_id = 0;
    StreamSessionState state = StreamSessionState::INITIALIZED;
    std::string remote_shard_id;
    StreamDirection direction;
    
    uint32_t files_completed = 0;
    uint32_t files_total = 0;
    uint64_t bytes_transferred = 0;
    uint64_t total_bytes = 0;
    
    std::vector<StreamFileProgress> file_progress;
    
    double getProgressPercent() const {
        return total_bytes > 0 ? (100.0 * bytes_transferred / total_bytes) : 0.0;
    }
};

/**
 * Statistics for streaming subsystem
 */
struct StreamingStats {
    std::atomic<uint64_t> sessions_total{0};
    std::atomic<uint64_t> sessions_successful{0};
    std::atomic<uint64_t> sessions_failed{0};
    std::atomic<uint64_t> bytes_sent_total{0};
    std::atomic<uint64_t> bytes_received_total{0};
    std::atomic<uint64_t> chunks_sent_total{0};
    std::atomic<uint64_t> chunks_received_total{0};
    std::atomic<uint64_t> chunk_retries_total{0};
    std::atomic<uint64_t> compression_bytes_saved{0};
    
    std::string toPrometheusFormat() const;
};

// ============================================================================
// Callbacks & Listeners
// ============================================================================

/**
 * Progress callback type
 */
using StreamProgressCallback = std::function<void(const StreamSessionProgress&)>;

/**
 * Completion callback type
 */
using StreamCompletionCallback = std::function<void(uint32_t session_id, bool success, const std::string& error)>;

/**
 * Listener interface for stream events
 */
class IStreamListener {
public:
    virtual ~IStreamListener() = default;
    
    virtual void onSessionStarted(uint32_t session_id, const std::string& remote_shard) = 0;
    virtual void onSessionProgress(const StreamSessionProgress& progress) = 0;
    virtual void onSessionCompleted(uint32_t session_id, bool success) = 0;
    virtual void onFileTransferStarted(uint32_t session_id, const StreamFileInfo& file) = 0;
    virtual void onFileTransferCompleted(uint32_t session_id, const std::string& file_id, bool success) = 0;
};

// ============================================================================
// Compression Utilities
// ============================================================================

/**
 * Compression helper class
 */
class StreamCompressor {
public:
    static std::vector<uint8_t> compress(
        const std::vector<uint8_t>& data,
        CompressionAlgorithm algorithm,
        int level = 1
    );
    
    static std::vector<uint8_t> decompress(
        const std::vector<uint8_t>& data,
        CompressionAlgorithm algorithm,
        size_t uncompressed_size
    );
    
    static bool isSupported(CompressionAlgorithm algorithm);
};

// ============================================================================
// Rate Limiter
// ============================================================================

/**
 * Token bucket rate limiter for bandwidth throttling
 */
class StreamRateLimiter {
public:
    explicit StreamRateLimiter(uint64_t bytes_per_second);
    
    /**
     * Acquire tokens for sending bytes
     * @param bytes Number of bytes to send
     * @return Time to wait before sending (0 if can send immediately)
     */
    std::chrono::milliseconds acquire(size_t bytes);
    
    /**
     * Update rate limit
     */
    void setRate(uint64_t bytes_per_second);
    
    /**
     * Get current rate
     */
    uint64_t getRate() const { return bytes_per_second_.load(); }
    
private:
    std::atomic<uint64_t> bytes_per_second_;
    std::atomic<uint64_t> available_tokens_;
    std::chrono::steady_clock::time_point last_refill_;
    std::mutex mutex_;
};

// ============================================================================
// Stream Transfer Task
// ============================================================================

/**
 * Handles sending data for one file in a stream session
 */
class StreamTransferTask {
public:
    StreamTransferTask(
        const StreamFileInfo& file,
        std::shared_ptr<StreamRateLimiter> rate_limiter,
        const StreamSessionConfig& config
    );
    
    ~StreamTransferTask();
    
    /**
     * Start the transfer
     */
    bool start();
    
    /**
     * Pause the transfer
     */
    void pause();
    
    /**
     * Resume the transfer
     */
    void resume();
    
    /**
     * Abort the transfer
     */
    void abort();
    
    /**
     * Handle acknowledgment for a chunk
     */
    void onChunkAck(uint32_t chunk_index);
    
    /**
     * Handle retry request for a chunk
     */
    void onRetryRequest(uint32_t chunk_index);
    
    /**
     * Get current progress
     */
    StreamFileProgress getProgress() const;
    
    /**
     * Check if transfer is complete
     */
    bool isComplete() const { return complete_.load(); }
    
    /**
     * Check if transfer failed
     */
    bool isFailed() const { return failed_.load(); }

private:
    StreamFileInfo file_;
    StreamSessionConfig config_;
    std::shared_ptr<StreamRateLimiter> rate_limiter_;
    
    std::atomic<bool> running_{false};
    std::atomic<bool> paused_{false};
    std::atomic<bool> complete_{false};
    std::atomic<bool> failed_{false};
    
    // Chunk tracking
    std::vector<bool> chunks_acked_;
    std::queue<uint32_t> pending_retries_;
    uint32_t next_chunk_to_send_ = 0;
    
    // Progress
    StreamFileProgress progress_;
    mutable std::mutex progress_mutex_;
    
    // Worker thread
    std::thread transfer_thread_;
    std::condition_variable cv_;
    std::mutex mutex_;
    
    void transferLoop();
    std::optional<StreamChunk> createChunk(uint32_t chunk_index);
    bool sendChunk(const StreamChunk& chunk);
};

// ============================================================================
// Stream Receive Task
// ============================================================================

/**
 * Handles receiving data for one file in a stream session
 */
class StreamReceiveTask {
public:
    StreamReceiveTask(
        const StreamFileInfo& file,
        const std::string& output_path,
        const StreamSessionConfig& config
    );
    
    ~StreamReceiveTask();
    
    /**
     * Start receiving
     */
    bool start();
    
    /**
     * Process received chunk
     */
    bool onChunkReceived(const StreamChunk& chunk);
    
    /**
     * Abort receiving
     */
    void abort();
    
    /**
     * Get current progress
     */
    StreamFileProgress getProgress() const;
    
    /**
     * Check if receive is complete
     */
    bool isComplete() const { return complete_.load(); }
    
    /**
     * Verify final file integrity
     */
    bool verifyIntegrity() const;

private:
    StreamFileInfo file_;
    std::string output_path_;
    StreamSessionConfig config_;
    
    std::atomic<bool> running_{false};
    std::atomic<bool> complete_{false};
    std::atomic<bool> failed_{false};
    
    // Chunk tracking
    std::vector<bool> chunks_received_;
    std::map<uint32_t, StreamChunk> out_of_order_chunks_;
    uint32_t next_expected_chunk_ = 0;
    
    // Progress
    StreamFileProgress progress_;
    mutable std::mutex progress_mutex_;
    
    // File output
    std::mutex write_mutex_;
    
    bool writeChunk(const StreamChunk& chunk);
    void requestRetry(uint32_t chunk_index);
};

// ============================================================================
// Stream Session
// ============================================================================

/**
 * Manages a single streaming session between two shards
 */
class StreamSession {
public:
    explicit StreamSession(const StreamSessionConfig& config);
    ~StreamSession();
    
    /**
     * Initialize the session (exchange metadata)
     */
    bool initialize();
    
    /**
     * Add file to transfer
     */
    void addFile(const StreamFileInfo& file);
    
    /**
     * Start streaming
     */
    bool start();
    
    /**
     * Pause streaming
     */
    void pause();
    
    /**
     * Resume streaming
     */
    void resume();
    
    /**
     * Abort session
     */
    void abort(const std::string& reason);
    
    /**
     * Get session ID
     */
    uint32_t getSessionId() const { return session_id_; }
    
    /**
     * Get current state
     */
    StreamSessionState getState() const { return state_.load(); }
    
    /**
     * Get progress
     */
    StreamSessionProgress getProgress() const;
    
    /**
     * Set progress callback
     */
    void setProgressCallback(StreamProgressCallback callback);
    
    /**
     * Set completion callback
     */
    void setCompletionCallback(StreamCompletionCallback callback);
    
    /**
     * Check if session is active
     */
    bool isActive() const;

private:
    StreamSessionConfig config_;
    uint32_t session_id_;
    std::atomic<StreamSessionState> state_{StreamSessionState::INITIALIZED};
    
    // Files to transfer
    std::vector<StreamFileInfo> files_;
    std::map<std::string, std::unique_ptr<StreamTransferTask>> transfer_tasks_;
    std::map<std::string, std::unique_ptr<StreamReceiveTask>> receive_tasks_;
    
    // Rate limiting
    std::shared_ptr<StreamRateLimiter> rate_limiter_;
    
    // Callbacks
    StreamProgressCallback progress_callback_;
    StreamCompletionCallback completion_callback_;
    
    // Threading
    std::atomic<bool> running_{false};
    std::thread session_thread_;
    std::thread heartbeat_thread_;
    std::mutex mutex_;
    std::condition_variable cv_;
    
    // Network
    // In real implementation, this would be the mTLS connection
    
    void sessionLoop();
    void heartbeatLoop();
    void processMessage(const StreamMessageHeader& header, const std::vector<uint8_t>& payload);
    bool sendMessage(StreamMessageType type, const std::vector<uint8_t>& payload);
    void notifyProgress();
    void transitionState(StreamSessionState new_state);
};

// ============================================================================
// Stream Plan
// ============================================================================

/**
 * Coordinates multiple stream sessions for a migration/repair operation
 */
class StreamPlan {
public:
    explicit StreamPlan(const StreamPlanConfig& config);
    ~StreamPlan();
    
    /**
     * Add session to plan
     */
    void addSession(std::unique_ptr<StreamSession> session);
    
    /**
     * Execute the plan
     */
    bool execute();
    
    /**
     * Abort the plan
     */
    void abort();
    
    /**
     * Wait for completion
     */
    bool waitForCompletion(std::chrono::milliseconds timeout = std::chrono::milliseconds::max());
    
    /**
     * Get plan ID
     */
    const std::string& getPlanId() const { return config_.plan_id; }
    
    /**
     * Get overall progress
     */
    std::vector<StreamSessionProgress> getProgress() const;
    
    /**
     * Check if plan is complete
     */
    bool isComplete() const { return complete_.load(); }
    
    /**
     * Check if plan succeeded
     */
    bool isSuccessful() const { return successful_.load(); }
    
    /**
     * Add listener
     */
    void addListener(std::shared_ptr<IStreamListener> listener);

private:
    StreamPlanConfig config_;
    
    std::vector<std::unique_ptr<StreamSession>> sessions_;
    std::vector<std::shared_ptr<IStreamListener>> listeners_;
    
    std::atomic<bool> running_{false};
    std::atomic<bool> complete_{false};
    std::atomic<bool> successful_{false};
    
    std::thread executor_thread_;
    std::mutex mutex_;
    std::condition_variable cv_;
    
    void executorLoop();
    void notifyListeners(std::function<void(IStreamListener&)> callback);
};

// ============================================================================
// Stream Coordinator
// ============================================================================

/**
 * Global coordinator for all streaming operations
 */
class StreamCoordinator {
public:
    static StreamCoordinator& getInstance();
    
    /**
     * Initialize coordinator
     */
    void initialize(const StreamThrottleConfig& throttle_config);
    
    /**
     * Shutdown coordinator
     */
    void shutdown();
    
    /**
     * Create new stream plan
     */
    std::shared_ptr<StreamPlan> createPlan(const StreamPlanConfig& config);
    
    /**
     * Get active plans
     */
    std::vector<std::shared_ptr<StreamPlan>> getActivePlans() const;
    
    /**
     * Get streaming statistics
     */
    const StreamingStats& getStats() const { return stats_; }
    
    /**
     * Get global rate limiter
     */
    std::shared_ptr<StreamRateLimiter> getRateLimiter() const { return global_rate_limiter_; }
    
    /**
     * Update throttle configuration
     */
    void updateThrottleConfig(const StreamThrottleConfig& config);

private:
    StreamCoordinator() = default;
    ~StreamCoordinator() = default;
    
    StreamThrottleConfig throttle_config_;
    std::shared_ptr<StreamRateLimiter> global_rate_limiter_;
    
    std::vector<std::shared_ptr<StreamPlan>> active_plans_;
    StreamingStats stats_;
    
    std::atomic<bool> initialized_{false};
    mutable std::mutex mutex_;
};

} // namespace streaming
} // namespace themisdb
