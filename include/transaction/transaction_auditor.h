/**
 * @file transaction_auditor.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=2, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
 * @note exportToKafka() and exportToS3() require an injected
 *       IAuditExportTransport (via setExportTransport()).  Without a
 *       transport they return Status::Error("export transport not configured").
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
    TransactionAuditor(TransactionAuditor&&)                 noexcept = default;
    TransactionAuditor& operator=(TransactionAuditor&&)      noexcept = default;

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

    // ── Export transport interface ────────────────────────────────────────────

    /**
     * @brief Pluggable transport interface for exporting audit records.
     *
     * Inject a concrete implementation via setExportTransport() to enable
     * exportToKafka() and exportToS3().  In production, this would wrap
     * librdkafka or aws-sdk-cpp.  In tests, a spy/mock can verify calls.
     *
     * Each method receives the serialised payload as newline-delimited JSON
     * (NDJSON) so the caller can forward it verbatim.
     */
    struct IAuditExportTransport {
        virtual ~IAuditExportTransport() = default;

        /**
         * Publish @p ndjson_payload to a Kafka @p topic.
         * @return Status::OK() on success; Status::Error(...) on failure.
         */
        virtual Status sendKafka(const std::string& topic,
                                 const std::string& ndjson_payload) = 0;

        /**
         * Write @p ndjson_payload to the S3-compatible object at
         * @p bucket / @p key.
         * @return Status::OK() on success; Status::Error(...) on failure.
         */
        virtual Status writeS3(const std::string& bucket,
                               const std::string& key,
                               const std::string& ndjson_payload) = 0;
    };

    /**
     * @brief Inject an export transport.
     *
     * The auditor holds a raw pointer — the caller is responsible for keeping
     * the transport alive for the lifetime of this auditor.  Pass nullptr to
     * remove a previously set transport.
     */
    void setExportTransport(IAuditExportTransport* transport);

    // ── Export ───────────────────────────────────────────────────────────────

    /**
     * @brief Export the audit log to a Kafka topic.
     *
     * Serialises all in-memory AuditRecords as newline-delimited JSON (NDJSON)
     * and publishes the payload to @p topic via the injected
     * IAuditExportTransport.
     *
     * @param topic  Kafka topic name.
     * @return Status::OK() on success.
     *         Status::Error("export transport not configured") when no
     *         transport has been set via setExportTransport().
     */
    Status exportToKafka(const std::string& topic);

    /**
     * @brief Export the audit log to an S3-compatible object store.
     *
     * Serialises all in-memory AuditRecords as newline-delimited JSON and
     * writes the resulting object to @p bucket using a key derived from
     * @p prefix and the current UTC timestamp
     * (e.g. "logs/audit_20260416T115000Z.ndjson").
     *
     * @param bucket  S3 bucket name.
     * @param prefix  Key prefix (directory path) inside the bucket.
     * @return Status::OK() on success.
     *         Status::Error("export transport not configured") when no
     *         transport has been set via setExportTransport().
     */
    Status exportToS3(const std::string& bucket, const std::string& prefix);

private:
    std::atomic<bool>        enabled_{false};
    mutable std::mutex       log_mutex_;
    std::vector<AuditRecord> log_; ///< Append-only in-memory audit log
    IAuditExportTransport*   export_transport_{nullptr};  ///< Injected export transport (not owned)
};

} // namespace themis
