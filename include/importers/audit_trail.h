/**
 * @file audit_trail.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: audit_trail.h | Version: 0.0.13 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 98
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace themis {
namespace importers {

using json = nlohmann::json;

/**
 * @brief Audit event type classification (Phase 2 T2.3.3).
 *
 * PHASE-2-HARDENING: Unified Audit Event Schema & Correlation
 * Determinism: yes (enum-based, no randomness)
 * Audit: all events tracked with structured type
 * Bounded: buffer limited to 100,000 events
 */
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

/**
 * @brief Audit event constants (Phase 2 T2.3.3).
 *
 * PHASE-2-HARDENING: Audit Buffer Management
 * Bounded: circular buffer limited to kMaxAuditBufferSize events
 * Audit: events preserved in chronological order
 */
constexpr size_t kMaxAuditBufferSize = 100000;  ///< Maximum events per process

/**
 * @brief Convert AuditEventType to string.
 *
 * @param t Event type enum value
 * @return String representation suitable for logging/audit trail
 */
std::string auditEventTypeToString(AuditEventType t);

/**
 * @brief SOX 404 / HIPAA compliant immutable audit trail for import operations.
 *
 * Events are chained via a SHA-256 Merkle hash to detect tampering.
 *
 * Standards:
 *   - NIST RBAC Model
 *   - OASIS ABAC Standard
 *   - SOX 404 Compliance Audit Trail
 */
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
        /** @brief Append an event to the chain. Thread-safe. */
        void recordEvent(const AuditEvent& event);

        /**
         * @brief Verify that no event in the chain has been tampered with.
         * @return true if all hashes are consistent.
         */
        bool verifyIntegrity() const;

        /**
         * @brief Export all events as a SIEM-compatible JSON array.
         * @param format  "splunk" | "elk" | "raw" (default).
         */
        json exportForSIEM(const std::string& format = "raw") const;

        /** @brief Number of events recorded so far. */
        size_t size() const;

        /** @brief Return all events (read-only). */
        const std::vector<AuditEvent>& events() const;

        /**
         * @brief Emit a structured audit event (Phase 2 T2.3.3).
         *
         * PHASE-2-HARDENING: Unified Audit Event Schema & Correlation
         * Determinism: yes (all fields deterministic)
         * Audit: centralized emission with timestamp and sequence
         * Bounded: buffer limited to 100,000 events; drops oldest when full
         *
         * Centralized entry point for recording audit events with:
         * - High-precision timestamp (nanosecond)
         * - Correlation ID propagation
         * - Chronological sequencing per import_id
         * - Automatic buffer overflow handling
         *
         * @param event  Audit event to emit (event_type, import_id, correlation_id required)
         * @throws std::exception if buffer is full and cannot drop oldest (rare)
         */
        void emitAuditEvent(const AuditEvent& event);

        /**
         * @brief Retrieve audit trail for a specific import session (Phase 2 T2.3.3).
         *
         * PHASE-2-HARDENING: Audit Trail Replay
         * Determinism: yes (ordered by sequence_number)
         * Audit: enables audit trail replay for debugging
         * Bounded: returns all events in chronological order
         *
         * Returns all events for a given import_id in chronological order
         * (ordered by sequence_number). Enables debugging and compliance
         * audit trail replay.
         *
         * @param import_id  Import session ID to retrieve events for
         * @return           Events ordered by sequence_number, or empty vector if not found
         */
        std::vector<AuditEvent> getAuditTrailForImport(const std::string& import_id) const;

    private:
        std::vector<AuditEvent> events_;
        std::vector<std::string> chain_hashes_; ///< SHA-256 per event
        mutable std::string current_root_;      ///< Running root hash

        std::string computeEventHash(const AuditEvent& event,
                                     const std::string& prev_hash) const;
    };
};

} // namespace importers
} // namespace themis
