/**
 * @file model_governance.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/// Request passed to ModelGovernancePolicy::checkExportPermission().
/// Captures all information about a training-data export job that the
/// policy engine needs to make an allow/deny decision.
struct ModelTrainingExportRequest {
    std::string export_job_id;             ///< Unique identifier of the export job
    std::vector<std::string> collection_ids; ///< Collections to be exported
    std::vector<std::string> field_selectors; ///< Fields selected from the collections
    std::string requesting_user;           ///< User or service requesting the export
    std::string adapter_id;                ///< LoRA adapter / model that will be trained
    std::string classification;            ///< Data classification level of the dataset
    std::string purpose{"MODEL_TRAINING"}; ///< Export purpose (must be "MODEL_TRAINING")

    /// Serialize to JSON for audit trail
    nlohmann::json toJson() const;
};

/// Decision returned by ModelGovernancePolicy::checkExportPermission().
struct ModelGovernanceDecision {
    bool is_permitted = false;             ///< Whether the export is allowed
    std::string denial_reason;             ///< Populated when is_permitted == false
    std::string lineage_event_id;          ///< ID of the recorded lineage event (if permitted)

    /// Serialize to JSON
    nlohmann::json toJson() const;
};

/**
 * @brief Evaluates governance policies for AI/ML training data exports.
 *
 * ModelGovernancePolicy is the single authority for deciding whether a
 * training-data export may proceed.  It enforces:
 *
 *   1. Classification restrictions: data classified "geheim" or
 *      "streng-geheim" is never permitted for model training.
 *   2. Collection allow-list: operators may restrict specific collections
 *      from being used for model training via addRestrictedCollection().
 *   3. Lineage recording: every approved export is recorded in the
 *      DataLineageTracker as a MODEL_TRAINING event so the full training
 *      data provenance is auditable.
 *   4. Audit trail: every decision (allow or deny) is written to the
 *      configured AuditLogger.
 *
 * Design constraints (from FUTURE_ENHANCEMENTS.md)
 * -------------------------------------------------
 * - checkExportPermission() must be called before any training export
 *   begins; partial exports started without a prior approval must be
 *   rejected, not retroactively audited.
 * - The method adds ≤ 2 ms to export job startup.
 * - Thread-safe; all public methods may be called concurrently.
 */
class ModelGovernancePolicy {
public:
    ModelGovernancePolicy() = default;

    /// Attach an audit logger.  Every call to checkExportPermission() will
    /// write a decision entry regardless of the allow/deny outcome.
    void setAuditLogger(std::shared_ptr<themis::utils::AuditLogger> logger);

    /// Attach the lineage tracker used to record approved exports.
    void setLineageTracker(std::shared_ptr<DataLineageTracker> tracker);

    /// Restrict a collection from being used as model-training data.
    /// Thread-safe; may be called at any time.
    void addRestrictedCollection(const std::string& collection_id);

    /// Remove a collection from the restricted list.
    void removeRestrictedCollection(const std::string& collection_id);

    /// Return true if the given collection is restricted for model training.
    bool isCollectionRestricted(const std::string& collection_id) const;

    /**
     * @brief Evaluate whether the training-data export is permitted.
     *
     * Checks classification restrictions and the restricted-collection list,
     * then records a MODEL_TRAINING lineage event (on approval) and writes an
     * audit entry.
     *
     * @param request  Fully populated ModelTrainingExportRequest.
     * @return ModelGovernanceDecision with is_permitted and, on denial, a
     *         human-readable denial_reason; on approval, lineage_event_id is
     *         set to the ID of the recorded lineage entry.
     */
    ModelGovernanceDecision checkExportPermission(
        const ModelTrainingExportRequest& request);

private:
    mutable std::mutex mutex_;

    std::shared_ptr<themis::utils::AuditLogger> audit_logger_;
    std::shared_ptr<DataLineageTracker> lineage_tracker_;
    std::unordered_set<std::string> restricted_collections_;

    /// Write the decision to the audit trail (does not throw)
    void writeAuditEntry(
        const ModelTrainingExportRequest& request,
        const ModelGovernanceDecision& decision,
        const std::shared_ptr<themis::utils::AuditLogger>& audit_log) const;
};

} // namespace governance
} // namespace themis
