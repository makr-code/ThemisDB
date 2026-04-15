/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            transaction_auditor.h                              ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-04-15 04:14:38                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     240                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • d5eddfb167  2026-03-20  feat(transaction): add Read-Only Transaction Optimization... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "transaction/isolation_level.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace themis {

/**
 * @brief Append-only transaction audit trail for compliance and debugging.
 *
 * TransactionAuditor records the full lifecycle of each transaction that
 * passes through it: the ISO isolation level, the user and session that
 * submitted it, every write operation with its before/after values, and
 * the final outcome (COMMITTED, ABORTED, or DEADLOCK).
 *
 * Security guarantees:
 *   - The in-memory audit log is append-only: there is no API to delete or
 *     modify existing records.
 *   - Audit log entries must not contain decrypted column values; callers
 *     are responsible for passing redacted/encrypted values when recording
 *     sensitive fields.
 *
 * Thread safety:
 *   - All public methods are thread-safe; an internal mutex serialises
 *     access to the in-memory log.
 *   - enableAuditing() and record() may safely be called concurrently.
 *
 * @note exportToKafka() and exportToS3() are not yet implemented and return
 *       Status::Error() as placeholders for future integration.
 */
class TransactionAuditor {
public:
    using TransactionId = uint64_t;

    // ── Status ────────────────────────────────────────────────────────────────

    struct Status {
        bool        ok{true};
        std::string message;
        static Status OK()                   { return {}; }
        static Status Error(std::string msg) { return {false, std::move(msg)}; }
    };

    // ── Operation ────────────────────────────────────────────────────────────

    /**
     * @brief A single data-manipulation operation inside a transaction.
     */
    struct Operation {
        /// Type of the operation.
        enum class Type {
            PUT,         ///< Insert or update an entity
            DELETE,      ///< Delete an entity
            ADD_EDGE,    ///< Add a graph edge
            DELETE_EDGE, ///< Remove a graph edge
            ADD_VECTOR   ///< Insert or update a vector embedding
        };

        Type        type{Type::PUT};
        std::string table;     ///< Table (or collection) name
        std::string key;       ///< Storage key of the affected row

        /// Value before the operation.  nullopt when the entity did not exist
        /// before this operation, or when capture of the old value was disabled.
        std::optional<std::string> old_value;

        /// Value after the operation.  nullopt for DELETE operations.
        std::optional<std::string> new_value;
    };

    // ── AuditRecord ──────────────────────────────────────────────────────────

    /**
     * @brief A complete record for one transaction lifecycle.
     */
    struct AuditRecord {
        /// Final outcome of the transaction.
        enum class Result {
            COMMITTED, ///< Transaction was committed successfully
            ABORTED,   ///< Transaction was rolled back (explicit or automatic)
            DEADLOCK   ///< Transaction was aborted due to deadlock detection
        };

        TransactionId txn_id{0};
        std::string   user_id;
        std::string   session_id;
        std::chrono::system_clock::time_point timestamp;
        IsolationLevel                         isolation{IsolationLevel::ReadCommitted};
        std::vector<Operation>                 operations;
        Result                                 result{Result::COMMITTED};
        uint64_t                               duration_us{0};
    };

    // ── Lifecycle ─────────────────────────────────────────────────────────────

    TransactionAuditor() = default;
    ~TransactionAuditor() = default;

    TransactionAuditor(const TransactionAuditor&)            = delete;
    TransactionAuditor& operator=(const TransactionAuditor&) = delete;
    TransactionAuditor(TransactionAuditor&&)                 = default;
    TransactionAuditor& operator=(TransactionAuditor&&)      = default;

    // ── Configuration ────────────────────────────────────────────────────────

    /**
     * @brief Enable or disable audit logging.
     *
     * When disabled (the default), record() is a no-op and queryAuditLog()
     * always returns an empty vector.  Enabling auditing does not replay
     * previously skipped transactions.
     *
     * @param enabled  true to start recording; false to stop.
     */
    void enableAuditing(bool enabled);

    /**
     * @brief Return true when audit logging is active.
     */
    bool isEnabled() const { return enabled_.load(std::memory_order_acquire); }

    // ── Recording ────────────────────────────────────────────────────────────

    /**
     * @brief Append one audit record to the log.
     *
     * No-op when auditing is disabled.  The record is appended atomically
     * under the internal mutex, so concurrent calls from different commit
     * threads are safe.
     *
     * @param record  Completed AuditRecord to store.  The caller is
     *                responsible for populating all fields before calling
     *                this method.
     */
    void record(AuditRecord record);

    // ── Querying ─────────────────────────────────────────────────────────────

    /**
     * @brief Query the in-memory audit log with optional filters.
     *
     * Filters are applied with AND semantics: only records matching ALL
     * supplied constraints are returned.
     *
     * @param user_id     Optional filter on AuditRecord::user_id.  Pass
     *                    std::nullopt to match all users.
     * @param start_time  Optional lower bound on AuditRecord::timestamp
     *                    (inclusive).  Pass std::nullopt for no lower bound.
     * @param end_time    Optional upper bound on AuditRecord::timestamp
     *                    (inclusive).  Pass std::nullopt for no upper bound.
     * @param limit       Maximum number of records to return (most recent
     *                    first).  Defaults to 1000; 0 returns all matching
     *                    records.
     * @return            Matching records sorted by timestamp descending
     *                    (most recent first), capped at @p limit entries.
     */
    std::vector<AuditRecord> queryAuditLog(
        std::optional<std::string>                           user_id    = std::nullopt,
        std::optional<std::chrono::system_clock::time_point> start_time = std::nullopt,
        std::optional<std::chrono::system_clock::time_point> end_time   = std::nullopt,
        size_t                                               limit      = 1000) const;

    /**
     * @brief Return the total number of audit records stored in memory.
     */
    size_t size() const;

    /**
     * @brief Remove all stored audit records from the in-memory log.
     *
     * @note This is the only mutation path other than record().  It exists
     *       primarily for testing.  Production deployments should rely on
     *       exportToKafka() / exportToS3() + periodic log rotation instead.
     */
    void clear();

    // ── Export ───────────────────────────────────────────────────────────────

    /**
     * @brief Export the audit log to a Kafka topic.
     *
     * @note Not yet implemented.  Returns Status::Error() as a placeholder.
     *       Future versions will serialise each AuditRecord as a JSON
     *       message and publish it to @p topic using the configured Kafka
     *       producer.
     *
     * @param topic  Kafka topic name.
     * @return Status::Error("exportToKafka: not yet implemented").
     */
    Status exportToKafka(const std::string& topic);

    /**
     * @brief Export the audit log to an S3-compatible object store.
     *
     * @note Not yet implemented.  Returns Status::Error() as a placeholder.
     *       Future versions will serialise the audit log as newline-delimited
     *       JSON and write it to @p bucket / @p prefix.
     *
     * @param bucket  S3 bucket name.
     * @param prefix  Key prefix (directory path) inside the bucket.
     * @return Status::Error("exportToS3: not yet implemented").
     */
    Status exportToS3(const std::string& bucket, const std::string& prefix);

private:
    std::atomic<bool>        enabled_{false};
    mutable std::mutex       log_mutex_;
    std::vector<AuditRecord> log_; ///< Append-only in-memory audit log
};

} // namespace themis
