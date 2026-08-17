/**
 * @file pitr_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <cstdint>
#include <memory>
#include <nlohmann/json.hpp>

#include "cdc/changefeed.h"
#include "transaction/snapshot_manager.h"

namespace themis {

namespace transaction { class SnapshotManager; }
class RocksDBWrapper;

/**
 * @brief Point-in-Time Recovery (PITR) Manager
 * 
 * Provides Git-like functionality to restore the database to a previous state:
 * - Restore to a specific sequence number
 * - Restore to a named snapshot (tag)
 * - Restore to a specific timestamp
 * - Preview restore operations (dry-run)
 * - Selective restore (table-level)
 * 
 * Safety features:
 * - Automatic backup before restore
 * - Dry-run mode with detailed preview
 * - Automatic rollback on errors
 * - Progress tracking for long operations
 * 
 * Use cases:
 * - Disaster Recovery: Restore after data corruption
 * - Schema Migration Rollback: Undo failed migrations
 * - Testing: Restore to known-good state
 * - Compliance: Restore to historical audit points
 * 
 * Implementation:
 * - Uses Changefeed events for backward replay
 * - Reverses operations: PUT→DELETE, DELETE→PUT
 * - Maintains transaction atomicity
 * 
 * Thread-safety:
 * - NOT thread-safe: Restore operations should be single-threaded
 * - Concurrent reads/writes during restore are blocked
 * - Use external locking for production environments
 */
class PITRManager {
public:
    /**
     * @brief Restore options
     */
    struct RestoreOptions {
        bool dry_run = false;                           // Preview only, don't apply changes
        bool create_backup = true;                      // Auto-backup before restore
        bool abort_on_first_error = true;              // Stop immediately on first replay error; otherwise continue scanning but still fail closed at end
        std::vector<std::string> tables;               // Empty = all tables, otherwise selective
        uint64_t max_events_to_replay = 0;             // 0 = unlimited, otherwise limit
        std::string backup_tag = "before_pitr_restore"; // Tag name for auto-backup
    };

    /**
     * @brief Restore preview (dry-run result)
     */
    struct RestorePreview {
        uint64_t target_sequence;                      // Target sequence to restore to
        uint64_t current_sequence;                     // Current sequence before restore
        uint64_t events_to_replay;                     // Number of events to replay backward
        std::vector<std::string> affected_tables;      // Tables that will be affected
        std::vector<std::string> affected_keys;        // Sample of keys (first 100)
        int64_t estimated_duration_sec;                // Estimated time to complete
        size_t estimated_size_bytes;                   // Estimated data size to process
    };

    /**
     * @brief Restore progress tracking
     */
    struct RestoreProgress {
        enum class Phase {
            NOT_STARTED,
            CREATING_BACKUP,
            VALIDATING,
            REPLAYING_EVENTS,
            COMMITTING,
            COMPLETED,
            FAILED,
            ROLLED_BACK
        };

        Phase phase = Phase::NOT_STARTED;
        uint64_t events_processed = 0;
        uint64_t total_events = 0;
        std::string current_table;
        std::string last_error;
        int64_t start_time_ms = 0;
        int64_t end_time_ms = 0;
        
        double getProgressPercent() const {
            if (total_events == 0) return 0.0;
            return (static_cast<double>(events_processed) / total_events) * 100.0;
        }
        
        int64_t getElapsedMs() const {
            if (start_time_ms == 0) return 0;
            int64_t end = (end_time_ms > 0) ? end_time_ms : getCurrentTimeMs();
            return end - start_time_ms;
        }
        
        static int64_t getCurrentTimeMs();
    };

    /**
     * @brief Status result for operations
     */
    struct Status {
        bool ok = true;
        std::string message;
        std::optional<RestoreProgress> progress;
        
        static Status OK() { return {true, "", std::nullopt}; }
        static Status Error(std::string msg) { return {false, std::move(msg), std::nullopt}; }
        static Status WithProgress(RestoreProgress prog) { 
            return {prog.phase == RestoreProgress::Phase::COMPLETED, "", prog}; 
        }
    };

    /**
     * @brief Construct PITRManager
     * @param db RocksDB wrapper instance (not owned)
     * @param changefeed Changefeed instance (not owned)
     * @param snapshot_mgr SnapshotManager instance (not owned)
     */
    explicit PITRManager(RocksDBWrapper* db,
                        Changefeed* changefeed,
                        transaction::SnapshotManager* snapshot_mgr);

    ~PITRManager() = default;

    // Disable copy, allow move
    PITRManager(const PITRManager&) = delete;
    PITRManager& operator=(const PITRManager&) = delete;
    PITRManager(PITRManager&&) noexcept noexcept = default;
    PITRManager& operator=(PITRManager&&) noexcept noexcept = default;

