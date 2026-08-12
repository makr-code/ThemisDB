/**
 * @file iso27001_rules.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=14, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "governance/iso27001_rules.h"

#include <algorithm>
#include <chrono>
#include <sstream>

#include "utils/logger.h"

namespace themis {
namespace governance {

// ============================================================================
// Helper – current time in milliseconds
// ============================================================================

static int64_t iso27001NowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

// ============================================================================
// Helper – determine if a resource is classified/sensitive
// ============================================================================

static bool iso27001IsSensitiveResource(const PolicyRule &rule) {
    const auto &level = rule.classification_level;
    if (level == "vs-nfd" || level == "geheim" || level == "streng-geheim") {
        return true;
    }
    static const std::vector<std::string> kSensitiveKeywords
        = {"classified", "sensitive", "phi", "pii", "secret", "confidential"};
    for (const auto &res : rule.resources) {
        for (const auto &kw : kSensitiveKeywords) {
            if (res.find(kw) != std::string::npos) {
                return true;
            }
        }
    }
    return false;
}

// ============================================================================
// Iso27001EvidenceItem
// ============================================================================

nlohmann::json Iso27001EvidenceItem::toJson() const {
    nlohmann::json j = {{"evidence_id", evidence_id},
                        {"control_id", control_id},
                        {"evidence_type", evidence_type},
                        {"timestamp_ms", timestamp_ms},
                        {"resource", resource},
                        {"principal", principal},
                        {"action", action},
                        {"control_met", control_met},
                        {"detail", detail}};
    if (!metadata.is_null()) {
        j["metadata"] = metadata;
    }
    return j;
}

// ============================================================================
// Iso27001ControlResult
// ============================================================================

nlohmann::json Iso27001ControlResult::toJson() const {
    nlohmann::json j      = {{"control_id", control_id},
                             {"annex_section", annex_section},
                             {"title", title},
                             {"compliant", compliant},
                             {"description", description},
                             {"recommendation", recommendation},
                             {"missing_controls", missing_controls}};
    nlohmann::json ev_arr = nlohmann::json::array();
    for (const auto &ev : evidence) {
        ev_arr.push_back(ev.toJson());
    }
    j["evidence"] = ev_arr;
    return j;
}

// ============================================================================
// Iso27001AuditReport
// ============================================================================

nlohmann::json Iso27001AuditReport::toJson() const {
    nlohmann::json j       = {{"report_id", report_id},
                              {"generated_at_ms", generated_at_ms},
                              {"scope", scope},
                              {"total_controls", total_controls},
                              {"controls_met", controls_met},
                              {"compliance_score", compliance_score}};
    nlohmann::json res_arr = nlohmann::json::array();
    for (const auto &r : results) {
        res_arr.push_back(r.toJson());
    }
    j["results"]          = res_arr;
    nlohmann::json ev_arr = nlohmann::json::array();
    for (const auto &ev : evidence_items) {
        ev_arr.push_back(ev.toJson());
    }
    j["evidence_items"] = ev_arr;
    return j;
}

// ============================================================================
// Helper – build evidence item for a policy-rule evaluation
// ============================================================================

static Iso27001EvidenceItem makeIso27001Evidence(const std::string &evidence_id, const std::string &control_id,
                                                 const PolicyRule &rule, bool control_met, const std::string &detail) {
    Iso27001EvidenceItem ev;
    ev.evidence_id   = evidence_id;
    ev.control_id    = control_id;
    ev.evidence_type = "policy_rule";
    ev.timestamp_ms  = iso27001NowMs();
    ev.resource      = rule.resources.empty() ? "*" : rule.resources.front();
    ev.principal     = rule.required_roles.empty() ? "*" : rule.required_roles.front();
    ev.action        = rule.actions.empty() ? "*" : rule.actions.front();
    ev.control_met   = control_met;
    ev.detail        = detail;
    ev.metadata      = {{"rule_id", rule.id},
                        {"rule_name", rule.name},
                        {"classification_level", rule.classification_level},
                        {"require_encryption", rule.require_encryption},
                        {"require_signature", rule.require_signature},
                        {"audit_access", rule.audit_access},
                        {"audit_changes", rule.audit_changes},
                        {"retention_days", rule.retention_days},
                        {"allow_export", rule.allow_export}};
    return ev;
}

// ============================================================================
// A.9.1.2 – Access Control Policy
// ============================================================================

Iso27001ControlResult Iso27001A912Control::evaluate(const PolicyRule &rule) const {
    Iso27001ControlResult result;
    result.control_id    = id();
    result.annex_section = annexSection();
    result.title         = title();

    if (!rule.enabled) {
        result.compliant   = true;
        result.description = "Rule is disabled; A.9.1.2 not applicable.";
        return result;
    }

    std::vector<std::string> gaps;

    // A.9.1.2 requires that access is restricted to identified roles (least privilege).
    if (rule.required_roles.empty()) {
        gaps.push_back("required_roles must list at least one authorised role to enforce "
                       "least-privilege access control (ISO 27001 A.9.1.2)");
    }

    result.compliant        = gaps.empty();
    result.missing_controls = gaps;

    if (result.compliant) {
        result.description = "A.9.1.2 satisfied: access is restricted to "
                             "authorised roles on rule '"
                             + rule.name + "'.";
    } else {
        result.description    = "A.9.1.2 gap detected: access control policy "
                                "not fully enforced on rule '"
                                + rule.name + "'.";
        result.recommendation = "Populate required_roles with at least one authorised "
                                "role to restrict access to identified principals and "
                                "satisfy ISO 27001 A.9.1.2 least-privilege requirements.";
    }

    result.evidence.push_back(
        makeIso27001Evidence("a912-" + rule.id, id(), rule, result.compliant, result.description));
    return result;
}

// ============================================================================
// A.10.1.1 – Cryptography Policy
// ============================================================================

Iso27001ControlResult Iso27001A1011Control::evaluate(const PolicyRule &rule) const {
    Iso27001ControlResult result;
    result.control_id    = id();
    result.annex_section = annexSection();
    result.title         = title();

    if (!rule.enabled) {
        result.compliant   = true;
        result.description = "Rule is disabled; A.10.1.1 not applicable.";
        return result;
    }

    const bool sensitive = iso27001IsSensitiveResource(rule);
    std::vector<std::string> gaps;

    // A.10.1.1 requires encryption for sensitive or classified resources.
    if (sensitive && !rule.require_encryption) {
        gaps.push_back("require_encryption must be true for classified or sensitive resources "
                       "to satisfy the cryptographic controls policy (ISO 27001 A.10.1.1)");
    }

    result.compliant        = gaps.empty();
    result.missing_controls = gaps;

    if (result.compliant) {
        result.description = "A.10.1.1 satisfied: cryptographic controls are "
                             "correctly applied for rule '"
                             + rule.name + "'.";
    } else {
        result.description    = "A.10.1.1 gap detected: encryption not enforced for "
                                "sensitive resource on rule '"
                                + rule.name + "'.";
        result.recommendation = "Set require_encryption=true for all rules that cover "
                                "classified or sensitive resources to satisfy "
                                "ISO 27001 A.10.1.1 cryptography policy requirements.";
    }

    result.evidence.push_back(
        makeIso27001Evidence("a1011-" + rule.id, id(), rule, result.compliant, result.description));
    return result;
}

// ============================================================================
// A.12.4.1 – Event Logging
// ============================================================================

Iso27001ControlResult Iso27001A1241Control::evaluate(const PolicyRule &rule) const {
    Iso27001ControlResult result;
    result.control_id    = id();
    result.annex_section = annexSection();
    result.title         = title();

    if (!rule.enabled) {
        result.compliant   = true;
        result.description = "Rule is disabled; A.12.4.1 not applicable.";
        return result;
    }

    std::vector<std::string> gaps;

    // A.12.4.1 requires that access and change events are logged.
    if (!rule.audit_access) {
        gaps.push_back("audit_access must be true to record all access events (ISO 27001 A.12.4.1)");
    }
    if (!rule.audit_changes) {
        gaps.push_back("audit_changes must be true to record all modification events (ISO 27001 A.12.4.1)");
    }

    result.compliant        = gaps.empty();
    result.missing_controls = gaps;

    if (result.compliant) {
        result.description = "A.12.4.1 satisfied: event logging is enabled for "
                             "both access and changes on rule '"
                             + rule.name + "'.";
    } else {
        result.description    = "A.12.4.1 gap detected: event logging incomplete "
                                "on rule '"
                                + rule.name + "'.";
        result.recommendation = "Enable audit_access=true and audit_changes=true "
                                "on all active rules to produce a complete event log "
                                "as required by ISO 27001 A.12.4.1.";
    }

    result.evidence.push_back(
        makeIso27001Evidence("a1241-" + rule.id, id(), rule, result.compliant, result.description));
    return result;
}

// ============================================================================
// A.12.4.2 – Protection of Log Information
// ============================================================================

Iso27001ControlResult Iso27001A1242Control::evaluate(const PolicyRule &rule) const {
    Iso27001ControlResult result;
    result.control_id    = id();
    result.annex_section = annexSection();
    result.title         = title();

    if (!rule.enabled) {
        result.compliant   = true;
        result.description = "Rule is disabled; A.12.4.2 not applicable.";
        return result;
    }

    std::vector<std::string> gaps;

    const int kMinRetentionDays = 90;
    if (rule.retention_days < kMinRetentionDays) {
        gaps.push_back("retention_days must be >= 90 to preserve audit logs for forensic "
                       "investigation and compliance review (ISO 27001 A.12.4.2). Current: "
                       + std::to_string(rule.retention_days));
    }

    result.compliant        = gaps.empty();
    result.missing_controls = gaps;

    if (result.compliant) {
        result.description = "A.12.4.2 satisfied: log retention period is "
                             "adequate on rule '"
                             + rule.name + "' (" + std::to_string(rule.retention_days) + " days).";
    } else {
        result.description    = "A.12.4.2 gap detected: log retention period is "
                                "insufficient on rule '"
                                + rule.name + "'.";
        result.recommendation = "Set retention_days >= 90 to ensure audit logs are "
                                "preserved for a sufficient period as required by "
                                "ISO 27001 A.12.4.2.";
    }

    result.evidence.push_back(
        makeIso27001Evidence("a1242-" + rule.id, id(), rule, result.compliant, result.description));
    return result;
}

// ============================================================================
// A.13.2.3 – Electronic Messaging
// ============================================================================

Iso27001ControlResult Iso27001A1323Control::evaluate(const PolicyRule &rule) const {
    Iso27001ControlResult result;
    result.control_id    = id();
    result.annex_section = annexSection();
    result.title         = title();

    if (!rule.enabled) {
        result.compliant   = true;
        result.description = "Rule is disabled; A.13.2.3 not applicable.";
        return result;
    }

    std::vector<std::string> gaps;

    // A.13.2.3: information transmitted electronically must be protected.
    // Violation: allow_export=true AND require_encryption=false
    if (rule.allow_export && !rule.require_encryption) {
        gaps.push_back("allow_export=true with require_encryption=false permits unprotected "
                       "transmission of information (ISO 27001 A.13.2.3). Either disable "
                       "export or require encryption.");
    }

    result.compliant        = gaps.empty();
    result.missing_controls = gaps;

    if (result.compliant) {
        result.description = "A.13.2.3 satisfied: electronic messaging controls "
                             "are correctly configured on rule '"
                             + rule.name + "'.";
    } else {
        result.description    = "A.13.2.3 gap detected: information may be transmitted "
                                "without appropriate protection on rule '"
                                + rule.name + "'.";
        result.recommendation = "Set require_encryption=true when allow_export=true, "
                                "or disable export entirely, to satisfy ISO 27001 A.13.2.3 "
                                "electronic messaging protection requirements.";
    }

    result.evidence.push_back(
        makeIso27001Evidence("a1323-" + rule.id, id(), rule, result.compliant, result.description));
    return result;
}

// ============================================================================
// A.18.1.3 – Protection of Records
// ============================================================================

Iso27001ControlResult Iso27001A1813Control::evaluate(const PolicyRule &rule) const {
    Iso27001ControlResult result;
    result.control_id    = id();
    result.annex_section = annexSection();
    result.title         = title();

    if (!rule.enabled) {
        result.compliant   = true;
        result.description = "Rule is disabled; A.18.1.3 not applicable.";
        return result;
    }

    std::vector<std::string> gaps;

    // A.18.1.3 requires that records are retained for a positive period.
    if (rule.retention_days <= 0) {
        gaps.push_back("retention_days must be > 0 to protect records from loss or destruction "
                       "(ISO 27001 A.18.1.3). Current: "
                       + std::to_string(rule.retention_days));
    }

    result.compliant        = gaps.empty();
    result.missing_controls = gaps;

    if (result.compliant) {
        result.description = "A.18.1.3 satisfied: records retention is defined "
                             "on rule '"
                             + rule.name + "' (" + std::to_string(rule.retention_days) + " days).";
    } else {
        result.description    = "A.18.1.3 gap detected: no retention period defined "
                                "on rule '"
                                + rule.name + "'.";
        result.recommendation = "Set retention_days > 0 to ensure records are retained "
                                "against loss or destruction as required by "
                                "ISO 27001 A.18.1.3.";
    }

    result.evidence.push_back(
        makeIso27001Evidence("a1813-" + rule.id, id(), rule, result.compliant, result.description));
    return result;
}

// ============================================================================
// Iso27001ControlSet
// ============================================================================

Iso27001ControlSet::Iso27001ControlSet() {
    controls_.push_back(std::make_shared<Iso27001A912Control>());
    controls_.push_back(std::make_shared<Iso27001A1011Control>());
    controls_.push_back(std::make_shared<Iso27001A1241Control>());
    controls_.push_back(std::make_shared<Iso27001A1242Control>());
    controls_.push_back(std::make_shared<Iso27001A1323Control>());
    controls_.push_back(std::make_shared<Iso27001A1813Control>());

    THEMIS_DEBUG("Iso27001ControlSet initialized with {} control evaluators", controls_.size());
}

std::vector<Iso27001ControlResult> Iso27001ControlSet::evaluateRule(const PolicyRule &rule) const {
    std::vector<Iso27001ControlResult> results;
    results.reserve(controls_.size());
    for (const auto &ctrl : controls_) {
        results.push_back(ctrl->evaluate(rule));
    }
    return results;
}

bool Iso27001ControlSet::isRuleCompliant(const PolicyRule &rule) const {
    for (const auto &ctrl : controls_) {
        if (!ctrl->evaluate(rule).compliant) {
            return false;
        }
    }
    return true;
}

Iso27001AuditReport Iso27001ControlSet::generateReport(const PolicyManager &policy_mgr,
                                                       const std::string &scope) const {
    Iso27001AuditReport report;

    const int64_t ts = iso27001NowMs();
    std::ostringstream id_ss;
    id_ss << "iso27001-" << ts;
    report.report_id       = id_ss.str();
    report.generated_at_ms = ts;
    report.scope           = scope;

    const auto rules = policy_mgr.listRules();
    int total        = 0;
    int met          = 0;

    for (const auto &rule : rules) {
        if (!rule.enabled) {
            continue;
        }

        auto ctrl_results = evaluateRule(rule);
        for (auto &cr : ctrl_results) {
            total++;
            if (cr.compliant) {
                met++;
            }

            for (const auto &ev : cr.evidence) {
                report.evidence_items.push_back(ev);
            }

            report.results.push_back(std::move(cr));
        }
    }

    report.total_controls   = total;
    report.controls_met     = met;
    report.compliance_score = (total > 0) ? (static_cast<double>(met) / static_cast<double>(total)) * 100.0 : 100.0;

    THEMIS_INFO("ISO 27001 audit report generated: {}/{} controls met, score={:.1f}", met, total,
                report.compliance_score);
    return report;
}

void Iso27001ControlSet::collectEvidence(const std::string &resource, const std::string &action,
                                         const std::string &principal, bool access_granted, bool encrypted) {
    std::lock_guard<std::mutex> lock(evidence_mutex_);

    Iso27001EvidenceItem ev;
    {
        std::ostringstream id_ss;
        id_ss << "ev-" << (++evidence_counter_) << "-" << iso27001NowMs();
        ev.evidence_id = id_ss.str();
    }
    ev.control_id    = "A.9.1.2";
    ev.evidence_type = "access_log";
    ev.timestamp_ms  = iso27001NowMs();
    ev.resource      = resource;
    ev.principal     = principal;
    ev.action        = action;
    ev.control_met   = access_granted && encrypted;
    ev.detail        = std::string("access_granted=") + (access_granted ? "true" : "false")
                       + " encrypted=" + (encrypted ? "true" : "false");
    ev.metadata      = {{"access_granted", access_granted}, {"encrypted", encrypted}};

    evidence_items_.push_back(std::move(ev));
}

std::vector<Iso27001EvidenceItem> Iso27001ControlSet::getEvidence() const {
    std::lock_guard<std::mutex> lock(evidence_mutex_);
    return evidence_items_;
}

void Iso27001ControlSet::clearEvidence() {
    std::lock_guard<std::mutex> lock(evidence_mutex_);
    evidence_items_.clear();
}

} // namespace governance
} // namespace themis
