/**
 * @file audit_trail.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace themis {
namespace importers {

using json = nlohmann::json;

enum class AuditEventType {
    IMPORT_STARTED,             ///< Import session started
    SCHEMA_ANALYZED,            ///< Schema inference completed
    RELATIONSHIP_MAPPED,        ///< Entity relationships established
    DATA_VALIDATED,             ///< Data validation completed
    CONFLICT_DETECTED,          ///< Conflict detected (Phase 2 T2.3)
    CONFLICT_RESOLVED,          ///< Conflict resolved with strategy (Phase 2 T2.3)
    QUALITY_CHECK_FAILED,       ///< Quality check failed (Phase 2 T2.3)
    QUALITY_GATE_BYPASSED,      ///< Quality gate bypassed with reason (Phase 2 T2.3)
    SCHEMA_VALIDATION_FAILED,   ///< Schema validation failed (Phase 2 T2.3)
    RECORD_IMPORTED,            ///< Record successfully imported
    IMPORT_COMPLETED,           ///< Import session completed
    IMPORT_ROLLBACK_REQUESTED,  ///< Import rollback initiated (Phase 2 T2.3)
    ERROR_OCCURRED              ///< Generic error occurred
};

constexpr size_t kMaxAuditBufferSize = 100000;  ///< Maximum events per process

/**
 * @brief Audit Event Type To String.
 * @param[in] t Input parameter.
 * @return Return value.
 */
std::string auditEventTypeToString(AuditEventType t);

// ============================================================================
// PHASE-3-ERROR-HANDLING: Rollback & Recovery Audit Trail
// ============================================================================

enum class RollbackReason {
    USER_REQUESTED,           ///< User explicitly requested rollback
    QUOTA_EXCEEDED,           ///< Resource quota limit exceeded
    SCHEMA_VALIDATION_FAILED, ///< Schema validation error prevented import
    CONNECTOR_UNAVAILABLE,    ///< Source connector connection failed
    QUALITY_GATE_FAILED,      ///< Quality check failed and bypass not enabled
    INTEGRITY_VIOLATION,      ///< Constraint violation during import
    TIMEOUT,                  ///< Import operation exceeded timeout
    UNKNOWN                   ///< Unspecified reason
};

/**
 * @brief Rollback Reason To String.
 * @param[in] reason Input parameter.
 * @return Return value.
 */
std::string rollbackReasonToString(RollbackReason reason);

struct RollbackAuditEvent {
    RollbackReason reason;

    uint64_t rows_attempted = {};

    uint64_t rows_committed = {};

    uint64_t rows_rolled_back;

    std::string failure_first_row_id;

    std::string recovery_suggestion;

    uint64_t rollback_timestamp_ns;

    json toJson() const {
        return json{
            {"reason", rollbackReasonToString(reason)},
            {"rows_attempted", rows_attempted},
            {"rows_committed", rows_committed},
            {"rows_rolled_back", rows_rolled_back},
            {"failure_first_row_id", failure_first_row_id},
            {"recovery_suggestion", recovery_suggestion},
            {"rollback_timestamp_ns", rollback_timestamp_ns}
        };
    }
};

class AuditedImporter {
public:
    // ------------------------------------------------------------------
    // Event types
    // ------------------------------------------------------------------
    enum class EventType {
        IMPORT_STARTED,
        SCHEMA_ANALYZED,
        RELATIONSHIP_MAPPED,
        DATA_VALIDATED,
        CONFLICT_RESOLVED,
        RECORD_IMPORTED,
        IMPORT_COMPLETED,
        ERROR_OCCURRED
    };

    /**
     * @brief Event Type To String.
     * @param[in] t Input parameter.
     * @return Return value.
     */
    static std::string eventTypeToString(EventType t);

    // ------------------------------------------------------------------
    // Audit event (Phase 2 T2.3.3 extended)
    // ------------------------------------------------------------------
    struct AuditEvent {
        // Legacy fields (backward compatible)
        EventType type;
        std::string timestamp;              ///< RFC 3339
        std::string user_principal;         ///< OIDC Subject
        std::string importer_instance_id;   ///< UUID
        json details;
        std::string correlation_id;         ///< For distributed log tracing

        // Phase 2 T2.3.3 fields (additive)
        AuditEventType event_type;          ///< Structured event type (Phase 2 extension)
        uint64_t event_timestamp_ns;        ///< Nanosecond precision timestamp
        std::string import_id;              ///< Unique import session ID
        std::string table_name;             ///< Target table name
        uint64_t sequence_number;           ///< Event order within import_id
    };

    // ------------------------------------------------------------------
    // Immutable audit log (Merkle-chained, Phase 2 T2.3.3 extended)
    // ------------------------------------------------------------------
    class ImmutableAuditLog {
    public:
        /**
         * @brief Record Event.
         * @param[in] event Input parameter.
         */
        void recordEvent(const AuditEvent& event);

        /**
         * @brief Verify Integrity.
         * @return True when the operation succeeds.
         */
        bool verifyIntegrity() const;

        json exportForSIEM(const std::string& format = "raw") const;

        /**
         * @brief Size.
         * @return Return value.
         */
        size_t size() const;

        /**
         * @brief Events.
         * @return Return value.
         */
        const std::vector<AuditEvent>& events() const;

        /**
         * @brief Emit Audit Event.
         * @param[in] event Input parameter.
         */
        void emitAuditEvent(const AuditEvent& event);

        /**
         * @brief Get Audit Trail For Import.
         * @param[in] import_id Identifier of the import.
         * @return Return value.
         */
        std::vector<AuditEvent> getAuditTrailForImport(const std::string& import_id) const;

        /**
         * @brief Emit Rollback Event.
         * @param[in] rollback_event Input parameter.
         * @param[in] import_id Identifier of the import.
         * @param[in] user_principal Input parameter.
         */
        void emitRollbackEvent(const RollbackAuditEvent& rollback_event,
                               const std::string& import_id,
                               const std::string& user_principal);

    private:
        std::vector<AuditEvent> events_;
        std::vector<std::string> chain_hashes_; ///< SHA-256 per event
        mutable std::string current_root_;      ///< Running root hash

        /**
         * @brief Compute Event Hash.
         * @param[in] event Input parameter.
         * @param[in] prev_hash Input parameter.
         * @return Return value.
         */
        std::string computeEventHash(const AuditEvent& event,
                                     const std::string& prev_hash) const;
    };
};

} // namespace importers
} // namespace themis
