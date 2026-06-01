/*
 * ThemisDB | File: model_governance.cpp | Version: 0.0.15 | Last Modified: 2026-05-21 16:50:40
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 184
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=0
 * PR History (last 5): #2874 feat(governance): AI/ML mod... (2026-03-12)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "governance/model_governance.h"

#include <algorithm>
#include <chrono>
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>

#include "observability/metrics_collector.h"
#include "utils/audit_logger.h"
#include "utils/logger.h"

namespace themis {
namespace governance {

namespace {

std::string sha256Hex(const std::string& input) {
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.data()), input.size(), digest);
    std::ostringstream ss;
    ss << std::hex << std::setfill('0');
    for (unsigned char b : digest) {
        ss << std::setw(2) << static_cast<int>(b);
    }
    return ss.str();
}

std::string joinStrings(const std::vector<std::string>& values) {
    std::ostringstream ss;
    bool first = true;
    for (const auto& v : values) {
        if (!first) {
            ss << '\n';
        }
        first = false;
        ss << v;
    }
    return ss.str();
}

} // namespace

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
    j["dataset_query"]   = dataset_query;
    j["schema_hash"]     = schema_hash;
    j["content_hash"]    = content_hash;
    j["random_seed"]     = random_seed;
    j["split"]           = split;
    j["redaction_policy"] = redaction_policy;
    j["exporter_version"] = exporter_version;
    return j;
}

// ─── DatasetSnapshot ──────────────────────────────────────────────────────────

nlohmann::json DatasetSnapshot::toJson() const {
    nlohmann::json j;
    j["snapshot_id"] = snapshot_id;
    j["export_job_id"] = export_job_id;
    j["adapter_id"] = adapter_id;
    j["collection_ids"] = collection_ids;
    j["field_selectors"] = field_selectors;
    j["query_hash"] = query_hash;
    j["schema_hash"] = schema_hash;
    j["content_hash"] = content_hash;
    j["random_seed"] = random_seed;
    j["split"] = split;
    j["redaction_policy"] = redaction_policy;
    j["exporter_version"] = exporter_version;
    j["governance_decision_id"] = governance_decision_id;
    j["created_at_ms"] = created_at_ms;
    return j;
}

DatasetSnapshot DatasetSnapshot::fromJson(const nlohmann::json& j) {
    DatasetSnapshot s;
    s.snapshot_id = j.value("snapshot_id", "");
    s.export_job_id = j.value("export_job_id", "");
    s.adapter_id = j.value("adapter_id", "");
    if (j.contains("collection_ids")) {
        s.collection_ids = j["collection_ids"].get<std::vector<std::string>>();
    }
    if (j.contains("field_selectors")) {
        s.field_selectors = j["field_selectors"].get<std::vector<std::string>>();
    }
    s.query_hash = j.value("query_hash", "");
    s.schema_hash = j.value("schema_hash", "");
    s.content_hash = j.value("content_hash", "");
    s.random_seed = j.value("random_seed", static_cast<uint64_t>(42));
    s.split = j.value("split", "train");
    s.redaction_policy = j.value("redaction_policy", "default");
    s.exporter_version = j.value("exporter_version", "");
    s.governance_decision_id = j.value("governance_decision_id", "");
    s.created_at_ms = j.value("created_at_ms", static_cast<int64_t>(0));
    return s;
}

// ─── ModelGovernanceDecision ─────────────────────────────────────────────────

nlohmann::json ModelGovernanceDecision::toJson() const {
    nlohmann::json j;
    j["is_permitted"]     = is_permitted;
    j["denial_reason"]    = denial_reason;
    j["lineage_event_id"] = lineage_event_id;
    j["dataset_snapshot_id"] = dataset_snapshot_id;
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
                           {"purpose", request.purpose},
                           {"dataset_query", request.dataset_query},
                           {"random_seed", request.random_seed},
                           {"split", request.split},
                           {"redaction_policy", request.redaction_policy},
                           {"exporter_version", request.exporter_version}};
        // event_id and timestamp are auto-assigned by DataLineageTracker
        lineage->recordEvent(ev);

        // Capture the assigned event_id from the recorded event
        auto record = lineage->getLineage(request.export_job_id);
        if (!record.events.empty()) {
            decision.lineage_event_id = record.events.back().event_id;
        }
    }

    // Build deterministic snapshot metadata and persist in-policy state.
    DatasetSnapshot snapshot;
    snapshot.export_job_id = request.export_job_id;
    snapshot.adapter_id = request.adapter_id;
    snapshot.collection_ids = request.collection_ids;
    snapshot.field_selectors = request.field_selectors;
    snapshot.random_seed = request.random_seed;
    snapshot.split = request.split.empty() ? "train" : request.split;
    snapshot.redaction_policy = request.redaction_policy.empty() ? "default" : request.redaction_policy;
    snapshot.exporter_version = request.exporter_version;
    snapshot.governance_decision_id = decision.lineage_event_id;
    snapshot.created_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    const std::string query_basis =
        request.dataset_query.empty()
            ? (joinStrings(request.collection_ids) + "\n" + joinStrings(request.field_selectors))
            : request.dataset_query;
    snapshot.query_hash = sha256Hex(query_basis);
    snapshot.schema_hash = request.schema_hash.empty()
                               ? sha256Hex(joinStrings(request.field_selectors))
                               : request.schema_hash;
    snapshot.content_hash = request.content_hash.empty()
                                ? sha256Hex(query_basis + "\n" + std::to_string(request.random_seed) + "\n" +
                                            snapshot.split + "\n" + snapshot.redaction_policy)
                                : request.content_hash;
    snapshot.snapshot_id = "dsnap_" + snapshot.content_hash.substr(0, 16) + "_" +
                           snapshot.query_hash.substr(0, 8);
    decision.dataset_snapshot_id = snapshot.snapshot_id;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        dataset_snapshots_[snapshot.snapshot_id] = snapshot;
        dataset_snapshots_by_export_job_[snapshot.export_job_id].push_back(snapshot.snapshot_id);
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

std::optional<DatasetSnapshot> ModelGovernancePolicy::getDatasetSnapshot(
    const std::string& snapshot_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = dataset_snapshots_.find(snapshot_id);
    if (it == dataset_snapshots_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::vector<DatasetSnapshot> ModelGovernancePolicy::listDatasetSnapshots(
    const std::string& export_job_id) const {
    std::vector<DatasetSnapshot> result;
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = dataset_snapshots_by_export_job_.find(export_job_id);
    if (it == dataset_snapshots_by_export_job_.end()) {
        return result;
    }
    result.reserve(it->second.size());
    for (const auto& snapshot_id : it->second) {
        auto s_it = dataset_snapshots_.find(snapshot_id);
        if (s_it != dataset_snapshots_.end()) {
            result.push_back(s_it->second);
        }
    }
    return result;
}

} // namespace governance
} // namespace themis
