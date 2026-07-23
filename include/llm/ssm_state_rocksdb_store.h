/**
 * @file ssm_state_rocksdb_store.h
 * @brief RocksDB-backed persistence for SSM state snapshots (Phase 2 P2-D04).
 * @version 0.1.0-beta
 * @note Maturity: BETA (Phase 2 P2-D04)
 * @note Status: RocksDB backend for ISSMStateStore with HLC-based MVCC
 */

#pragma once

#include "llm/i_ssm_plugin.h"
#include "llm/ssm_state_store.h"
#include "storage/hlc.h"

#include <memory>
#include <optional>
#include <string>
#include <mutex>
#include <cstdint>

namespace rocksdb {
    class TransactionDB;
    class ColumnFamilyHandle;
}

namespace themis::llm {

/**
 * @brief RocksDB-backed SSM state store for durable, MVCC-compatible persistence.
 *
 * Implements ISSMStateStore with RocksDB as the backend storage. Each state
 * snapshot is keyed by HLC timestamp to support:
 * - Point-in-time recovery via `resume(session_id, snapshot_ts)`
 * - MVCC-style isolation for concurrent readers
 * - Automatic cleanup via compaction of old snapshots
 *
 * **Key Format:**
 * ```
 * ssm_state:{session_id}:{hlc_timestamp_physical}:{hlc_timestamp_logical}
 * ```
 *
 * **Value Format:**
 * - Binary serialization of SSMStateSnapshot
 * - Format version prefix (1 byte) for forward compatibility
 *
 * **Thread Safety:**
 * - RocksDB TransactionDB ensures ACID properties
 * - Multiple concurrent checkpoint/resume operations on different sessions
 * - HLC timestamps enforce causal consistency
 *
 * **Performance:**
 * - Checkpoint: O(log N) RocksDB write with HLC-ordered key
 * - Resume: O(log N) RocksDB read via key prefix/range scan
 * - Compact: O(N) full-table scan to identify expired snapshots
 *
 * **Dependency Gate:** P2-D03 (L2 Episodic Memory) must succeed first
 */
class SSMStateRocksDBStore final : public ISSMStateStore {
public:
    /**
     * @brief Configuration for RocksDB SSM state store.
     */
    struct Config {
        // Column family name for SSM state (if nullptr, uses default CF)
        std::string column_family_name = "ssm_state";

        // Retention window for old snapshots (milliseconds)
        int64_t retention_window_ms = 24 * 60 * 60 * 1000;  // 24 hours default

        // Maximum snapshots per session to retain
        int32_t max_snapshots_per_session = 100;

        // Enable compression for stored snapshots
        bool enable_compression = true;

        // Sync writes to disk (safety vs performance tradeoff)
        bool sync_on_checkpoint = false;
    };

    /**
     * @brief Construct an SSM RocksDB store.
     *
     * @param db RocksDB TransactionDB instance (not owned; must outlive this store)
     * @param cf Optional column family handle (nullptr = use default)
     * @param config Runtime configuration
     *
     * @throws std::invalid_argument if db is nullptr
     * @throws std::runtime_error if column family initialization fails
     */
    explicit SSMStateRocksDBStore(
        rocksdb::TransactionDB* db,
        rocksdb::ColumnFamilyHandle* cf = nullptr,
        const Config& config = Config());

    ~SSMStateRocksDBStore();

    // ISSMStateStore implementation
    bool checkpoint(const std::string& session_id,
                   const SSMStateSnapshot& snapshot) override;

    std::optional<SSMStateSnapshot> resume(
        const std::string& session_id,
        const std::optional<HLCTimestamp>& snapshot_ts = std::nullopt) override;

    bool invalidate(const std::string& session_id) override;

    uint64_t compact(uint64_t retention_window_ms = 24 * 60 * 60 * 1000) override;

    /**
     * @brief Get statistics about stored snapshots.
     *
     * @return JSON object with session count, snapshot count, storage size, etc.
     */
    std::string getStats() const override;

    // Backwards-compat wrapper for older tests that expect `getStatistics()`
    inline std::string getStatistics() const { return getStats(); }

private:
    rocksdb::TransactionDB* db_;  // Not owned
    rocksdb::ColumnFamilyHandle* cf_;  // Not owned
    Config config_;
    mutable std::mutex mutex_;

    // Statistics
    int64_t total_checkpoints_ = 0;
    int64_t successful_checkpoints_ = 0;
    int64_t failed_checkpoints_ = 0;

    /**
     * @brief Construct RocksDB key for SSM state entry.
     *
     * Format: `ssm_state:{session_id}:{physical_time}:{logical_counter}`
     */
    std::string makeSSMStateKey(const std::string& session_id,
                               const HLCTimestamp& ts);

    /**
     * @brief Serialize SSMStateSnapshot to binary.
     *
     * @return Binary string with format version prefix + serialized data
     */
    std::string serializeSnapshot(const SSMStateSnapshot& snapshot);

    /**
     * @brief Deserialize SSMStateSnapshot from binary.
     *
     * @param data Binary string from RocksDB value
     * @return Deserialized snapshot, or empty if deserialization failed
     */
    std::optional<SSMStateSnapshot> deserializeSnapshot(const std::string& data);

    /**
     * @brief Parse HLC timestamp from RocksDB key.
     *
     * Extracts the physical and logical components from a key.
     *
     * @return HLCTimestamp if key is valid, empty if parse failed
     */
    std::optional<HLCTimestamp> parseTimestampFromKey(const std::string& key);

    /**
     * @brief Find the most recent snapshot for a session.
     *
     * Scans all snapshots with matching session_id prefix and returns the
     * one with the highest HLC timestamp.
     *
     * @return Snapshot with HLC ts, or empty if no snapshots exist
     */
    std::optional<std::pair<HLCTimestamp, std::string>>
    findMostRecentSnapshot(const std::string& session_id);
};

} // namespace themis::llm
