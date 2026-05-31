/*
 * ThemisDB | File: audit_trail.h | Version: 0.0.13 | Last Modified: 2026-05-20 17:13:04
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
    // Audit event
    // ------------------------------------------------------------------
    struct AuditEvent {
        EventType type;
        std::string timestamp;              ///< RFC 3339
        std::string user_principal;         ///< OIDC Subject
        std::string importer_instance_id;   ///< UUID
        json details;
        std::string correlation_id;         ///< For distributed log tracing
    };

    // ------------------------------------------------------------------
    // Immutable audit log (Merkle-chained)
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
