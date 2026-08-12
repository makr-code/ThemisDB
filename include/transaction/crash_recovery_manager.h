/**
 * @file crash_recovery_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.45
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <mutex>
#include <atomic>
#include <chrono>
#include <functional>
#include <cstdint>
#include "transaction/isolation_level.h"

namespace themis {

// Forward declaration
class RocksDBWrapper;

namespace transaction {

/**
 * @brief Phase 8 – Durability & Crash-Recovery
 *
 * CrashRecoveryManager provides a transaction-level Write-Ahead Log (WAL)
 * that records every transaction lifecycle event and every individual write
 * operation (with its "before" value for undo).
 *
 * ## WAL layout (one append-only file per session)
 * Each entry is a newline-terminated JSON record:
 *   {"t":<ms>,"txn":<id>,"type":"BEGIN"|"OP"|"COMMIT"|"ABORT",
 *    "iso":<int>,          // BEGIN only
 *    "op":"put"|"del",     // OP only
 *    "key":"...",          // OP only
 *    "old":"...",          // OP only – base64 for undo
 *    "new":"..."}          // OP only – base64 for redo
 *
 * ## Recovery procedure (called at startup)
 * 1. Scan the WAL file for all BEGIN entries that do NOT have a matching
 *    COMMIT or ABORT entry → these are in-flight transactions.
 * 2. For each in-flight transaction, replay its OPerations in reverse order,
 *    writing the "old" value back (or deleting the key when old is absent).
 * 3. Mark the recovery as complete and append a CHECKPOINT record so that
 *    the next startup can skip already-recovered entries.
 *
 * ## Thread-Safety
 * All public methods are thread-safe.
 */
class CrashRecoveryManager {
public:
    // ── Types ────────────────────────────────────────────────────────────────

    enum class EntryType : uint8_t {
        BEGIN      = 0,  ///< Transaction start
        OPERATION  = 1,  ///< Single write within a transaction (undo/redo log)
        COMMIT     = 2,  ///< Transaction committed successfully
        ABORT      = 3,  ///< Transaction rolled back / aborted
        CHECKPOINT = 4,  ///< Recovery checkpoint – entries before this are safe to prune
    };

    /// A single undo/redo operation logged inside a transaction.
    struct OperationEntry {
        std::string op;           ///< "put" | "del"
        std::string key;          ///< Key affected
        std::string old_value;    ///< Value before the op (empty if key did not exist)
        std::string new_value;    ///< Value after the op  (empty for "del")
        bool        had_old{false};///< true → old_value is valid (key existed before op)
    };

    /// A WAL record.
    struct LogEntry {
        int64_t        timestamp_ms{0};
        uint64_t       txn_id{0};
        EntryType      type{EntryType::BEGIN};
        IsolationLevel isolation{IsolationLevel::READ_COMMITTED}; // BEGIN only
        OperationEntry operation;  // OPERATION only
    };

    // ── Result types ─────────────────────────────────────────────────────────

    /// Result returned by recover().
    struct RecoveryResult {
        bool   success{false};
        size_t in_flight_found{0};       ///< In-flight txns detected in WAL
        size_t rolled_back{0};           ///< Transactions successfully rolled back
        size_t operations_undone{0};     ///< Individual operations undone
        std::vector<uint64_t> rolled_back_ids; ///< IDs of recovered transactions
        std::string message;

        bool hadWorkToDo() const { return in_flight_found > 0; }
    };

    /// Aggregate metrics for monitoring.
    struct RecoveryMetrics {
        uint64_t total_begins_logged{0};
        uint64_t total_operations_logged{0};
        uint64_t total_commits_logged{0};
        uint64_t total_aborts_logged{0};
        uint64_t wal_prune_count{0};

        // Last recovery run
        bool     last_recovery_ran{false};
        size_t   last_in_flight_found{0};
        size_t   last_rolled_back{0};
        size_t   last_operations_undone{0};
    };

    // ── Constructor / destructor ──────────────────────────────────────────────

    /**
     * @brief Construct a CrashRecoveryManager.
     * @param wal_path  Full path to the WAL file.  Created on first write.
     * @param sync_on_write  If true, fdatasync after every append (safe but slower).
     */
    explicit CrashRecoveryManager(const std::string& wal_path,
                                   bool sync_on_write = true);
    ~CrashRecoveryManager();

    CrashRecoveryManager(const CrashRecoveryManager&) = delete;
    CrashRecoveryManager& operator=(const CrashRecoveryManager&) = delete;

    // ── WAL write methods (called by TransactionManager) ─────────────────────

