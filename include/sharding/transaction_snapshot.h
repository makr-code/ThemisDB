/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            transaction_snapshot.h                             ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:13:48                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     193                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 8cf91c826b  2026-03-01  feat: implement Calvin protocol for deterministic distrib... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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

// Transaction state for snapshot
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

// Participant status in transaction
struct ParticipantStatus {
    std::string participant_id;
    bool prepared = false;          // For 2PC/3PC
    bool pre_committed = false;     // For 3PC only
    bool committed = false;
    bool aborted = false;
    std::string response_data;
    uint64_t timestamp = 0;
};

// SAGA compensation step
struct SAGAStep {
    uint32_t step_number;
    std::string operation;
    nlohmann::json data;
    bool completed = false;
    bool compensated = false;
    uint64_t timestamp = 0;
};

// Percolator write intent
struct PercolatorIntent {
    std::string key;
    nlohmann::json value;
    uint64_t start_timestamp;
    bool locked = false;
};

// Single transaction snapshot state
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

// Complete snapshot of all active transactions
struct TransactionSnapshot {
    uint64_t snapshot_id;
    LSN last_applied_lsn;
    std::string coordinator_id;
    uint64_t timestamp;
    std::vector<TransactionSnapshotEntry> active_transactions;
    std::string checksum;  // SHA-256
    size_t total_transactions;
    
    // Serialize to JSON
    nlohmann::json toJson() const;
    
    // Deserialize from JSON
    static std::optional<TransactionSnapshot> fromJson(const nlohmann::json& j);
};

// Manager for transaction snapshots
class TransactionSnapshotManager {
public:
    TransactionSnapshotManager(const std::string& snapshot_directory, size_t max_snapshots = 10);
    ~TransactionSnapshotManager() = default;
    
    // Create a new snapshot
    std::optional<uint64_t> createSnapshot(
        const std::string& coordinator_id,
        LSN last_applied_lsn,
        const std::vector<TransactionSnapshotEntry>& active_transactions
    );
    
    // Load the latest snapshot
    std::optional<TransactionSnapshot> loadLatestSnapshot();
    
    // Load a specific snapshot by ID
    std::optional<TransactionSnapshot> loadSnapshot(uint64_t snapshot_id);
    
    // List all available snapshots
    std::vector<uint64_t> listSnapshots();
    
    // Delete a specific snapshot
    bool deleteSnapshot(uint64_t snapshot_id);
    
    // Clean up old snapshots (keep only max_snapshots)
    void cleanupOldSnapshots();
    
    // Verify snapshot integrity
    bool verifySnapshot(const TransactionSnapshot& snapshot);
    
private:
    std::string snapshot_directory_;
    size_t max_snapshots_;
    
    // Generate snapshot filename
    std::string getSnapshotPath(uint64_t snapshot_id) const;
    
    // Calculate SHA-256 checksum
    std::string calculateChecksum(const nlohmann::json& data) const;
    
    // Save snapshot to file
    bool saveSnapshotToFile(const TransactionSnapshot& snapshot);
    
    // Load snapshot from file
    std::optional<TransactionSnapshot> loadSnapshotFromFile(const std::string& filepath);
};

// Helper functions for enum conversion
std::string transactionStateToString(TransactionState state);
TransactionState transactionStateFromString(const std::string& str);
std::string transactionProtocolToString(TransactionProtocol protocol);
TransactionProtocol transactionProtocolFromString(const std::string& str);

// JSON serialization helpers
void to_json(nlohmann::json& j, const ParticipantStatus& p);
void from_json(const nlohmann::json& j, ParticipantStatus& p);

void to_json(nlohmann::json& j, const SAGAStep& s);
void from_json(const nlohmann::json& j, SAGAStep& s);

void to_json(nlohmann::json& j, const PercolatorIntent& i);
void from_json(const nlohmann::json& j, PercolatorIntent& i);

void to_json(nlohmann::json& j, const TransactionSnapshotEntry& e);
void from_json(const nlohmann::json& j, TransactionSnapshotEntry& e);

}  // namespace sharding
