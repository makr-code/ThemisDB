/**
 * @file soc2_controls.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 80/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=0, M=12, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "governance/soc2_controls.h"

#include <algorithm>
#include <chrono>
#include <sstream>

#include "utils/logger.h"

namespace themis {
namespace governance {

// ============================================================================
// Helper – current time in milliseconds
// ============================================================================

static int64_t nowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

// ============================================================================
// Soc2EvidenceItem
// ============================================================================

nlohmann::json Soc2EvidenceItem::toJson() const {
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
// Soc2ControlResult
// ============================================================================

nlohmann::json Soc2ControlResult::toJson() const {
    nlohmann::json j      = {{"control_id", control_id},
                             {"criteria", criteria},
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
// Soc2AuditReport
// ============================================================================

nlohmann::json Soc2AuditReport::toJson() const {
    nlohmann::json j           = {{"report_id", report_id},
                                  {"generated_at_ms", generated_at_ms},
                                  {"scope", scope},
                                  {"total_controls", total_controls},
                                  {"controls_met", controls_met},
                                  {"compliance_score", compliance_score}};
    nlohmann::json results_arr = nlohmann::json::array();
    for (const auto &r : results) {
        results_arr.push_back(r.toJson());
    }
    j["results"]          = results_arr;
    nlohmann::json ev_arr = nlohmann::json::array();
    for (const auto &ev : evidence_items) {
        ev_arr.push_back(ev.toJson());
    }
    j["evidence_items"] = ev_arr;
    return j;
}

// ============================================================================
// Helpers shared by control evaluators
// ============================================================================

/// Classify a resource/rule as "sensitive" when the classification level or
/// resource name indicates it holds personal, health, or confidential data.
static bool isSensitiveResource(const PolicyRule &rule) {
    const std::string &lvl = rule.classification_level;
    if (lvl == "vs-nfd" || lvl == "geheim" || lvl == "streng-geheim") {
        return true;
    }
    // Also treat rules that mention common PII/PHI resource patterns as sensitive
    for (const auto &res : rule.resources) {
        const std::string r = [&] {
            std::string s = res;
            std::transform(s.begin(), s.end(), s.begin(), ::tolower);
            return s;
        }();
        if (r.find("user") != std::string::npos || r.find("patient") != std::string::npos
            || r.find("health") != std::string::npos || r.find("pii") != std::string::npos
            || r.find("phi") != std::string::npos || r.find("sensitive") != std::string::npos
            || r.find("secret") != std::string::npos || r.find("key") != std::string::npos) {
            return true;
        }
    }
    return false;
}

/// Build a minimal evidence item for a rule-based control evaluation.
static Soc2EvidenceItem makeRuleEvidence(const std::string &evidence_id, const std::string &control_id,
                                         const PolicyRule &rule, bool control_met, const std::string &detail) {
    Soc2EvidenceItem ev;
    ev.evidence_id   = evidence_id;
    ev.control_id    = control_id;
    ev.evidence_type = "policy_rule";
    ev.timestamp_ms  = nowMs();
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
// CC6.1 – Logical Access Controls: field-level encryption requirement
// ============================================================================

Soc2ControlResult Soc2Cc6Control::evaluate(const PolicyRule &rule) const {
    Soc2ControlResult result;
    result.control_id = id();
    result.criteria   = criteria();
    result.title      = title();

    if (!rule.enabled) {
        // Disabled rules are not enforced; treat as not applicable (pass).
        result.compliant   = true;
        result.description = "Rule is disabled; CC6.1 not applicable.";
        return result;
    }

    const bool sensitive = isSensitiveResource(rule);
    std::vector<std::string> gaps;

    // CC6.1 requires that field-level encryption is enforced for any rule
    // protecting a sensitive resource.  The require_encryption flag signals
    // that every document written under this rule must be encrypted before
    // storage – this is the policy-layer counterpart of the storage engine's
    // field-level AES-256-GCM encryption (not simulated; enforced at write time
    // by the storage layer when require_content_encryption is set on the
    // PolicyDecision derived from this rule).
    if (sensitive && !rule.require_encryption) {
        gaps.push_back("require_encryption must be true for sensitive resources (CC6.1)");
    }

    // CC6.1 also requires that access is restricted to identified roles.
    if (rule.required_roles.empty()) {
        gaps.push_back("required_roles must list at least one authorized role (CC6.1)");
    }

    result.compliant        = gaps.empty();
    result.missing_controls = gaps;

    if (result.compliant) {
        result.description = "CC6.1 satisfied: sensitive resources require "
                             "field-level encryption and role-based access control.";
    } else {
        result.description
            = "CC6.1 gap detected: " + std::to_string(gaps.size()) + " missing control(s) on rule '" + rule.name + "'.";
        result.recommendation = "Set require_encryption=true and populate "
                                "required_roles for all rules covering sensitive "
                                "or classified resources.";
    }

    result.evidence.push_back(makeRuleEvidence("cc6-" + rule.id, id(), rule, result.compliant, result.description));
    return result;
}

// ============================================================================
// CC7.2 – System Operations: change detection and audit logging
// ============================================================================

Soc2ControlResult Soc2Cc7Control::evaluate(const PolicyRule &rule) const {
    Soc2ControlResult result;
    result.control_id = id();
    result.criteria   = criteria();
    result.title      = title();

    if (!rule.enabled) {
        result.compliant   = true;
        result.description = "Rule is disabled; CC7.2 not applicable.";
        return result;
    }

    std::vector<std::string> gaps;

    // CC7.2: Detect and respond to unauthorized changes.
    // Audit access logging captures every read/query so that anomalous patterns
    // are detectable.  Audit change logging captures mutations so that
    // unauthorized modifications are recorded for forensic analysis.
    if (!rule.audit_access) {
        gaps.push_back("audit_access must be true to detect unauthorized queries (CC7.2)");
    }
    if (!rule.audit_changes) {
        gaps.push_back("audit_changes must be true to detect unauthorized mutations (CC7.2)");
    }

    result.compliant        = gaps.empty();
    result.missing_controls = gaps;

    if (result.compliant) {
        result.description = "CC7.2 satisfied: both access and change audit "
                             "logging are enabled for rule '"
                             + rule.name + "'.";
    } else {
        result.description    = "CC7.2 gap detected: audit logging is incomplete "
                                "on rule '"
                                + rule.name + "'.";
        result.recommendation = "Enable audit_access=true and audit_changes=true "
                                "on all active rules to satisfy SOC 2 CC7.2 "
                                "system-operations monitoring requirements.";
    }

    result.evidence.push_back(makeRuleEvidence("cc7-" + rule.id, id(), rule, result.compliant, result.description));
    return result;
}

// ============================================================================
// CC8.1 – Change Management: authorized change procedures
// ============================================================================

Soc2ControlResult Soc2Cc8Control::evaluate(const PolicyRule &rule) const {
    Soc2ControlResult result;
    result.control_id = id();
    result.criteria   = criteria();
    result.title      = title();

    if (!rule.enabled) {
        result.compliant   = true;
        result.description = "Rule is disabled; CC8.1 not applicable.";
        return result;
    }

    std::vector<std::string> gaps;

    // CC8.1: Changes to infrastructure, data, and procedures must be
    // authorized.  require_signature ensures a cryptographic approval step
    // before data is modified; audit_changes produces the tamper-evident
    // log required by auditors to verify the change was authorized.
    if (!rule.require_signature) {
        gaps.push_back("require_signature must be true to enforce authorized "
                       "change approval (CC8.1)");
    }
    if (!rule.audit_changes) {
        gaps.push_back("audit_changes must be true to produce a change log "
                       "for CC8.1 verification");
    }

    result.compliant        = gaps.empty();
    result.missing_controls = gaps;

    if (result.compliant) {
        result.description
            = "CC8.1 satisfied: rule '" + rule.name + "' requires authorized signatures and audits all changes.";
    } else {
        result.description    = "CC8.1 gap: change-management controls are "
                                "incomplete on rule '"
                                + rule.name + "'.";
        result.recommendation = "Set require_signature=true and audit_changes=true "
                                "on all rules governing critical or classified resources.";
    }

    result.evidence.push_back(makeRuleEvidence("cc8-" + rule.id, id(), rule, result.compliant, result.description));
    return result;
}

// ============================================================================
// A1.1 – Availability: data retention and recovery
// ============================================================================

Soc2ControlResult Soc2A1Control::evaluate(const PolicyRule &rule) const {
    Soc2ControlResult result;
    result.control_id = id();
    result.criteria   = criteria();
    result.title      = title();

    if (!rule.enabled) {
        result.compliant   = true;
        result.description = "Rule is disabled; A1.1 not applicable.";
        return result;
    }

    std::vector<std::string> gaps;

    // A1.1: The entity maintains capacity and availability commitments.
    // A finite, positive retention_days ensures that data is neither purged
    // prematurely (breaking availability) nor retained indefinitely (violating
    // other obligations).  Values of 0 mean "no retention policy" which
    // leaves availability undefined.
    if (rule.retention_days <= 0) {
        gaps.push_back("retention_days must be a positive value to define "
                       "data availability commitments (A1.1)");
    }

    result.compliant        = gaps.empty();
    result.missing_controls = gaps;

    if (result.compliant) {
        result.description = "A1.1 satisfied: rule '" + rule.name + "' defines a retention period of "
                             + std::to_string(rule.retention_days) + " day(s).";
    } else {
        result.description    = "A1.1 gap: availability commitments are undefined "
                                "on rule '"
                                + rule.name + "' (retention_days <= 0).";
        result.recommendation = "Set retention_days to a positive value that "
                                "reflects the service availability commitment for "
                                "this resource.";
    }

    result.evidence.push_back(makeRuleEvidence("a1-" + rule.id, id(), rule, result.compliant, result.description));
    return result;
}

// ============================================================================
// C1.1 – Confidentiality: data classification and protection
// ============================================================================

Soc2ControlResult Soc2C1Control::evaluate(const PolicyRule &rule) const {
    Soc2ControlResult result;
    result.control_id = id();
    result.criteria   = criteria();
    result.title      = title();

    if (!rule.enabled) {
        result.compliant   = true;
        result.description = "Rule is disabled; C1.1 not applicable.";
        return result;
    }

    const bool sensitive = isSensitiveResource(rule);
    std::vector<std::string> gaps;

    if (sensitive) {
        // C1.1: Confidential information must be identified and protected.
        // Rules that cover confidential resources must:
        //   1. Enforce encryption to protect data at rest.
        //   2. Disable unrestricted export to prevent disclosure.
        //   3. Apply at least "standard" redaction in query results.
        if (!rule.require_encryption) {
            gaps.push_back("require_encryption must be true for confidential "
                           "resources (C1.1)");
        }
        if (rule.allow_export) {
            gaps.push_back("allow_export should be false (or gated by signature) "
                           "for confidential resources (C1.1)");
        }
        if (rule.redaction_level == "none") {
            gaps.push_back("redaction_level must be 'standard' or 'strict' for "
                           "confidential resources (C1.1)");
        }
    }

    result.compliant        = gaps.empty();
    result.missing_controls = gaps;

    if (result.compliant) {
        if (sensitive) {
            result.description = "C1.1 satisfied: confidential resource is "
                                 "protected by encryption, export restriction, "
                                 "and appropriate redaction on rule '"
                                 + rule.name + "'.";
        } else {
            result.description = "C1.1 satisfied: rule '" + rule.name
                                 + "' does not govern confidential resources; "
                                   "no additional controls required.";
        }
    } else {
        result.description    = "C1.1 gap: confidential data protection controls "
                                "are incomplete on rule '"
                                + rule.name + "'.";
        result.recommendation = "For confidential resources set require_encryption=true, "
                                "allow_export=false, and redaction_level to 'standard' "
                                "or 'strict'.";
    }

    result.evidence.push_back(makeRuleEvidence("c1-" + rule.id, id(), rule, result.compliant, result.description));
    return result;
}

// ============================================================================
// PI1.2 – Processing Integrity: audit trail completeness
// ============================================================================

Soc2ControlResult Soc2Pi1Control::evaluate(const PolicyRule &rule) const {
    Soc2ControlResult result;
    result.control_id = id();
    result.criteria   = criteria();
    result.title      = title();

    if (!rule.enabled) {
        result.compliant   = true;
        result.description = "Rule is disabled; PI1.2 not applicable.";
        return result;
    }

    std::vector<std::string> gaps;

    // PI1.2: System processing is complete, valid, accurate, timely, and
    // authorized.  Audit access logging is the primary mechanism for verifying
    // completeness – every access event must be recorded so that an auditor
    // can confirm the data was processed as intended and that no events were
    // silently dropped or bypassed.
    if (!rule.audit_access) {
        gaps.push_back("audit_access must be true to verify processing "
                       "completeness and integrity (PI1.2)");
    }

    result.compliant        = gaps.empty();
    result.missing_controls = gaps;

    if (result.compliant) {
        result.description = "PI1.2 satisfied: audit_access=true ensures a "
                             "complete and tamper-evident access log for rule '"
                             + rule.name + "'.";
    } else {
        result.description    = "PI1.2 gap: processing integrity cannot be "
                                "verified without access audit logging on rule '"
                                + rule.name + "'.";
        result.recommendation = "Enable audit_access=true on all active rules "
                                "to produce a complete audit trail required by "
                                "SOC 2 PI1.2.";
    }

    result.evidence.push_back(makeRuleEvidence("pi1-" + rule.id, id(), rule, result.compliant, result.description));
    return result;
}

// ============================================================================
// Soc2ControlSet
// ============================================================================

Soc2ControlSet::Soc2ControlSet() {
    controls_.push_back(std::make_shared<Soc2Cc6Control>());
    controls_.push_back(std::make_shared<Soc2Cc7Control>());
    controls_.push_back(std::make_shared<Soc2Cc8Control>());
    controls_.push_back(std::make_shared<Soc2A1Control>());
    controls_.push_back(std::make_shared<Soc2C1Control>());
    controls_.push_back(std::make_shared<Soc2Pi1Control>());
}

std::vector<Soc2ControlResult> Soc2ControlSet::evaluateRule(const PolicyRule &rule) const {
    std::vector<Soc2ControlResult> results;
    results.reserve(controls_.size());
    for (const auto &ctrl : controls_) {
        results.push_back(ctrl->evaluate(rule));
    }
    return results;
}

bool Soc2ControlSet::isRuleCompliant(const PolicyRule &rule) const {
    for (const auto &ctrl : controls_) {
        if (!ctrl->evaluate(rule).compliant) {
            return false;
        }
    }
    return true;
}

Soc2AuditReport Soc2ControlSet::generateReport(const PolicyManager &policy_mgr, const std::string &scope) const {
    Soc2AuditReport report;

    // Generate a deterministic report ID from current time
    const int64_t ts = nowMs();
    std::ostringstream id_ss;
    id_ss << "soc2-" << ts;
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

            // Attach evidence items to the top-level report as well
            for (const auto &ev : cr.evidence) {
                report.evidence_items.push_back(ev);
            }

            report.results.push_back(std::move(cr));
        }
    }

    report.total_controls   = total;
    report.controls_met     = met;
    report.compliance_score = (total > 0) ? (static_cast<double>(met) / static_cast<double>(total)) * 100.0 : 100.0;

    THEMIS_INFO("SOC 2 audit report generated: {}/{} controls met, score={:.1f}", met, total, report.compliance_score);
    return report;
}

void Soc2ControlSet::collectEvidence(const std::string &resource, const std::string &action,
                                     const std::string &principal, bool access_granted, bool encrypted) {
    std::lock_guard<std::mutex> lock(evidence_mutex_);

    Soc2EvidenceItem ev;
    {
        std::ostringstream id_ss;
        id_ss << "ev-" << (++evidence_counter_) << "-" << nowMs();
        ev.evidence_id = id_ss.str();
    }
    ev.control_id    = "CC6.1";
    ev.evidence_type = "access_log";
    ev.timestamp_ms  = nowMs();
    ev.resource      = resource;
    ev.principal     = principal;
    ev.action        = action;
    ev.control_met   = access_granted && encrypted;
    ev.detail        = std::string("access_granted=") + (access_granted ? "true" : "false")
                       + " encrypted=" + (encrypted ? "true" : "false");
    ev.metadata      = {{"access_granted", access_granted}, {"encrypted", encrypted}};

    evidence_items_.push_back(std::move(ev));
}

std::vector<Soc2EvidenceItem> Soc2ControlSet::getEvidence() const {
    std::lock_guard<std::mutex> lock(evidence_mutex_);
    return evidence_items_;
}

void Soc2ControlSet::clearEvidence() {
    std::lock_guard<std::mutex> lock(evidence_mutex_);
    evidence_items_.clear();
}

} // namespace governance
} // namespace themis