    /**
     * @brief Log the start of a new transaction.
     * @param txn_id    Transaction identifier.
     * @param isolation Isolation level for this transaction.
     */
    void logBegin(uint64_t txn_id, IsolationLevel isolation);

    /**
     * @brief Log a write operation within a transaction.
     *
     * Should be called *before* the write is applied to the DB so that
     * the old value is still readable.
     *
     * @param txn_id     Transaction identifier.
     * @param op         "put" or "del".
     * @param key        Key being written.
     * @param old_value  Previous value for the key (nullopt if key was absent).
     * @param new_value  New value for the key (nullopt for "del").
     */
    void logOperation(uint64_t txn_id,
                      const std::string& op,
                      const std::string& key,
                      const std::optional<std::string>& old_value,
                      const std::optional<std::string>& new_value);

    /**
     * @brief Log a successful commit.
     * @param txn_id Transaction identifier.
     */
    void logCommit(uint64_t txn_id);

    /**
     * @brief Log an abort / rollback.
     * @param txn_id Transaction identifier.
     */
    void logAbort(uint64_t txn_id);

    // ── Recovery methods ──────────────────────────────────────────────────────

    /**
     * @brief Check whether the WAL contains in-flight transactions.
     *
     * Should be called at startup *before* accepting new transactions.
     * Returns true if recover() needs to be run.
     */
    bool needsRecovery() const;

    /**
     * @brief Perform crash recovery.
     *
     * Scans the WAL file for uncommitted transactions and undoes their
     * operations by writing old values (or deleting new keys) via @p db.
     * Appends a CHECKPOINT entry when done.
     *
     * @param db  RocksDB wrapper used to apply undo operations.
     * @return    Summary of the recovery run.
     */
    RecoveryResult recover(RocksDBWrapper& db);

    /**
     * @brief Get the IDs of transactions that were in-flight at last scan.
     *
     * Updated by needsRecovery() and recover().
     */
    std::vector<uint64_t> getInFlightTransactionIds() const;

    // ── Maintenance ───────────────────────────────────────────────────────────

    /**
     * @brief Remove log entries for committed/aborted transactions to keep
     *        the WAL file small.
     *
     * Rewrites the WAL file keeping only:
     * - Entries for currently in-flight transactions.
     * - The final CHECKPOINT entry.
     *
     * @return Number of entries removed.
     */
    size_t pruneLog();

    /**
     * @brief Aggregate runtime metrics.
     */
    RecoveryMetrics getMetrics() const;

    // ── Introspection (for testing / monitoring) ──────────────────────────────

    /**
     * @brief Read all log entries from the WAL file.
     */
    std::vector<LogEntry> readAllEntries() const;

    /**
     * @brief Number of entries currently in the in-memory pending map
     *        (transactions started but not yet committed/aborted).
     */
    size_t pendingTransactionCount() const;

private:
    std::string wal_path_;
    bool        sync_on_write_;

    mutable std::mutex mutex_;

    // In-memory tracking of active (not yet committed/aborted) transactions.
    // txn_id → ordered list of logged operation entries.
    std::unordered_map<uint64_t, std::vector<OperationEntry>> pending_ops_;
    std::unordered_set<uint64_t> committed_ids_;
    std::unordered_set<uint64_t> aborted_ids_;

    // Metrics
    std::atomic<uint64_t> metric_begins_{0};
    std::atomic<uint64_t> metric_ops_{0};
    std::atomic<uint64_t> metric_commits_{0};
    std::atomic<uint64_t> metric_aborts_{0};
    std::atomic<uint64_t> metric_prunes_{0};

    // Cache of in-flight ids after last scan (mutable for const methods)
    mutable std::vector<uint64_t> last_in_flight_ids_;

    /// Append a raw JSON line to the WAL file (mutex must be held).
    void appendLine(const std::string& json_line);

    /// Scan WAL to find in-flight transaction IDs.
    std::unordered_set<uint64_t> scanInFlight() const;

    /// Serialize a LogEntry to a JSON string (single line, no newline).
    static std::string serialize(const LogEntry& e);

    /// Deserialize a JSON string into a LogEntry.  Returns nullopt on error.
    static std::optional<LogEntry> deserialize(const std::string& line);

    /// Base64-encode raw bytes (for storing binary values in JSON).
    static std::string base64Encode(const std::string& s);

    /// Base64-decode a base64 string back to raw bytes.
    static std::string base64Decode(const std::string& s);

    /// Current monotonic timestamp in milliseconds.
    static int64_t nowMs();
};

} // namespace transaction
} // namespace themis

