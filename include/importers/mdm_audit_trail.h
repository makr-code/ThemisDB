/**
 * @file mdm_audit_trail.h
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
#include <optional>
#include <mutex>
#include <nlohmann/json.hpp>

namespace themis {
namespace importers {

using json = nlohmann::json;

class MDMAuditTrail {
public:
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

        std::string  chain_hash;

        /**
         * @brief To Json.
         * @return Return value.
         */
        json toJson() const;
    };

    MDMAuditTrail() = default;

    /**
     * @brief Record Event.
     * @param[in] event Input parameter.
     */
    void recordEvent(AuditEvent event);

    std::vector<AuditEvent> getAuditFor(
        const std::string&              entity_id,
        const std::string&              collection_name,
        const std::optional<Operation>& operation_filter = std::nullopt
    ) const;

    /**
     * @brief Verify Audit Chain.
     * @return True when the operation succeeds.
     */
    bool verifyAuditChain() const;

    /**
     * @brief Export Audit Report.
     * @param[in] collection_name Name of the collection.
     * @param[in] start_date Input parameter.
     * @param[in] end_date Input parameter.
     * @return Return value.
     */
    json exportAuditReport(
        const std::string& collection_name,
        const std::string& start_date,
        const std::string& end_date
    ) const;

    /**
     * @brief Event Count.
     * @return Return value.
     */
    size_t eventCount() const;

    /**
     * @brief Clear.
     */
    void clear();

    /**
     * @brief Operation Name.
     * @param[in] op Input parameter.
     * @return Return value.
     */
    static std::string operationName(Operation op);

private:
    mutable std::mutex   mutex_;
    std::vector<AuditEvent> events_;

    /**
     * @brief Compute Chain Hash.
     * @param[in] previous_hash Input parameter.
     * @param[in] event Input parameter.
     * @return Return value.
     */
    static std::string computeChainHash(
        const std::string& previous_hash,
        const AuditEvent&  event
    );
    /**
     * @brief Generate UUID.
     * @return Return value.
     */
    static std::string generateUUID();
    /**
     * @brief Now Rfc3339.
     * @return Return value.
     */
    static std::string nowRfc3339();
};

} // namespace importers
} // namespace themis
