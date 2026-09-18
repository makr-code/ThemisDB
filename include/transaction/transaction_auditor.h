/**
 * @file transaction_auditor.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class TransactionAuditor {
public:
    using TransactionId = uint64_t;

    // ── Status ────────────────────────────────────────────────────────────────

    struct Status {
        bool        ok{true};
        std::string message;
        /**
         * @brief OK.
         * @return Return value.
         * @details Implements OK without additional internal calls.
         */
        static Status OK()                   { return {}; }
        /**
         * @brief Error.
         * @param[in] msg Input parameter.
         * @return Return value.
         * @details Calls: std::move().
         */
        static Status Error(std::string msg) { return {false, std::move(msg)}; }
    };

    // ── Operation ────────────────────────────────────────────────────────────

    struct Operation {
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

        std::optional<std::string> old_value;

        std::optional<std::string> new_value;
    };

    // ── AuditRecord ──────────────────────────────────────────────────────────

    struct AuditRecord {
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


    /**
     * @brief Enable Auditing.
     * @param[in] enabled Input parameter.
     */
    void enableAuditing(bool enabled);

    bool isEnabled() const { return enabled_.load(std::memory_order_acquire); }


    /**
     * @brief Record.
     * @param[in] record Input parameter.
     */
    void record(AuditRecord record);

    // ── Querying ─────────────────────────────────────────────────────────────

    std::vector<AuditRecord> queryAuditLog(
        std::optional<std::string>                           user_id    = std::nullopt,
        std::optional<std::chrono::system_clock::time_point> start_time = std::nullopt,
        std::optional<std::chrono::system_clock::time_point> end_time   = std::nullopt,
        size_t                                               limit      = 1000) const;

    /**
     * @brief Size.
     * @return Return value.
     */
    size_t size() const;

    /**
     * @brief Clear.
     */
    void clear();

    // ── Export transport interface ────────────────────────────────────────────

    struct IAuditExportTransport {
        /**
         * @brief IAudit Export Transport.
         * @return Return value.
         */
        virtual ~IAuditExportTransport() = default;

        /**
         * @brief Send Kafka.
         * @param[in] topic Input parameter.
         * @param[in] ndjson_payload Input parameter.
         * @return Return value.
         */
        virtual Status sendKafka(const std::string& topic,
                                 const std::string& ndjson_payload) = 0;

        /**
         * @brief Write S3.
         * @param[in] bucket Input parameter.
         * @param[in] key Input parameter.
         * @param[in] ndjson_payload Input parameter.
         * @return Return value.
         */
        virtual Status writeS3(const std::string& bucket,
                               const std::string& key,
                               const std::string& ndjson_payload) = 0;
    };

    /**
     * @brief Set Export Transport.
     * @param[in,out] transport Input/output parameter.
     */
    void setExportTransport(IAuditExportTransport* transport);


    /**
     * @brief Export To Kafka.
     * @param[in] topic Input parameter.
     * @return Return value.
     */
    Status exportToKafka(const std::string& topic);

    /**
     * @brief Export To S3.
     * @param[in] bucket Input parameter.
     * @param[in] prefix Input parameter.
     * @return Return value.
     */
    Status exportToS3(const std::string& bucket, const std::string& prefix);

private:
    std::atomic<bool>        enabled_{false};
    mutable std::mutex       log_mutex_;
    std::vector<AuditRecord> log_; ///< Append-only in-memory audit log
    IAuditExportTransport*   export_transport_{nullptr};  ///< Injected export transport (not owned)
};

} // namespace themis
