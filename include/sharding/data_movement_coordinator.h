#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <chrono>
#include <functional>

namespace themis {
namespace sharding {

// Forward declarations
class PrometheusMetrics;

/**
 * Data batch for streaming rebalance
 */
struct DataBatch {
    std::string source_shard_id;
    std::string target_shard_id;
    std::vector<std::string> rows;  // Serialized rows (JSON or binary)
    uint64_t batch_number;
    uint64_t total_batches;
    std::string checksum;  // SHA256 of batch content
    std::chrono::system_clock::time_point timestamp;
    
    // Token range this batch belongs to
    uint64_t token_range_start;
    uint64_t token_range_end;
};

/**
 * Stream state for active data movement
 */
struct StreamState {
    std::string stream_id;
    std::string source_shard_id;
    std::string target_shard_id;
    uint64_t token_range_start;
    uint64_t token_range_end;
    
    // Progress tracking
    uint64_t batches_sent{0};
    uint64_t batches_acknowledged{0};
    uint64_t total_batches{0};
    uint64_t bytes_transferred{0};
    
    // Timestamps
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point last_activity;
    
    // State
    bool is_complete{false};
    bool has_error{false};
    std::string error_message;
};

/**
 * DataMovementCoordinator
 * 
 * Coordinates streaming data movement between shards during rebalance operations.
 * 
 * Features:
 * - Batch-based streaming with configurable size
 * - Checksum verification for data integrity
 * - Progress tracking and checkpointing
 * - Concurrent batch handling
 * - Retry logic for failed batches
 */
class DataMovementCoordinator {
public:
    struct Config {
        size_t batch_size{10000};
        std::chrono::milliseconds batch_timeout{5000};
        size_t max_concurrent_batches{10};
        bool verify_checksums{true};
    };
    
    using ProgressCallback = std::function<void(const StreamState&)>;
    
    explicit DataMovementCoordinator(const Config& config);
    
    /**
     * Start streaming from source to target
     * @param source_shard_id Source shard identifier
     * @param target_shard_id Target shard identifier
     * @param token_ranges Token ranges to transfer
     * @param progress_callback Optional progress callback
     * @return Stream ID for tracking
     */
    std::string startStreaming(
        const std::string& source_shard_id,
        const std::string& target_shard_id,
        const std::vector<uint64_t>& token_ranges,
        ProgressCallback progress_callback = nullptr
    );
    
    /**
     * Receive and apply batch at target
     * @param batch Data batch to apply
     * @return true if applied successfully
     */
    bool receiveBatch(const DataBatch& batch);
    
    /**
     * Verify data integrity after movement
     * @param source_shard_id Source shard
     * @param target_shard_id Target shard
     * @param token_ranges Token ranges to verify
     * @return true if data matches
     */
    bool verifyDataIntegrity(
        const std::string& source_shard_id,
        const std::string& target_shard_id,
        const std::vector<uint64_t>& token_ranges
    );
    
    /**
     * Checkpoint progress for recovery
     * @param stream_id Stream identifier
     */
    void checkpointProgress(const std::string& stream_id);
    
    /**
     * Get stream state
     * @param stream_id Stream identifier
     * @return Stream state if found
     */
    std::optional<StreamState> getStreamState(const std::string& stream_id) const;
    
    /**
     * Cancel active stream
     * @param stream_id Stream identifier
     * @return true if cancelled successfully
     */
    bool cancelStream(const std::string& stream_id);
    
    /**
     * Set metrics collector
     */
    void setMetrics(std::shared_ptr<PrometheusMetrics> metrics);
    
private:
    Config config_;
    std::map<std::string, StreamState> active_streams_;
    std::map<std::string, ProgressCallback> progress_callbacks_;
    std::shared_ptr<PrometheusMetrics> metrics_;
    mutable std::mutex mutex_;
    
    // Internal helpers
    std::string generateStreamId() const;
    std::string calculateChecksum(const std::vector<std::string>& rows) const;
    bool verifyChecksum(const DataBatch& batch) const;
    void updateStreamProgress(const std::string& stream_id, const DataBatch& batch);
};

} // namespace sharding
} // namespace themis
