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


struct SSMStateRocksDBStoreConfig {
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

class SSMStateRocksDBStore final : public ISSMStateStore {
public:
    using Config = SSMStateRocksDBStoreConfig;

    /**
     * @brief SSMState Rocks DBStore.
     * @param[in,out] db Input/output parameter.
     * @param[in,out] cf Input/output parameter.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit SSMStateRocksDBStore(
        rocksdb::TransactionDB* db,
        rocksdb::ColumnFamilyHandle* cf,
        const Config& config);

    explicit SSMStateRocksDBStore(
        rocksdb::TransactionDB* db,
        rocksdb::ColumnFamilyHandle* cf = nullptr);

    ~SSMStateRocksDBStore();

    // ISSMStateStore implementation
    bool checkpoint(const std::string& session_id,
                   const SSMStateSnapshot& snapshot) override;

    std::optional<SSMStateSnapshot> resume(
        const std::string& session_id,
        const std::optional<HLCTimestamp>& snapshot_ts = std::nullopt) override;

    bool invalidate(const std::string& session_id) override;

    uint64_t compact(uint64_t retention_window_ms = 24 * 60 * 60 * 1000) override;

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
     * @brief Make SSMState Key.
     * @param[in] session_id Identifier of the session.
     * @param[in] ts Input parameter.
     * @return Return value.
     */
    std::string makeSSMStateKey(const std::string& session_id,
                               const HLCTimestamp& ts);

    /**
     * @brief Serialize Snapshot.
     * @param[in] snapshot Input parameter.
     * @return Return value.
     */
    std::string serializeSnapshot(const SSMStateSnapshot& snapshot);

    /**
     * @brief Deserialize Snapshot.
     * @param[in] data Input parameter.
     * @return Return value.
     */
    std::optional<SSMStateSnapshot> deserializeSnapshot(const std::string& data);

    /**
     * @brief Parse Timestamp From Key.
     * @param[in] key Input parameter.
     * @return Return value.
     */
    std::optional<HLCTimestamp> parseTimestampFromKey(const std::string& key);

    std::optional<std::pair<HLCTimestamp, std::string>>
    findMostRecentSnapshot(const std::string& session_id);
};

} // namespace themis::llm
