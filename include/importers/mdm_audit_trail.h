/**
 * @file mdm_audit_trail.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <optional>
#include <mutex>
#include <nlohmann/json.hpp>

namespace themis {
namespace importers {

using json = nlohmann::json;

/**
 * @brief Immutable audit trail for all MDM operations.
 *
 * Records are appended in chronological order.  Each event carries a
 * SHA-256 chain hash over the preceding event so that the integrity of
 * the trail can be verified independently.
 *
 * Thread-safety: recordEvent() / getAuditFor() / verifyAuditChain() are
 * protected by an internal mutex and safe to call from multiple threads.
 */
class MDMAuditTrail {
public:
    /**
     * @brief Type of MDM operation captured in an audit event.
     */
    enum class Operation {
        MATCH_FOUND,
        LINK_CREATED,
        CONFLICT_DETECTED,
        CONFLICT_RESOLVED,
        GOLDEN_RECORD_CREATED,
        ENTITY_MERGED,
        REVIEW_REQUESTED,
        REVIEW_COMPLETED
    };

    /**
     * @brief A single immutable audit event.
     */
    struct AuditEvent {
        std::string  event_id;             ///< UUID
        Operation    operation;
        std::string  collection_name;
        std::string  source_entity_id;
        std::string  target_entity_id;
        double       confidence_score = 0.0;
        json         event_details;
        std::string  timestamp;            ///< RFC 3339
        std::string  initiated_by;         ///< "importer_v2.2" or user identifier
        std::string  status;               ///< "pending" | "completed" | "failed"

        /// SHA-256 hex digest over: previous_hash + event_id + timestamp + source_id + target_id
        std::string  chain_hash;

        json toJson() const;
    };

    MDMAuditTrail() = default;

    /**
     * @brief Append a new audit event to the trail.
     *
     * Automatically computes and attaches the chain hash before insertion.
     *
     * @param event  Event to record.  The @c chain_hash field is filled in
     *               by this method; any caller-supplied value is overwritten.
     */
    void recordEvent(AuditEvent event);

    /**
     * @brief Retrieve all audit events for a specific entity.
     *
     * @param entity_id         Entity whose events to return.
     * @param collection_name   Collection scope (empty = all collections).
     * @param operation_filter  Optional filter; only events with this operation
     *                          are included.
     * @return                  Events sorted by insertion order (oldest first).
     */
    std::vector<AuditEvent> getAuditFor(
        const std::string&              entity_id,
        const std::string&              collection_name,
        const std::optional<Operation>& operation_filter = std::nullopt
    ) const;

    /**
     * @brief Verify the integrity of the entire audit chain.
     *
     * Recomputes each chain hash in sequence and compares it against the
     * stored value.
     *
     * @return true if all chain hashes are correct, false if any has been
     *         tampered with or is missing.
     */
    bool verifyAuditChain() const;

    /**
     * @brief Export a structured audit report for compliance purposes.
     *
     * @param collection_name  Collection to report on (empty = all).
     * @param start_date       ISO 8601 / RFC 3339 lower bound (inclusive).
     * @param end_date         ISO 8601 / RFC 3339 upper bound (inclusive).
     * @return                 JSON object with summary statistics and event list.
     */
    json exportAuditReport(
        const std::string& collection_name,
        const std::string& start_date,
        const std::string& end_date
    ) const;

    /**
     * @brief Return the total number of events recorded.
     */
    size_t eventCount() const;

    /**
     * @brief Clear all stored events (used in testing).
     */
    void clear();

    /**
     * @brief Convert an Operation enum value to its string name.
     */
    static std::string operationName(Operation op);

private:
    mutable std::mutex   mutex_;
    std::vector<AuditEvent> events_;

    static std::string computeChainHash(
        const std::string& previous_hash,
        const AuditEvent&  event
    );
    static std::string generateUUID();
    static std::string nowRfc3339();
};

} // namespace importers
} // namespace themis
