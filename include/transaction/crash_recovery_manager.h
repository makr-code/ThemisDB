/**
 * @file crash_recovery_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.45
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

    struct OperationEntry {
        std::string op;           ///< "put" | "del"
        std::string key;          ///< Key affected
        std::string old_value;    ///< Value before the op (empty if key did not exist)
        std::string new_value;    ///< Value after the op  (empty for "del")
        bool        had_old{false};///< true → old_value is valid (key existed before op)
    };

    struct LogEntry {
        int64_t        timestamp_ms{0};
        uint64_t       txn_id{0};
        EntryType      type{EntryType::BEGIN};
        IsolationLevel isolation{IsolationLevel::READ_COMMITTED}; // BEGIN only
        OperationEntry operation;  // OPERATION only
    };

    // ── Result types ─────────────────────────────────────────────────────────

    struct RecoveryResult {
        bool   success{false};
        size_t in_flight_found{0};       ///< In-flight txns detected in WAL
        size_t rolled_back{0};           ///< Transactions successfully rolled back
        size_t operations_undone{0};     ///< Individual operations undone
        std::vector<uint64_t> rolled_back_ids; ///< IDs of recovered transactions
        std::string message;

        bool hadWorkToDo() const { return in_flight_found > 0; }
    };

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

    explicit CrashRecoveryManager(const std::string& wal_path,
                                   bool sync_on_write = true);
    ~CrashRecoveryManager();

    CrashRecoveryManager(const CrashRecoveryManager&) = delete;
    CrashRecoveryManager& operator=(const CrashRecoveryManager&) = delete;


    /**
     * @brief Log Begin.
     * @param[in] txn_id Identifier of the txn.
     * @param[in] isolation Input parameter.
     */
    void logBegin(uint64_t txn_id, IsolationLevel isolation);

    /**
     * @brief Log Operation.
     * @param[in] txn_id Identifier of the txn.
     * @param[in] op Input parameter.
     * @param[in] key Input parameter.
     * @param[in] old_value Input parameter.
     * @param[in] new_value Input parameter.
     */
    void logOperation(uint64_t txn_id,
                      const std::string& op,
                      const std::string& key,
                      const std::optional<std::string>& old_value,
                      const std::optional<std::string>& new_value);

    /**
     * @brief Log Commit.
     * @param[in] txn_id Identifier of the txn.
     */
    void logCommit(uint64_t txn_id);

    /**
     * @brief Log Abort.
     * @param[in] txn_id Identifier of the txn.
     */
    void logAbort(uint64_t txn_id);


    /**
     * @brief Needs Recovery.
     * @return True when the operation succeeds.
     */
    bool needsRecovery() const;

    /**
     * @brief Recover.
     * @param[in,out] db Input/output parameter.
     * @return Return value.
     */
    RecoveryResult recover(RocksDBWrapper& db);

    /**
     * @brief Get In Flight Transaction Ids.
     * @return Return value.
     */
    std::vector<uint64_t> getInFlightTransactionIds() const;


    /**
     * @brief Prune Log.
     * @return Return value.
     */
    size_t pruneLog();

    /**
     * @brief Get Metrics.
     * @return Return value.
     */
    RecoveryMetrics getMetrics() const;


    /**
     * @brief Read All Entries.
     * @return Return value.
     */
    std::vector<LogEntry> readAllEntries() const;

    /**
     * @brief Pending Transaction Count.
     * @return Return value.
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

    /**
     * @brief Append Line.
     * @param[in] json_line Input parameter.
     */
    void appendLine(const std::string& json_line);

    /**
     * @brief Scan In Flight.
     * @return Return value.
     */
    std::unordered_set<uint64_t> scanInFlight() const;

    /**
     * @brief Serialize.
     * @param[in] e Input parameter.
     * @return Return value.
     */
    static std::string serialize(const LogEntry& e);

    /**
     * @brief Deserialize.
     * @param[in] line Input parameter.
     * @return Return value.
     */
    static std::optional<LogEntry> deserialize(const std::string& line);

    /**
     * @brief Base64 Encode.
     * @param[in] s Input parameter.
     * @return Return value.
     */
    static std::string base64Encode(const std::string& s);

    /**
     * @brief Base64 Decode.
     * @param[in] s Input parameter.
     * @return Return value.
     */
    static std::string base64Decode(const std::string& s);

    /**
     * @brief Now Ms.
     * @return Return value.
     */
    static int64_t nowMs();
};

} // namespace transaction
} // namespace themis

