/**
 * @file stream_protocol.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.9.0-beta
 * @note Maturity: PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
    
    /** @brief Serialize message header into fixed-size big-endian wire format. */
    std::vector<uint8_t> serialize() const;

    /** @brief Parse message header from wire bytes. */
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
    
    /** @brief Serialize file metadata for transport. */
    std::vector<uint8_t> serialize() const;

    /** @brief Parse file metadata from transport bytes. */
    static std::optional<StreamFileInfo> deserialize(const std::vector<uint8_t>& data);
};

/**
 * Data chunk for transfer
 */
struct StreamChunk {
    uint64_t file_offset = 0;
    uint32_t chunk_index;
    uint32_t uncompressed_size;
    uint32_t compressed_size;
    std::vector<uint8_t> data;
    uint32_t checksum;            // CRC32 of uncompressed data
    
    /** @brief Verify chunk checksum/integrity. */
    bool verify() const;

    /** @brief Serialize chunk header and payload. */
    std::vector<uint8_t> serialize() const;

    /** @brief Parse chunk from serialized bytes with basic metadata checks. */
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
    
    /** @brief Export streaming counters in Prometheus exposition format. */
    std::string toPrometheusFormat() const;
};

// ============================================================================
// Callbacks & Listeners
// ============================================================================

/** @brief Callback invoked with incremental session progress snapshots. */
using StreamProgressCallback = std::function<void(const StreamSessionProgress&)>;

/** @brief Callback invoked when a session reaches terminal success/failure state. */
using StreamCompletionCallback = std::function<void(uint32_t session_id, bool success, const std::string& error)>;

/**
 * Listener interface for stream events
 */
class IStreamListener {
public:
    virtual ~IStreamListener() = default;

    /** @brief Called when a session transitions to started state. */
    virtual void onSessionStarted(uint32_t session_id, const std::string& remote_shard) = 0;

    /** @brief Called when session progress updates are available. */
    virtual void onSessionProgress(const StreamSessionProgress& progress) = 0;

    /** @brief Called when a session completes successfully or with failure. */
    virtual void onSessionCompleted(uint32_t session_id, bool success) = 0;

    /** @brief Called before transfer of one file begins. */
    virtual void onFileTransferStarted(uint32_t session_id, const StreamFileInfo& file) = 0;

    /** @brief Called when one file transfer completes. */
    virtual void onFileTransferCompleted(uint32_t session_id, const std::string& file_id, bool success) = 0;
};

// ============================================================================
// Compression Utilities
// ============================================================================

/** @brief Stateless compression/decompression utility for stream chunks. */
class StreamCompressor {
public:
    /** @brief Compress input bytes with selected algorithm. */
    static std::vector<uint8_t> compress(
        const std::vector<uint8_t>& data,
        CompressionAlgorithm algorithm,
        int level = 1
    );

    /** @brief Decompress bytes to expected uncompressed size. */
    static std::vector<uint8_t> decompress(
        const std::vector<uint8_t>& data,
        CompressionAlgorithm algorithm,
        size_t uncompressed_size
    );

    /** @brief Return whether compression algorithm is supported in current build. */
    static bool isSupported(CompressionAlgorithm algorithm);
};

// ============================================================================
// Rate Limiter
// ============================================================================

/** @brief Token-bucket limiter used to enforce bandwidth budgets. */
class StreamRateLimiter {
public:
    /** @brief Construct token-bucket limiter with byte-per-second budget. */
    explicit StreamRateLimiter(uint64_t bytes_per_second);
    
    /**
     * @brief Acquire send budget for a payload.
     * @param bytes Number of bytes intended to be sent.
     * @return Required wait duration before sending (zero means immediate send).
     */
    std::chrono::milliseconds acquire(size_t bytes);
    
    /** @brief Update byte-per-second rate limit. */
    void setRate(uint64_t bytes_per_second);
    
    /** @brief Return currently configured byte-per-second limit. */
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

/** @brief Worker responsible for sending one file within a session. */
class StreamTransferTask {
public:
    /** @brief Construct transfer task for one file. */
    StreamTransferTask(
        const StreamFileInfo& file,
        std::shared_ptr<StreamRateLimiter> rate_limiter,
        const StreamSessionConfig& config
    );

    /** @brief Destructor stops worker thread if running. */
    ~StreamTransferTask();
    
    /** @brief Start asynchronous file transfer worker. */
    bool start();
    
    /** @brief Pause transfer progress until resume is called. */
    void pause();
    
    /** @brief Resume a previously paused transfer. */
    void resume();
    
    /** @brief Abort transfer and mark task as failed/incomplete. */
    void abort();
    
    /** @brief Mark chunk as acknowledged by receiver. */
    void onChunkAck(uint32_t chunk_index);
    
    /** @brief Queue chunk for retransmission after retry request. */
    void onRetryRequest(uint32_t chunk_index);
    
