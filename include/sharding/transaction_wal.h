/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            transaction_wal.h                                  ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:13:48                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     254                                            ║
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

#include "sharding/wal_manager.h"
#include <nlohmann/json.hpp>
#include <memory>
#include <string>
#include <vector>
#include <optional>

namespace sharding {

using LSN = themis::sharding::LSN;
using WALEntry = themis::sharding::WALEntry;
using WALManager = themis::sharding::WALManager;

/**
 * Transaction protocol types
 */
enum class TransactionProtocol {
    TWO_PHASE_COMMIT,
    THREE_PHASE_COMMIT,
    SAGA,
    PERCOLATOR,
    CALVIN           // Deterministic distributed transactions via pre-ordering
};

/**
 * Transaction WAL entry types
 * Base type starts at 30, actual types are 130-138 (base + 100)
 */
enum class TransactionWALEntryType {
    BEGIN = 130,      // Transaction started
    PREPARE = 131,    // Prepare request sent to participant
    PREPARED = 132,   // Participant voted (prepared or aborted)
    COMMIT = 133,     // Coordinator decided to commit
    COMMITTED = 134,  // Participant confirmed commit
    ABORT = 135,      // Coordinator decided to abort
    ABORTED = 136,    // Participant confirmed abort
    COMPENSATE = 137  // SAGA compensation step
};

/**
 * Transaction WAL entry structure
 */
struct TransactionWALEntry {
    LSN lsn;
    TransactionWALEntryType type;
    uint64_t timestamp;
    std::string transaction_id;
    TransactionProtocol protocol;
    std::vector<std::string> participants;
    std::string participant_id;  // For participant-specific entries
    nlohmann::json data;         // Protocol-specific data
    bool vote;                   // For PREPARED entries (true=yes, false=no)
    std::string reason;          // For ABORT/COMPENSATE entries

    TransactionWALEntry()
                : lsn(0, 0), type(TransactionWALEntryType::BEGIN), timestamp(0),
          protocol(TransactionProtocol::TWO_PHASE_COMMIT), vote(false) {}
};

/**
 * Configuration for Transaction WAL
 */
struct TransactionWALConfig {
    std::string wal_directory;
    std::string snapshot_directory;
    size_t segment_size = 16 * 1024 * 1024;  // 16 MB
    uint64_t snapshot_interval = 1000;        // Snapshot every 1K transactions
    size_t max_snapshots = 10;                // Keep last 10 snapshots
    bool sync_on_write = true;                // fsync after each write
};

/**
 * Transaction Write-Ahead Log
 * 
 * Provides durable logging for distributed transaction coordinator state.
 * Supports 2PC, 3PC, SAGA, and Percolator protocols.
 * 
 * Usage:
 *   TransactionWALConfig config;
 *   config.wal_directory = "./data/transactions/wal";
 *   
 *   TransactionWAL wal(config);
 *   wal.initialize();
 *   
 *   // Log transaction lifecycle
 *   LSN lsn1 = wal.logBegin(txn_id, protocol, participants);
 *   LSN lsn2 = wal.logPrepare(txn_id, participant, data);
 *   LSN lsn3 = wal.logPrepared(txn_id, participant, true, response);
 *   LSN lsn4 = wal.logCommit(txn_id, data);
 *   LSN lsn5 = wal.logCommitted(txn_id, participant);
 *   
 *   // For recovery
 *   auto entries = wal.readEntries(start_lsn);
 */
class TransactionWAL {
public:
    explicit TransactionWAL(const TransactionWALConfig& config);
    ~TransactionWAL();

    /**
     * Initialize the WAL
     * Creates directories and sets up WAL manager
     */
    bool initialize();

    /**
     * Log transaction begin
     * 
     * @param transaction_id Unique transaction identifier
     * @param protocol Transaction protocol (2PC, 3PC, SAGA, PERCOLATOR)
     * @param participants List of participant shard IDs
     * @return LSN of the logged entry
     */
    LSN logBegin(const std::string& transaction_id,
                 TransactionProtocol protocol,
                 const std::vector<std::string>& participants);

    /**
     * Log prepare request sent to participant
     * 
     * @param transaction_id Transaction identifier
     * @param participant_id Participant shard ID
     * @param data Protocol-specific prepare data
     * @return LSN of the logged entry
     */
    LSN logPrepare(const std::string& transaction_id,
                   const std::string& participant_id,
                   const nlohmann::json& data);

    /**
     * Log participant prepare response (vote)
     * 
     * @param transaction_id Transaction identifier
     * @param participant_id Participant shard ID
     * @param vote true=prepared/yes, false=aborted/no
     * @param response Response data from participant
     * @return LSN of the logged entry
     */
    LSN logPrepared(const std::string& transaction_id,
                    const std::string& participant_id,
                    bool vote,
                    const std::string& response);

    /**
     * Log coordinator commit decision
     * 
     * @param transaction_id Transaction identifier
     * @param data Commit data
     * @return LSN of the logged entry
     */
    LSN logCommit(const std::string& transaction_id,
                  const nlohmann::json& data);

    /**
     * Log participant commit confirmation
     * 
     * @param transaction_id Transaction identifier
     * @param participant_id Participant shard ID
     * @return LSN of the logged entry
     */
    LSN logCommitted(const std::string& transaction_id,
                     const std::string& participant_id);

    /**
     * Log coordinator abort decision
     * 
     * @param transaction_id Transaction identifier
     * @param reason Reason for abort
     * @return LSN of the logged entry
     */
    LSN logAbort(const std::string& transaction_id,
                 const std::string& reason);

    /**
     * Log participant abort confirmation
     * 
     * @param transaction_id Transaction identifier
     * @param participant_id Participant shard ID
     * @return LSN of the logged entry
     */
    LSN logAborted(const std::string& transaction_id,
                   const std::string& participant_id);

    /**
     * Log SAGA compensation step
     * 
     * @param transaction_id Transaction identifier
     * @param step_id Step identifier
     * @param compensation_data Compensation data
     * @return LSN of the logged entry
     */
    LSN logCompensate(const std::string& transaction_id,
                      const std::string& step_id,
                      const nlohmann::json& compensation_data);

    /**
     * Read WAL entries starting from a given LSN
     * 
     * @param start_lsn Starting LSN (0 = from beginning)
     * @return Vector of WAL entries
     */
    std::vector<TransactionWALEntry> readEntries(LSN start_lsn = LSN(0, 0));

    /**
     * Check if snapshot should be created
     * 
     * @param operations_count Number of operations since last snapshot
     * @return true if snapshot should be created
     */
    bool shouldCreateSnapshot(uint64_t operations_count) const;

    /**
     * Get current LSN
     */
    LSN getCurrentLSN() const;

private:
    TransactionWALConfig config_;
    std::unique_ptr<WALManager> wal_manager_;
    LSN current_lsn_;

    // Helper to convert TransactionWALEntry to WALEntry
    WALEntry toWALEntry(const TransactionWALEntry& txn_entry);

    // Helper to convert WALEntry to TransactionWALEntry
    std::optional<TransactionWALEntry> fromWALEntry(const WALEntry& wal_entry);
};

} // namespace sharding
