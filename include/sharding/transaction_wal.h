/**
 * @file transaction_wal.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

/** @brief Distributed transaction protocol families recorded in transaction WAL. */
enum class TransactionProtocol {
    TWO_PHASE_COMMIT,
    THREE_PHASE_COMMIT,
    SAGA,
    PERCOLATOR,
    CALVIN           // Deterministic distributed transactions via pre-ordering
};

/** @brief Transaction WAL entry kinds persisted by coordinator/participants. */
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

/** @brief Canonical transaction WAL payload after decode from base WAL entry. */
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

/** @brief Runtime configuration for transaction WAL path, retention and fsync policy. */
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
    /** @brief Construct transaction WAL facade with immutable configuration. */
    explicit TransactionWAL(const TransactionWALConfig& config);
    /** @brief Destroy transaction WAL facade and owned WAL manager. */
    ~TransactionWAL();

    /** @brief Initialize directories and underlying WAL manager instance. */
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

    /** @brief Return whether operation count reached configured snapshot interval. */
    bool shouldCreateSnapshot(uint64_t operations_count) const;

    /** @brief Return latest known WAL LSN from manager (or cached fallback). */
    LSN getCurrentLSN() const;

private:
    TransactionWALConfig config_;
    std::unique_ptr<WALManager> wal_manager_;
    // TWAL-1: current_lsn_ is updated from any calling thread; protect with mutex.
    mutable std::mutex lsn_mutex_;
    LSN current_lsn_;

    /** @brief Convert transaction WAL payload into generic WAL entry wire shape. */
    WALEntry toWALEntry(const TransactionWALEntry& txn_entry);

    /** @brief Decode generic WAL entry into transaction WAL payload if compatible. */
    std::optional<TransactionWALEntry> fromWALEntry(const WALEntry& wal_entry);
};

} // namespace sharding