    /** @brief Return point-in-time file transfer progress snapshot. */
    StreamFileProgress getProgress() const;
    
    /** @brief Return true once all required chunks are transferred/acked. */
    bool isComplete() const { return complete_.load(); }
    
    /** @brief Return true when task reached a failed terminal state. */
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

/** @brief Worker responsible for receiving one file within a session. */
class StreamReceiveTask {
public:
    /** @brief Construct receive task for one incoming file. */
    StreamReceiveTask(
        const StreamFileInfo& file,
        const std::string& output_path,
        const StreamSessionConfig& config
    );

    /** @brief Destructor stops receive task if running. */
    ~StreamReceiveTask();
    
    /** @brief Start receive side state and output bookkeeping. */
    bool start();
    
    /**
     * Process a received chunk.
     *
     * Fail-closed behavior:
     * - Rejects stale/duplicate/out-of-range chunk indices.
     * - Rejects inconsistent metadata (offset/size mismatch).
     * - Returns false on integrity or write failures instead of applying partial state.
     */
    bool onChunkReceived(const StreamChunk& chunk);
    
    /** @brief Abort receive workflow and stop accepting chunks. */
    void abort();
    
    /** @brief Return point-in-time receive progress snapshot. */
    StreamFileProgress getProgress() const;
    
    /** @brief Return true when all required chunks were persisted. */
    bool isComplete() const { return complete_.load(); }
    
    /** @brief Verify final output integrity (content hash/checksum). */
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

/** @brief Stateful sender/receiver session between two shard endpoints. */
class StreamSession {
public:
    /** @brief Construct one stream session endpoint. */
    explicit StreamSession(const StreamSessionConfig& config);

    /** @brief Destructor ensures session background threads are stopped. */
    ~StreamSession();
    
    /** @brief Initialize protocol handshake and session metadata exchange. */
    bool initialize();
    
    /** @brief Add one file descriptor to this session transfer queue. */
    void addFile(const StreamFileInfo& file);
    
    /** @brief Start session execution and worker threads. */
    bool start();
    
    /** @brief Pause active stream tasks in this session. */
    void pause();
    
    /** @brief Resume stream tasks after pause. */
    void resume();
    
    /** @brief Abort session and transition into terminal aborted state. */
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
     * @brief Inject a preparation callback for the mTLS PREPARE_REQUEST/ACK handshake.
     *
     * When set, `initialize()` calls this function instead of unconditionally
     * returning true.  The callback should establish a real mTLS connection to
     * `config_.remote_endpoint`, exchange PREPARE_REQUEST/PREPARE_ACK messages,
     * and return true on success.
     *
     * Production code should inject a real transport handler here.  Tests may
     * leave the callback unset (fallback: validate that `remote_endpoint` is
     * non-empty and return true so the state machine can proceed).
     *
     * @param cb Callable returning bool — true means prepared successfully
     */
    void setPrepareTransferCallback(std::function<bool()> cb);

    /** @brief Return true while session is running in non-terminal state. */
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
    // Preparation callback (injected for mTLS PREPARE_REQUEST/ACK handshake)
    std::function<bool()> prepare_callback_;
    
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

/** @brief Execution plan coordinating multiple stream sessions. */
class StreamPlan {
public:
    /** @brief Construct stream plan with execution policy. */
    explicit StreamPlan(const StreamPlanConfig& config);

    /** @brief Destructor waits/stops plan execution resources. */
    ~StreamPlan();
    
    /**
     * Add session to plan
     */
    void addSession(std::unique_ptr<StreamSession> session);
    
    /** @brief Execute plan using configured concurrency/retry settings. */
    bool execute();
    
    /** @brief Abort all in-flight sessions for this plan. */
    void abort();
    
    /** @brief Wait for plan completion or timeout. */
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

/** @brief Process-wide coordinator for stream plans and throttling. */
class StreamCoordinator {
public:
    /** @brief Return global stream coordinator singleton. */
    static StreamCoordinator& getInstance();
    
    /** @brief Initialize global coordinator and throttle state. */
    void initialize(const StreamThrottleConfig& throttle_config);
    
    /** @brief Shutdown coordinator and stop active plans/workers. */
    void shutdown();
    
    /** @brief Create and register a new stream plan instance. */
    std::shared_ptr<StreamPlan> createPlan(const StreamPlanConfig& config);
    
    /** @brief Return currently active stream plans snapshot. */
    std::vector<std::shared_ptr<StreamPlan>> getActivePlans() const;
    
    /**
     * Get streaming statistics
     */
    const StreamingStats& getStats() const { return stats_; }
    
    /** @brief Return global shared rate limiter used by plans/sessions. */
    std::shared_ptr<StreamRateLimiter> getRateLimiter() const { return global_rate_limiter_; }
    
    /** @brief Update throttle policy and refresh limiter budget. */
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
