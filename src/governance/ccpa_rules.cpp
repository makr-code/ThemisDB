/**
 * @file ccpa_rules.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @author makr-code
 * @version 0.0.15
 * @date 2026-06-02 11:49:05
 * @note Maturity: 🟡 RELEASE-CANDIDATE
 * @note Score: 79/100
 * @note Lines: 257
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=5, L=0
 * @note PR History (last 5): #3560 docs(governance): reality-c... (2026-03-12) | #2863 feat(governance): CCPA/CPRA... (2026-03-12)
 * @note Status: Release Candidate
 * @note This block is auto-generated and will be overwritten.
 */


#include "governance/ccpa_rules.h"

#include <algorithm>
#include <chrono>

#include "utils/logger.h"

namespace themis {
namespace governance {

// ============================================================================
// CcpaRuleEvalResult
// ============================================================================

nlohmann::json CcpaRuleEvalResult::toJson() const {
    return {{"rule_id", rule_id},
            {"ccpa_check_id", ccpa_check_id},
            {"compliant", compliant},
            {"description", description},
            {"recommendation", recommendation}};
}

// ============================================================================
// DataSubjectRequest
// ============================================================================

nlohmann::json DataSubjectRequest::toJson() const {
    return {{"request_id", request_id}, {"subject_id", subject_id}, {"request_type", request_type},
            {"timestamp", timestamp},   {"status", status},         {"denial_reason", denial_reason}};
}

// ============================================================================
// RightToKnow
// ============================================================================

bool RightToKnow::evaluate(const PolicyRule &rule) const {
    // A rule satisfies right-to-know when it has audit access enabled so
    // that the data inventory required by CCPA §1798.100 can be produced.
    if (!rule.enabled) {
        return true; // Disabled rules don't need to satisfy active requirements.
    }
    return rule.audit_access;
}

// ============================================================================
// RightToDelete
// ============================================================================

bool RightToDelete::evaluate(const PolicyRule &rule) const {
    if (!rule.enabled) {
        return true;
    }
    // A rule satisfies right-to-delete when:
    //   1. It audits changes (deletion events are traceable).
    //   2. The retention period is not set to an unreasonably long value
    //      without justification (proxy: <= 3650 days / 10 years).
    const bool audit_ok     = rule.audit_changes;
    const bool retention_ok = (rule.retention_days > 0 && rule.retention_days <= 3650);
    return audit_ok && retention_ok;
}

// ============================================================================
// OptOutOfSale
// ============================================================================

bool OptOutOfSale::evaluate(const PolicyRule &rule) const {
    if (!rule.enabled) {
        return true;
    }
    // A rule satisfies opt-out-of-sale when:
    //   • Export is disabled by default (allow_export=false), OR
    //   • A signature/consent is required before export (require_signature=true).
    // This ensures that for opted-out subjects, data cannot flow to third
    // parties unless the consumer explicitly consents.
    return !rule.allow_export || rule.require_signature;
}

// ============================================================================
// DataPortability
// ============================================================================

bool DataPortability::evaluate(const PolicyRule &rule) const {
    if (!rule.enabled) {
        return true;
    }
    // A rule satisfies data portability when either:
    //   • Direct automated export is allowed (allow_export=true), OR
    //   • Audit access is enabled (audit_access=true) so that operators can
    //     discover and manually fulfil consumer portability requests even when
    //     automated export is restricted.
    // A rule with allow_export=false AND audit_access=false leaves no viable
    // path to honour a portability request and is therefore non-compliant.
    return rule.allow_export || rule.audit_access;
}

// ============================================================================
// CcpaRuleSet
// ============================================================================

CcpaRuleSet::CcpaRuleSet() {
    rules_.push_back(std::make_shared<RightToKnow>());
    rules_.push_back(std::make_shared<RightToDelete>());
    rules_.push_back(std::make_shared<OptOutOfSale>());
    rules_.push_back(std::make_shared<DataPortability>());

    THEMIS_DEBUG("CcpaRuleSet initialized with {} rule evaluators", rules_.size());
}

void CcpaRuleSet::addOptOut(const std::string &subject_id) {
    if (subject_id.empty()) {
        return;
    }
    std::lock_guard<std::mutex> lock(opt_out_mutex_);
    opt_out_subjects_.insert(subject_id);
    THEMIS_INFO("CCPA: subject '{}' added to opt-out registry", subject_id);
}

void CcpaRuleSet::removeOptOut(const std::string &subject_id) {
    std::lock_guard<std::mutex> lock(opt_out_mutex_);
    opt_out_subjects_.erase(subject_id);
    THEMIS_INFO("CCPA: subject '{}' removed from opt-out registry", subject_id);
}

bool CcpaRuleSet::isOptedOut(const std::string &subject_id) const {
    if (subject_id.empty()) {
        return false;
    }
    std::lock_guard<std::mutex> lock(opt_out_mutex_);
    return opt_out_subjects_.count(subject_id) > 0;
}

void CcpaRuleSet::setOptOutRegistry(const std::unordered_set<std::string> &subjects) {
    std::lock_guard<std::mutex> lock(opt_out_mutex_);
    opt_out_subjects_ = subjects;
    THEMIS_INFO("CCPA: opt-out registry replaced with {} subjects", subjects.size());
}

size_t CcpaRuleSet::optOutCount() const {
    std::lock_guard<std::mutex> lock(opt_out_mutex_);
    return opt_out_subjects_.size();
}

std::vector<CcpaRuleEvalResult> CcpaRuleSet::evaluateRule(const PolicyRule &rule) const {
    std::vector<CcpaRuleEvalResult> results;
    results.reserve(rules_.size());

    for (const auto &ccpa_rule : rules_) {
        CcpaRuleEvalResult res;
        res.rule_id       = rule.id;
        res.ccpa_check_id = ccpa_rule->id();
        res.compliant     = ccpa_rule->evaluate(rule);

        if (res.compliant) {
            res.description = "Rule '" + rule.name + "' satisfies " + ccpa_rule->id();
        } else {
            res.description    = "Rule '" + rule.name + "' does not satisfy " + ccpa_rule->id();
            res.recommendation = ccpa_rule->description();
        }
        results.push_back(std::move(res));
    }

    return results;
}

bool CcpaRuleSet::isRuleCompliant(const PolicyRule &rule) const {
    for (const auto &ccpa_rule : rules_) {
        if (!ccpa_rule->evaluate(rule)) {
            return false;
        }
    }
    return true;
}

std::vector<std::string> CcpaRuleSet::detectHipaaConflicts(const PolicyRule &rule) const {
    std::vector<std::string> conflicts;

    if (!rule.enabled) {
        return conflicts;
    }

    // HIPAA mandates audit_access for PHI resources (audit controls).
    // CCPA right-to-delete requires audit_changes.
    // Conflict: HIPAA demands audit_access=true, but if audit_changes is
    // simultaneously false, deletion events are invisible, violating CCPA.
    if (rule.audit_access && !rule.audit_changes) {
        conflicts.push_back("Rule '" + rule.id
                            + "': HIPAA requires audit_access=true but CCPA "
                              "right-to-delete requires audit_changes=true. Enable audit_changes "
                              "to satisfy both frameworks simultaneously.");
    }

    // HIPAA requires retention of PHI for 6 years (2190 days).
    // CCPA right-to-delete allows consumers to request deletion before that.
    // Conflict: retention_days < 2190 may violate HIPAA; >= 2190 may delay CCPA deletion.
    // Flag when a rule has audit_access (HIPAA indicator) but a short retention
    // that would prevent meeting the 6-year HIPAA minimum.
    const int kHipaaMinRetentionDays = 2190; // 6 years
    if (rule.audit_access && rule.retention_days > 0 && rule.retention_days < kHipaaMinRetentionDays) {
        conflicts.push_back("Rule '" + rule.id + "': retention_days=" + std::to_string(rule.retention_days)
                            + " may be insufficient for HIPAA (min " + std::to_string(kHipaaMinRetentionDays)
                            + " days). Review whether CCPA right-to-delete applies to HIPAA-covered PHI.");
    }

    return conflicts;
}

void CcpaRuleSet::recordRequest(const DataSubjectRequest &request) {
    std::lock_guard<std::mutex> lock(requests_mutex_);
    requests_.push_back(request);
    THEMIS_INFO("CCPA: recorded {} request for subject '{}'", request.request_type, request.subject_id);
}

std::vector<DataSubjectRequest> CcpaRuleSet::getRequestsForSubject(const std::string &subject_id) const {
    std::lock_guard<std::mutex> lock(requests_mutex_);
    std::vector<DataSubjectRequest> result;
    for (const auto &req : requests_) {
        if (req.subject_id == subject_id) {
            result.push_back(req);
        }
    }
    return result;
}

std::vector<DataSubjectRequest> CcpaRuleSet::getRequestsByType(const std::string &request_type, int64_t start_time,
                                                               int64_t end_time) const {
    std::lock_guard<std::mutex> lock(requests_mutex_);
    std::vector<DataSubjectRequest> result;
    for (const auto &req : requests_) {
        if (req.request_type == request_type && req.timestamp >= start_time && req.timestamp <= end_time) {
            result.push_back(req);
        }
    }
    return result;
}

int CcpaRuleSet::countOptOutRequests(int64_t start_time, int64_t end_time) const {
    std::lock_guard<std::mutex> lock(requests_mutex_);
    int count = 0;
    for (const auto &req : requests_) {
        if (req.request_type == "opt_out_of_sale" && req.timestamp >= start_time && req.timestamp <= end_time) {
            ++count;
        }
    }
    return count;
}

} // namespace governance
} // namespace themis
