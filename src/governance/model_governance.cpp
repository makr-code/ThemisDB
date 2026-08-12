/**
 * @file model_governance.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "governance/model_governance.h"

#include <algorithm>
#include <chrono>
#include <sstream>

#include "observability/metrics_collector.h"
#include "utils/audit_logger.h"
#include "utils/logger.h"

namespace themis {
namespace governance {

// ─── ModelTrainingExportRequest ──────────────────────────────────────────────

nlohmann::json ModelTrainingExportRequest::toJson() const {
    nlohmann::json j;
    j["export_job_id"]   = export_job_id;
    j["collection_ids"]  = collection_ids;
    j["field_selectors"] = field_selectors;
    j["requesting_user"] = requesting_user;
    j["adapter_id"]      = adapter_id;
    j["classification"]  = classification;
    j["purpose"]         = purpose;
    return j;
}

// ─── ModelGovernanceDecision ─────────────────────────────────────────────────

nlohmann::json ModelGovernanceDecision::toJson() const {
    nlohmann::json j;
    j["is_permitted"]     = is_permitted;
    j["denial_reason"]    = denial_reason;
    j["lineage_event_id"] = lineage_event_id;
    return j;
}

// ─── ModelGovernancePolicy ───────────────────────────────────────────────────

void ModelGovernancePolicy::setAuditLogger(std::shared_ptr<themis::utils::AuditLogger> logger) {
    std::lock_guard<std::mutex> lock(mutex_);
    audit_logger_ = std::move(logger);
}

void ModelGovernancePolicy::setLineageTracker(std::shared_ptr<DataLineageTracker> tracker) {
    std::lock_guard<std::mutex> lock(mutex_);
    lineage_tracker_ = std::move(tracker);
}

void ModelGovernancePolicy::addRestrictedCollection(const std::string &collection_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    restricted_collections_.insert(collection_id);
    THEMIS_INFO("ModelGovernancePolicy: collection '{}' restricted for model training", collection_id);
}

void ModelGovernancePolicy::removeRestrictedCollection(const std::string &collection_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    restricted_collections_.erase(collection_id);
}

bool ModelGovernancePolicy::isCollectionRestricted(const std::string &collection_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return restricted_collections_.count(collection_id) > 0;
}

ModelGovernanceDecision ModelGovernancePolicy::checkExportPermission(const ModelTrainingExportRequest &request) {
    // Snapshot mutable state under the lock so the rest of the method is
    // lock-free and the lock hold time is minimal (satisfies ≤ 2 ms target).
    std::shared_ptr<themis::utils::AuditLogger> audit_log;
    std::shared_ptr<DataLineageTracker> lineage;
    std::unordered_set<std::string> restricted;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        audit_log  = audit_logger_;
        lineage    = lineage_tracker_;
        restricted = restricted_collections_;
    }

    ModelGovernanceDecision decision;

    // ── 1. Classification check ──────────────────────────────────────────────
    // Data classified "geheim" or "streng-geheim" must never be used for model
    // training (per FUTURE_ENHANCEMENTS.md security constraint).
    const std::string cls_lower = [&] {
        std::string s = request.classification;
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(::tolower(c)); });
        return s;
    }();

    if (cls_lower == "geheim" || cls_lower == "streng-geheim") {
        decision.is_permitted = false;
        decision.denial_reason
            = "Data classification '" + request.classification + "' is not permitted for model training";
        THEMIS_WARN("ModelGovernancePolicy: export denied for job '{}': {}", request.export_job_id,
                    decision.denial_reason);
        observability::MetricsCollector::getInstance().addCounter("governance_model_export_total", 1,
                                                                  {{"result", "denied_classification"}});
        writeAuditEntry(request, decision, audit_log);
        return decision;
    }

    // ── 2. Restricted-collection check ───────────────────────────────────────
    for (const auto &cid : request.collection_ids) {
        if (restricted.count(cid) > 0) {
            decision.is_permitted  = false;
            decision.denial_reason = "Collection '" + cid + "' is restricted for model training";
            THEMIS_WARN("ModelGovernancePolicy: export denied for job '{}': {}", request.export_job_id,
                        decision.denial_reason);
            observability::MetricsCollector::getInstance().addCounter("governance_model_export_total", 1,
                                                                      {{"result", "denied_restricted_collection"}});
            writeAuditEntry(request, decision, audit_log);
            return decision;
        }
    }

    // ── 3. Export is permitted — record training data lineage ─────────────────
    decision.is_permitted = true;

    if (lineage) {
        LineageEvent ev;
        ev.dataset_id   = request.export_job_id;
        ev.event_type   = LineageEventType::MODEL_TRAINING;
        ev.performed_by = request.requesting_user;
        ev.operation    = "model_training_export";
        ev.metadata     = {{"adapter_id", request.adapter_id},
                           {"collection_ids", request.collection_ids},
                           {"field_selectors", request.field_selectors},
                           {"classification", request.classification},
                           {"purpose", request.purpose}};
        // event_id and timestamp are auto-assigned by DataLineageTracker
        lineage->recordEvent(ev);

        // Capture the assigned event_id from the recorded event
        auto record = lineage->getLineage(request.export_job_id);
        if (!record.events.empty()) {
            decision.lineage_event_id = record.events.back().event_id;
        }
    }

    observability::MetricsCollector::getInstance().addCounter("governance_model_export_total", 1,
                                                              {{"result", "permitted"}});

    THEMIS_INFO("ModelGovernancePolicy: export permitted for job '{}' (adapter='{}')", request.export_job_id,
                request.adapter_id);

    writeAuditEntry(request, decision, audit_log);
    return decision;
}

void ModelGovernancePolicy::writeAuditEntry(const ModelTrainingExportRequest &request,
                                            const ModelGovernanceDecision &decision,
                                            const std::shared_ptr<themis::utils::AuditLogger> &audit_log) const {
    if (!audit_log) {
        return;
    }

    nlohmann::json entry
        = {{"event_type", "model_governance_decision"},
           {"export_job_id", request.export_job_id},
           {"adapter_id", request.adapter_id},
           {"requesting_user", request.requesting_user},
           {"collection_ids", request.collection_ids},
           {"field_selectors", request.field_selectors},
           {"classification", request.classification},
           {"is_permitted", decision.is_permitted},
           {"denial_reason", decision.denial_reason},
           {"lineage_event_id", decision.lineage_event_id},
           {"timestamp",
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
                .count()}};
    audit_log->logEvent(entry);
}

} // namespace governance
} // namespace themis
