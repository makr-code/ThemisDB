/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ccpa_rules.cpp                                     ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-02-25                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "governance/ccpa_rules.h"
#include "utils/logger.h"

#include <chrono>

namespace themis {
namespace governance {

// ========== CcpaSubjectRecord ==========

nlohmann::json CcpaSubjectRecord::toJson() const {
    nlohmann::json j;
    j["subject_id"]               = subject_id;
    j["data_categories"]          = data_categories;
    j["opt_out_of_sale"]          = opt_out_of_sale;
    j["opt_out_of_sharing"]       = opt_out_of_sharing;
    j["deletion_requested"]       = deletion_requested;
    j["right_to_know_requested"]  = right_to_know_requested;
    j["portability_requested"]    = portability_requested;
    j["third_party_disclosures"]  = third_party_disclosures;
    j["opt_out_timestamp"]        = opt_out_timestamp;
    j["last_updated"]             = last_updated;
    return j;
}

CcpaSubjectRecord CcpaSubjectRecord::fromJson(const nlohmann::json& j) {
    CcpaSubjectRecord rec;
    if (j.contains("subject_id"))              rec.subject_id              = j["subject_id"].get<std::string>();
    if (j.contains("data_categories"))         rec.data_categories         = j["data_categories"].get<std::vector<std::string>>();
    if (j.contains("opt_out_of_sale"))         rec.opt_out_of_sale         = j["opt_out_of_sale"].get<bool>();
    if (j.contains("opt_out_of_sharing"))      rec.opt_out_of_sharing      = j["opt_out_of_sharing"].get<bool>();
    if (j.contains("deletion_requested"))      rec.deletion_requested      = j["deletion_requested"].get<bool>();
    if (j.contains("right_to_know_requested")) rec.right_to_know_requested = j["right_to_know_requested"].get<bool>();
    if (j.contains("portability_requested"))   rec.portability_requested   = j["portability_requested"].get<bool>();
    if (j.contains("third_party_disclosures")) rec.third_party_disclosures = j["third_party_disclosures"].get<std::vector<std::string>>();
    if (j.contains("opt_out_timestamp"))       rec.opt_out_timestamp       = j["opt_out_timestamp"].get<int64_t>();
    if (j.contains("last_updated"))            rec.last_updated            = j["last_updated"].get<int64_t>();
    return rec;
}

// ========== CcpaReport ==========

nlohmann::json CcpaReport::toJson() const {
    nlohmann::json j;
    j["report_id"]                = report_id;
    j["generated_at"]             = generated_at;
    j["window_start"]             = window_start;
    j["window_end"]               = window_end;
    j["total_subjects"]           = total_subjects;
    j["opted_out_of_sale"]        = opted_out_of_sale;
    j["opted_out_of_sharing"]     = opted_out_of_sharing;
    j["deletion_requests"]        = deletion_requests;
    j["right_to_know_requests"]   = right_to_know_requests;
    j["portability_requests"]     = portability_requests;
    j["subjects_by_category"]     = subjects_by_category;
    j["disclosures_by_third_party"] = disclosures_by_third_party;
    return j;
}

// ========== CcpaRuleSet ==========

void CcpaRuleSet::registerSubject(const CcpaSubjectRecord& record) {
    std::lock_guard<std::mutex> lk(mutex_);
    subjects_[record.subject_id] = record;
}

bool CcpaRuleSet::removeSubject(const std::string& subject_id) {
    std::lock_guard<std::mutex> lk(mutex_);
    return subjects_.erase(subject_id) > 0;
}

std::optional<CcpaSubjectRecord> CcpaRuleSet::getSubject(const std::string& subject_id) const {
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = subjects_.find(subject_id);
    if (it == subjects_.end()) return std::nullopt;
    return it->second;
}

int CcpaRuleSet::subjectCount() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return static_cast<int>(subjects_.size());
}

// ---- evaluateOptOutOfSale -----------------------------------------------

CcpaEvaluationResult CcpaRuleSet::evaluateOptOutOfSale(
    const CcpaEvaluationContext& ctx
) const {
    CcpaEvaluationResult result;
    result.rule_id = RULE_OPT_OUT_OF_SALE;
    result.audit_required = true;

    // Rule is only relevant for sell/share actions
    const bool is_sale_or_share = (ctx.action == "sell" || ctx.action == "share");
    if (!is_sale_or_share) {
        result.allowed = true;
        result.reason  = "Action is not a sale or sharing; opt-out rule does not apply";
        return result;
    }

    // Service providers are exempt from opt-out restrictions (§ 1798.140(ag))
    if (ctx.is_service_provider) {
        result.allowed = true;
        result.reason  = "Requestor is a service provider; exempt from opt-out-of-sale rule";
        return result;
    }

    std::lock_guard<std::mutex> lk(mutex_);
    auto it = subjects_.find(ctx.subject_id);
    if (it == subjects_.end()) {
        // No record: allow but flag for awareness
        result.allowed = true;
        result.reason  = "No CCPA record found for subject; allowing with no opt-out flag on file";
        return result;
    }

    const auto& rec = it->second;

    if (ctx.action == "sell" && rec.opt_out_of_sale) {
        result.allowed = false;
        result.reason  = "Subject has exercised right to opt out of sale (Cal. Civ. Code § 1798.120)";
        result.required_actions = {"honor_opt_out", "notify_requestor"};
        return result;
    }

    if (ctx.action == "share" && rec.opt_out_of_sharing) {
        result.allowed = false;
        result.reason  = "Subject has opted out of cross-context behavioral advertising sharing (CPRA § 1798.135)";
        result.required_actions = {"honor_opt_out", "notify_requestor"};
        return result;
    }

    result.allowed = true;
    result.reason  = "Subject has not opted out; sale or sharing is permitted";
    return result;
}

// ---- evaluateRightToDelete ----------------------------------------------

CcpaEvaluationResult CcpaRuleSet::evaluateRightToDelete(
    const CcpaEvaluationContext& ctx
) const {
    CcpaEvaluationResult result;
    result.rule_id = RULE_RIGHT_TO_DELETE;
    result.audit_required = true;

    // Block read and share on data with a pending deletion request
    const bool is_access = (ctx.action == "read" || ctx.action == "share");
    if (!is_access) {
        result.allowed = true;
        result.reason  = "Action is not a read or share; right-to-delete rule does not apply";
        return result;
    }

    std::lock_guard<std::mutex> lk(mutex_);
    auto it = subjects_.find(ctx.subject_id);
    if (it == subjects_.end()) {
        result.allowed = true;
        result.reason  = "No CCPA record found for subject; no pending deletion request";
        return result;
    }

    const auto& rec = it->second;

    if (rec.deletion_requested) {
        result.allowed = false;
        result.reason  = "Subject has a pending deletion request (Cal. Civ. Code § 1798.105); "
                         "data must not be accessed until deletion is fulfilled";
        result.required_actions = {"fulfill_deletion_request", "notify_subject"};
        return result;
    }

    result.allowed = true;
    result.reason  = "No pending deletion request for subject";
    return result;
}

// ---- evaluateRightToKnow ------------------------------------------------

CcpaEvaluationResult CcpaRuleSet::evaluateRightToKnow(
    const CcpaEvaluationContext& ctx
) const {
    CcpaEvaluationResult result;
    result.rule_id = RULE_RIGHT_TO_KNOW;
    result.audit_required = true;

    // The right-to-know governs disclosure actions only
    if (ctx.action != "disclose_categories" && ctx.action != "disclose_specific_pieces") {
        result.allowed = true;
        result.reason  = "Action is not a right-to-know disclosure; rule does not apply";
        return result;
    }

    std::lock_guard<std::mutex> lk(mutex_);
    auto it = subjects_.find(ctx.subject_id);
    if (it == subjects_.end()) {
        // No record on file: operator must still respond within 45 days
        result.allowed = true;
        result.reason  = "No personal information on file for subject; respond with empty disclosure";
        result.required_actions = {"send_disclosure_response"};
        return result;
    }

    const auto& rec = it->second;

    if (rec.right_to_know_requested) {
        result.allowed = true;
        result.reason  = "Right-to-know request on record; disclosure is permitted and required "
                         "(Cal. Civ. Code § 1798.110 / § 1798.115)";
        result.required_actions = {"send_disclosure_response", "record_fulfillment"};
        return result;
    }

    // No pending request; block unsolicited disclosure of the full record
    result.allowed = false;
    result.reason  = "No right-to-know request on record; disclosure not triggered";
    return result;
}

// ---- evaluateDataPortability --------------------------------------------

CcpaEvaluationResult CcpaRuleSet::evaluateDataPortability(
    const CcpaEvaluationContext& ctx
) const {
    CcpaEvaluationResult result;
    result.rule_id = RULE_DATA_PORTABILITY;
    result.audit_required = true;

    if (ctx.action != "export") {
        result.allowed = true;
        result.reason  = "Action is not an export; data portability rule does not apply";
        return result;
    }

    std::lock_guard<std::mutex> lk(mutex_);
    auto it = subjects_.find(ctx.subject_id);
    if (it == subjects_.end()) {
        result.allowed = false;
        result.reason  = "No personal information on file for subject; nothing to export";
        return result;
    }

    const auto& rec = it->second;

    if (rec.portability_requested) {
        result.allowed = true;
        result.reason  = "Data portability request on record; machine-readable export is permitted "
                         "(Cal. Civ. Code § 1798.100)";
        result.required_actions = {"produce_portable_export", "record_fulfillment"};
        return result;
    }

    result.allowed = false;
    result.reason  = "No portability request on record; export not authorized under CCPA";
    return result;
}

// ---- evaluateAll --------------------------------------------------------

CcpaEvaluationResult CcpaRuleSet::evaluateAll(
    const CcpaEvaluationContext& ctx,
    std::vector<CcpaEvaluationResult>* individual_results
) const {
    const std::vector<CcpaEvaluationResult> results = {
        evaluateOptOutOfSale(ctx),
        evaluateRightToDelete(ctx),
        evaluateRightToKnow(ctx),
        evaluateDataPortability(ctx),
    };

    if (individual_results) {
        *individual_results = results;
    }

    // Most restrictive wins: any denial overrides allows
    for (const auto& r : results) {
        if (!r.allowed) {
            return r;
        }
    }

    // All rules allowed – return the first result as representative
    return results[0];
}

// ---- generateReport -----------------------------------------------------

CcpaReport CcpaRuleSet::generateReport(
    int64_t window_start_ms,
    int64_t window_end_ms
) const {
    CcpaReport report;
    report.generated_at = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    report.window_start = window_start_ms;
    report.window_end   = window_end_ms;

    // Build a stable report ID from the generation timestamp
    report.report_id = "ccpa_" + std::to_string(report.generated_at);

    std::lock_guard<std::mutex> lk(mutex_);

    for (const auto& [id, rec] : subjects_) {
        // Filter by time window using last_updated timestamp
        if (rec.last_updated > 0) {
            if (rec.last_updated < window_start_ms || rec.last_updated >= window_end_ms) {
                continue;
            }
        }

        ++report.total_subjects;

        if (rec.opt_out_of_sale)         ++report.opted_out_of_sale;
        if (rec.opt_out_of_sharing)      ++report.opted_out_of_sharing;
        if (rec.deletion_requested)      ++report.deletion_requests;
        if (rec.right_to_know_requested) ++report.right_to_know_requests;
        if (rec.portability_requested)   ++report.portability_requests;

        for (const auto& cat : rec.data_categories) {
            ++report.subjects_by_category[cat];
        }

        for (const auto& tp : rec.third_party_disclosures) {
            ++report.disclosures_by_third_party[tp];
        }
    }

    THEMIS_INFO("Generated CCPA report {}: {} subjects in window [{}, {})",
        report.report_id, report.total_subjects,
        window_start_ms, window_end_ms);

    return report;
}

} // namespace governance
} // namespace themis
