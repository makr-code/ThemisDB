/**
 * @file shard_durability.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

#pragma once

#include <string>
#include <memory>
#include <chrono>
#include <atomic>
#include <mutex>
#include <functional>

// Forward declarations
namespace rocksdb {
    class TransactionDB;
    class Checkpoint;
}

namespace themisdb {
namespace sharding {

/**
 * @brief Durability modes for operations
 */
enum class DurabilityMode {
    NONE,           // No durability guarantees (fastest, risky)
    ASYNC,          // Async WAL sync (high performance)
    SYNC,           // Sync WAL on every write (safest)
    GROUP_COMMIT    // Group commits for balanced performance/safety
};

/**
 * @brief Checkpoint metadata
 */
struct CheckpointInfo {
    std::string checkpoint_id;
    std::string path;
    std::chrono::system_clock::time_point created_at;
    uint64_t sequence_number;
    uint64_t size_bytes;
    bool is_valid;
};

/**
 * @brief Crash recovery statistics
 */
struct RecoveryStats {
    bool recovery_needed{false};
    bool recovery_successful{false};
    uint64_t records_recovered{0};
    uint64_t records_corrupted{0};
    std::chrono::milliseconds recovery_duration{0};
    std::string last_error;
};

/**
 * @brief Configuration for shard durability
 */
struct ShardDurabilityConfig {
    DurabilityMode mode{DurabilityMode::ASYNC};
    bool enable_wal{true};
    bool sync_wal_on_commit{false};
    size_t group_commit_batch_size{100};
    std::chrono::milliseconds group_commit_timeout{10};
    std::string checkpoint_dir{"./checkpoints"};
    std::chrono::hours checkpoint_interval{24};
    size_t max_checkpoints{7};  // Keep last 7 checkpoints
    bool enable_auto_recovery{true};
};

/**
 * @brief Manages durability guarantees for sharded data
 * 
 * Integrates with RocksDB's native WAL (Write-Ahead Log) to provide:
 * - Configurable durability modes (sync/async/group commit)
 * - Checkpoint management for point-in-time recovery
 * - Automatic crash recovery
 * - Durability guarantees under failure scenarios
 */
class ShardDurability {
public:
    using RecoveryCallback = std::function<void(const RecoveryStats&)>;
    
    /**
     * @brief Constructor
     * @param rocksdb_instance RocksDB TransactionDB instance
     * @param config Durability configuration
     */
    explicit ShardDurability(
        rocksdb::TransactionDB* rocksdb_instance,
        const ShardDurabilityConfig& config
    );
    
    ~ShardDurability();
    
    /**
     * @brief Initialize durability layer
     * Performs crash recovery if needed
     * @return true on success
     */
    bool initialize();
    
    /**
     * @brief Shutdown durability layer
     * Ensures all pending writes are synced
     */
    void shutdown();
    
    /**
     * @brief Force sync WAL to disk
     * @return true on success
     */
    bool syncWAL();
    
    /**
     * @brief Create a checkpoint (point-in-time snapshot)
     * @param checkpoint_name Optional checkpoint name
     * @return Checkpoint info on success, nullopt on failure
     */
    std::optional<CheckpointInfo> createCheckpoint(
        const std::string& checkpoint_name = ""
    );
    
    /**
     * @brief List available checkpoints
     */
    std::vector<CheckpointInfo> listCheckpoints() const;
    
    /**
     * @brief Restore from a checkpoint
     * WARNING: This operation requires database shutdown and restart
     * @param checkpoint_id Checkpoint to restore from
     * @return true on success
     */
    bool restoreFromCheckpoint(const std::string& checkpoint_id);
    
    /**
     * @brief Perform crash recovery
     * Replays WAL to recover uncommitted transactions
     * @return Recovery statistics
     */
    RecoveryStats performRecovery();
    
    /**
     * @brief Verify WAL integrity
     * @return true if WAL is intact
     */
    bool verifyWALIntegrity() const;
    
    /**
     * @brief Get current WAL sequence number
     */
    uint64_t getCurrentSequenceNumber() const;
    
    /**
     * @brief Set recovery callback
     * Called when recovery is performed
     */
    void setRecoveryCallback(RecoveryCallback callback);
    
    /**
     * @brief Update configuration
     */
    void updateConfig(const ShardDurabilityConfig& config);
    
    /**
     * @brief Get current configuration
     */
    const ShardDurabilityConfig& getConfig() const { return config_; }
    
    /**
     * @brief Get statistics
     */
    struct Statistics {
        std::atomic<uint64_t> total_syncs{0};
        std::atomic<uint64_t> checkpoints_created{0};
        std::atomic<uint64_t> recoveries_performed{0};
        std::atomic<uint64_t> wal_bytes_written{0};
        std::chrono::system_clock::time_point last_sync_time;
        std::chrono::system_clock::time_point last_checkpoint_time;
    };
    
    const Statistics& getStatistics() const { return stats_; }
    
    /**
     * @brief Check if durability is enabled
     */
    bool isDurabilityEnabled() const { return config_.enable_wal; }

private:
    rocksdb::TransactionDB* db_;
    ShardDurabilityConfig config_;
    mutable std::mutex mutex_;
    Statistics stats_;
    RecoveryCallback recovery_callback_;
    
    std::unique_ptr<rocksdb::Checkpoint> checkpoint_manager_;
    std::vector<CheckpointInfo> checkpoints_;
    
    /**
     * @brief Generate unique checkpoint ID
     */
    std::string generateCheckpointId() const;
    
    /**
     * @brief Clean up old checkpoints
     */
    void cleanupOldCheckpoints();
    
    /**
     * @brief Scan checkpoint directory
     */
    void scanCheckpointDirectory();
    
    /**
     * @brief Validate checkpoint
     */
    bool validateCheckpoint(const std::string& checkpoint_path) const;
};

}  // namespace sharding
}  // namespace themisdb
