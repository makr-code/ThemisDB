/**
 * @file model_governance.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "governance/data_lineage.h"
#include <string>
#include <vector>
#include <unordered_set>
#include <memory>
#include <mutex>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace themis {
namespace utils {
    class AuditLogger;
}

namespace governance {

struct ModelTrainingExportRequest {
    std::string export_job_id;             ///< Unique identifier of the export job
    std::vector<std::string> collection_ids; ///< Collections to be exported
    std::vector<std::string> field_selectors; ///< Fields selected from the collections
    std::string requesting_user;           ///< User or service requesting the export
    std::string adapter_id;                ///< LoRA adapter / model that will be trained
    std::string classification;            ///< Data classification level of the dataset
    std::string purpose{"MODEL_TRAINING"}; ///< Export purpose (must be "MODEL_TRAINING")

    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
};

struct ModelGovernanceDecision {
    bool is_permitted = false;             ///< Whether the export is allowed
    std::string denial_reason;             ///< Populated when is_permitted == false
    std::string lineage_event_id;          ///< ID of the recorded lineage event (if permitted)

    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
};

class ModelGovernancePolicy {
public:
    ModelGovernancePolicy() = default;

    /**
     * @brief Set Audit Logger.
     * @param[in] logger Input parameter.
     */
    void setAuditLogger(std::shared_ptr<themis::utils::AuditLogger> logger);

    /**
     * @brief Set Lineage Tracker.
     * @param[in] tracker Input parameter.
     */
    void setLineageTracker(std::shared_ptr<DataLineageTracker> tracker);

    /**
     * @brief Add Restricted Collection.
     * @param[in] collection_id Identifier of the collection.
     */
    void addRestrictedCollection(const std::string& collection_id);

    /**
     * @brief Remove Restricted Collection.
     * @param[in] collection_id Identifier of the collection.
     */
    void removeRestrictedCollection(const std::string& collection_id);

    /**
     * @brief Is Collection Restricted.
     * @param[in] collection_id Identifier of the collection.
     * @return True when the operation succeeds.
     */
    bool isCollectionRestricted(const std::string& collection_id) const;

    /**
     * @brief Check Export Permission.
     * @param[in] request Input parameter.
     * @return Return value.
     */
    ModelGovernanceDecision checkExportPermission(
        const ModelTrainingExportRequest& request);

private:
    mutable std::mutex mutex_;

    std::shared_ptr<themis::utils::AuditLogger> audit_logger_;
    std::shared_ptr<DataLineageTracker> lineage_tracker_;
    std::unordered_set<std::string> restricted_collections_;

    /**
     * @brief Write Audit Entry.
     * @param[in] request Input parameter.
     * @param[in] decision Input parameter.
     * @param[in] audit_log Input parameter.
     */
    void writeAuditEntry(
        const ModelTrainingExportRequest& request,
        const ModelGovernanceDecision& decision,
        const std::shared_ptr<themis::utils::AuditLogger>& audit_log) const;
};

} // namespace governance
} // namespace themis
