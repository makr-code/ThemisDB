/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            audit_trail.h                                      ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-04-13 04:15:33                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     111                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 9efa3acd76  2026-03-11  feat(importers): add PostgreSQL Importer v2.1+ with 12 ne... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