    /**
     * @brief Restore database to a specific sequence number
     * 
     * @param target_sequence Sequence number to restore to
     * @param options Restore options (dry_run, backup, etc.)
     * @return Status with progress information
     * 
     * Process:
     * 1. Create auto-backup snapshot (if enabled)
     * 2. Validate target sequence
     * 3. Replay events backward from current to target
     * 4. Commit or rollback on error
     * 
     * Errors:
     * - INVALID_SEQUENCE: Target sequence is invalid or in the future
     * - BACKUP_FAILED: Auto-backup creation failed
    * - WAL_REPLAY_INCOMPLETE: Required replay events are missing (truncated log)
     * - REPLAY_FAILED: Event replay failed
     */
    Status restoreToSequence(uint64_t target_sequence, const RestoreOptions& options);
    
    /// Restore to sequence with default options
    Status restoreToSequence(uint64_t target_sequence);

    /**
     * @brief Restore database to a named snapshot tag
     * 
     * @param tag_name Tag identifier
     * @param options Restore options
     * @return Status with progress information
     * 
     * Errors:
     * - TAG_NOT_FOUND: Tag does not exist
     * - (plus all errors from restoreToSequence)
     */
    Status restoreToTag(const std::string& tag_name, const RestoreOptions& options);
    
    /// Restore to tag with default options
    Status restoreToTag(const std::string& tag_name);

    /**
     * @brief Restore database to a specific timestamp
     * 
     * @param timestamp_ms Unix timestamp in milliseconds
     * @param options Restore options
     * @return Status with progress information
     * 
     * Finds the latest sequence number <= timestamp and restores to it.
     * 
     * Errors:
     * - NO_EVENTS_AT_TIME: No events found at or before timestamp
     * - (plus all errors from restoreToSequence)
     */
    Status restoreToTimestamp(int64_t timestamp_ms, const RestoreOptions& options);
    
    /// Restore to timestamp with default options
    Status restoreToTimestamp(int64_t timestamp_ms);

    /**
     * @brief Preview restore operation (dry-run)
     * 
     * @param target_sequence Sequence to restore to
     * @param options Restore options (only tables filter is used)
     * @return Preview with estimated impact
     * 
     * Useful for:
     * - Estimating restore time
     * - Checking affected tables/keys
     * - Validating restore feasibility
     */
    RestorePreview previewRestore(uint64_t target_sequence, const RestoreOptions& options) const;
    
    /// Preview restore with default options
    RestorePreview previewRestore(uint64_t target_sequence) const;

    /**
     * @brief Get current restore progress
     * 
     * @return Current progress, or nullopt if no restore in progress
     */
    std::optional<RestoreProgress> getProgress() const;

    /**
     * @brief Check if a restore operation is currently in progress
     * 
     * @return true if restore is active
     */
    bool isRestoreInProgress() const;

    /**
     * @brief Get sequence number for a named tag
     * 
     * @param tag_name Tag identifier
     * @return Sequence number if tag exists, nullopt otherwise
     * 
     * Useful for API clients that want to convert tags to sequences.
     */
    std::optional<uint64_t> getSequenceForTag(const std::string& tag_name) const;

    /**
     * @brief Get sequence number for a timestamp
     * 
     * @param timestamp_ms Unix timestamp in milliseconds
     * @return Latest sequence number <= timestamp, nullopt if no events found
     * 
     * Useful for API clients that want to convert timestamps to sequences.
     */
    std::optional<uint64_t> getSequenceForTimestamp(int64_t timestamp_ms) const;

private:
    RocksDBWrapper* db_;
    Changefeed* changefeed_;
    transaction::SnapshotManager* snapshot_mgr_;
    
    // Progress tracking (mutable for thread-safe updates)
    mutable RestoreProgress progress_;

    /**
     * @brief Find sequence number for a given timestamp
     * 
     * Returns the latest sequence <= timestamp
     */
    std::optional<uint64_t> findSequenceForTimestamp(int64_t timestamp_ms) const;

    /**
     * @brief Replay events backward from current to target sequence
     * 
     * For each event in reverse order:
     * - PUT → DELETE (remove the value)
     * - DELETE → PUT (restore the value)
     * - TRANSACTION_COMMIT/ROLLBACK → Metadata only, skip
     */
    Status replayBackward(uint64_t from_sequence, uint64_t to_sequence, 
                         const RestoreOptions& options);

    /**
     * @brief Apply a single event in reverse
     * 
     * - PUT event → Delete the key
    * - DELETE event → Restore previous value (requires value or before_snapshot)
    * 
    * Fails closed when the previous value is unavailable.
     */
    Status applyEventReverse(const Changefeed::ChangeEvent& event);

    /**
     * @brief Create automatic backup before restore
     */
    Status createAutoBackup(const RestoreOptions& options);

    /**
     * @brief Validate restore parameters
     */
    Status validate(uint64_t target_sequence, uint64_t current_sequence) const;

    /**
     * @brief Update progress tracking
     */
    void updateProgress(RestoreProgress::Phase phase, const std::string& message = "");
};

} // namespace themis
