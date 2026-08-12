/**
 * @file transaction_snapshot.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <memory>
#include <nlohmann/json.hpp>
#include "transaction_wal.h"

namespace sharding {

using LSN = themis::sharding::LSN;

/** @brief Transaction lifecycle states persisted in snapshot entries. */
enum class TransactionState {
    INITIATED,           // Transaction started
    PREPARING,          // Preparing phase in progress
    PREPARED,           // All participants prepared (2PC/3PC)
    PRE_COMMITTING,     // Pre-commit phase (3PC only)
    PRE_COMMITTED,      // Pre-commit phase done (3PC only)
    COMMITTING,         // Committing in progress
    COMMITTED,          // Transaction committed
    ABORTING,           // Aborting in progress
    ABORTED,            // Transaction aborted
    COMPENSATING,       // SAGA compensation in progress
    COMPENSATED         // SAGA compensation done
};

/** @brief Per-participant status details captured in one transaction snapshot entry. */
struct ParticipantStatus {
    std::string participant_id;
    bool prepared = false;          // For 2PC/3PC
    bool pre_committed = false;     // For 3PC only
    bool committed = false;
    bool aborted = false;
    std::string response_data;
    uint64_t timestamp = 0;
};

/** @brief One SAGA step or compensation step snapshot record. */
struct SAGAStep {
    uint32_t step_number;
    std::string operation;
    nlohmann::json data;
    bool completed = false;
    bool compensated = false;
    uint64_t timestamp = 0;
};

/** @brief Percolator write-intent state captured in snapshot metadata. */
struct PercolatorIntent {
    std::string key;
    nlohmann::json value;
    uint64_t start_timestamp;
    bool locked = false;
};

/** @brief Full persisted state for one active distributed transaction. */
struct TransactionSnapshotEntry {
    std::string transaction_id;
    TransactionProtocol protocol;
    TransactionState state;
    std::vector<std::string> participants;
    std::map<std::string, ParticipantStatus> participant_status;
    uint64_t start_timestamp;
    uint64_t timeout_ms;
    
    // Protocol-specific data
    nlohmann::json prepare_data;
    nlohmann::json commit_data;
    
    // SAGA-specific
    std::vector<SAGAStep> saga_steps;
    std::vector<SAGAStep> saga_compensations;
    
    // Percolator-specific
    std::vector<PercolatorIntent> write_intents;
    uint64_t percolator_commit_timestamp = 0;
    
    // Additional metadata
    std::string coordinator_id;
    nlohmann::json metadata;
};

/** @brief Complete transaction snapshot payload including checksum metadata. */
struct TransactionSnapshot {
    uint64_t snapshot_id;
    LSN last_applied_lsn;
    std::string coordinator_id;
    uint64_t timestamp;
    std::vector<TransactionSnapshotEntry> active_transactions;
    std::string checksum;  // SHA-256
    size_t total_transactions;
    
    /** @brief Serialize snapshot content (without auto-checksum recompute) to JSON. */
    nlohmann::json toJson() const;
    
    /** @brief Parse snapshot payload from JSON document. */
    static std::optional<TransactionSnapshot> fromJson(const nlohmann::json& j);
};

/** @brief Filesystem-backed manager for writing/reading transaction snapshots. */
class TransactionSnapshotManager {
public:
    /**
     * @brief Construct manager with target directory and retention size.
     * @param snapshot_directory Snapshot storage directory.
     * @param max_snapshots Maximum retained snapshot files.
     */
    TransactionSnapshotManager(const std::string& snapshot_directory, size_t max_snapshots = 10);
    ~TransactionSnapshotManager() = default;
    
    /**
     * @brief Create and persist a new transaction snapshot.
     * @return Snapshot id on success, nullopt on persistence/error failure.
     */
    std::optional<uint64_t> createSnapshot(
        const std::string& coordinator_id,
        LSN last_applied_lsn,
        const std::vector<TransactionSnapshotEntry>& active_transactions
    );
    
    /** @brief Load most recent snapshot by snapshot-id ordering. */
    std::optional<TransactionSnapshot> loadLatestSnapshot();
    
    /** @brief Load snapshot by explicit id. */
    std::optional<TransactionSnapshot> loadSnapshot(uint64_t snapshot_id);
    
    /** @brief List all available snapshot ids (newest first). */
    std::vector<uint64_t> listSnapshots();
    
    /** @brief Delete snapshot file by id if it exists. */
    bool deleteSnapshot(uint64_t snapshot_id);
    
    /** @brief Remove oldest snapshot files beyond retention window. */
    void cleanupOldSnapshots();
    
    /** @brief Verify snapshot checksum against serialized payload. */
    bool verifySnapshot(const TransactionSnapshot& snapshot);
    
private:
    std::string snapshot_directory_;
    size_t max_snapshots_;
    
    /** @brief Build absolute snapshot filepath for a snapshot id. */
    std::string getSnapshotPath(uint64_t snapshot_id) const;
    
    /** @brief Compute SHA-256 checksum for serialized JSON payload. */
    std::string calculateChecksum(const nlohmann::json& data) const;
    
    /** @brief Persist snapshot document to disk. */
    bool saveSnapshotToFile(const TransactionSnapshot& snapshot);
    
    /** @brief Load snapshot document from disk path and validate checksum. */
    std::optional<TransactionSnapshot> loadSnapshotFromFile(const std::string& filepath);
};

/** @brief Convert transaction state enum to stable storage string. */
std::string transactionStateToString(TransactionState state);
/** @brief Parse transaction state enum from storage string. */
TransactionState transactionStateFromString(const std::string& str);
/** @brief Convert transaction protocol enum to stable storage string. */
std::string transactionProtocolToString(TransactionProtocol protocol);
/** @brief Parse transaction protocol enum from storage string. */
TransactionProtocol transactionProtocolFromString(const std::string& str);

/** @brief JSON serializer for ParticipantStatus. */
void to_json(nlohmann::json& j, const ParticipantStatus& p);
/** @brief JSON deserializer for ParticipantStatus. */
void from_json(const nlohmann::json& j, ParticipantStatus& p);

/** @brief JSON serializer for SAGAStep. */
void to_json(nlohmann::json& j, const SAGAStep& s);
/** @brief JSON deserializer for SAGAStep. */
void from_json(const nlohmann::json& j, SAGAStep& s);

/** @brief JSON serializer for PercolatorIntent. */
void to_json(nlohmann::json& j, const PercolatorIntent& i);
/** @brief JSON deserializer for PercolatorIntent. */
void from_json(const nlohmann::json& j, PercolatorIntent& i);

/** @brief JSON serializer for TransactionSnapshotEntry. */
void to_json(nlohmann::json& j, const TransactionSnapshotEntry& e);
/** @brief JSON deserializer for TransactionSnapshotEntry. */
void from_json(const nlohmann::json& j, TransactionSnapshotEntry& e);

}  // namespace sharding
