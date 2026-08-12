/**
 * @file hipaa_rules.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @author makr-code
 * @version 0.0.10
 * @date 2026-06-02 11:49:05
 * @note Maturity: 🟡 RELEASE-CANDIDATE
 * @note Score: 79/100
 * @note Lines: 165
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0
 * @note PR History (last 5): #4484 feat(governance): add ISO 2... (2026-04-11)
 * @note Status: Release Candidate
 * @note This block is auto-generated and will be overwritten.
 */


#include "governance/hipaa_rules.h"
#include "utils/logger.h"

namespace themis {
namespace governance {

// ============================================================================
// HipaaRuleEvalResult
// ============================================================================

nlohmann::json HipaaRuleEvalResult::toJson() const {
    return {
        {"rule_id",        rule_id},
        {"hipaa_check_id", hipaa_check_id},
        {"requirement",    requirement},
        {"compliant",      compliant},
        {"description",    description},
        {"recommendation", recommendation}
    };
}

// ============================================================================
// HipaaAccessControl – §164.312(a)(1)
// ============================================================================

bool HipaaAccessControl::evaluate(const PolicyRule& rule) const {
    if (!rule.enabled) {
        return true; // Disabled rules are not in force; exempt from active checks.
    }
    // §164.312(a)(1) requires that only persons or software programs that have
    // been granted access rights may access ePHI. required_roles must be
    // non-empty to identify and restrict those access rights.
    return !rule.required_roles.empty();
}

// ============================================================================
// HipaaEncryption – §164.312(a)(2)(iv)
// ============================================================================

bool HipaaEncryption::evaluate(const PolicyRule& rule) const {
    if (!rule.enabled) {
        return true;
    }
    // §164.312(a)(2)(iv) requires a mechanism to encrypt ePHI.
    // require_encryption=true signals that the storage/query layer must apply
    // encryption to all data governed by this rule.
    return rule.require_encryption;
}

// ============================================================================
// HipaaAuditControls – §164.312(b)
// ============================================================================

bool HipaaAuditControls::evaluate(const PolicyRule& rule) const {
    if (!rule.enabled) {
        return true;
    }
    // §164.312(b) mandates hardware, software, and procedural mechanisms that
    // record and examine activity in information systems containing ePHI.
    // audit_access=true ensures every read or query event is logged.
    return rule.audit_access;
}

// ============================================================================
// HipaaIntegrityControls – §164.312(c)(1)
// ============================================================================

bool HipaaIntegrityControls::evaluate(const PolicyRule& rule) const {
    if (!rule.enabled) {
        return true;
    }
    // §164.312(c)(1) requires policies and procedures to protect ePHI from
    // improper alteration or destruction. audit_changes=true ensures that all
    // modification events are recorded, enabling detection of improper changes.
    return rule.audit_changes;
}

// ============================================================================
// HipaaTransmissionSecurity – §164.312(e)(2)(ii)
// ============================================================================

bool HipaaTransmissionSecurity::evaluate(const PolicyRule& rule) const {
    if (!rule.enabled) {
        return true;
    }
    // §164.312(e)(2)(ii): ePHI must be encrypted whenever it is transmitted.
    // Transmission is safe when either:
    //   1. Export is disabled altogether (no ePHI leaves the system), OR
    //   2. Encryption is required (ePHI can only be transmitted encrypted).
    // The violation pattern is: allow_export=true AND require_encryption=false.
    return !rule.allow_export || rule.require_encryption;
}

// ============================================================================
// HipaaRetention – §164.530(j)
// ============================================================================

bool HipaaRetention::evaluate(const PolicyRule& rule) const {
    if (!rule.enabled) {
        return true;
    }
    // §164.530(j) requires documentation to be retained for at least 6 years.
    // 6 years = 6 * 365 = 2190 days.
    const int kHipaaMinRetentionDays = 2190;
    return rule.retention_days >= kHipaaMinRetentionDays;
}

// ============================================================================
// HipaaRuleSet
// ============================================================================

HipaaRuleSet::HipaaRuleSet() {
    rules_.push_back(std::make_shared<HipaaAccessControl>());
    rules_.push_back(std::make_shared<HipaaEncryption>());
    rules_.push_back(std::make_shared<HipaaAuditControls>());
    rules_.push_back(std::make_shared<HipaaIntegrityControls>());
    rules_.push_back(std::make_shared<HipaaTransmissionSecurity>());
    rules_.push_back(std::make_shared<HipaaRetention>());

    THEMIS_DEBUG("HipaaRuleSet initialized with {} rule evaluators", rules_.size());
}

std::vector<HipaaRuleEvalResult> HipaaRuleSet::evaluateRule(const PolicyRule& rule) const {
    std::vector<HipaaRuleEvalResult> results;
    results.reserve(rules_.size());

    for (const auto& hipaa_rule : rules_) {
        HipaaRuleEvalResult res;
        res.rule_id        = rule.id;
        res.hipaa_check_id = hipaa_rule->id();
        res.requirement    = hipaa_rule->id();
        res.compliant      = hipaa_rule->evaluate(rule);

        if (res.compliant) {
            res.description = "Rule '" + rule.name + "' satisfies " + hipaa_rule->id();
        } else {
            res.description    = "Rule '" + rule.name + "' does not satisfy " + hipaa_rule->id();
            res.recommendation = hipaa_rule->description();
        }
        results.push_back(std::move(res));
    }

    return results;
}

bool HipaaRuleSet::isRuleCompliant(const PolicyRule& rule) const {
    for (const auto& hipaa_rule : rules_) {
        if (!hipaa_rule->evaluate(rule)) {
            return false;
        }
    }
    return true;
}

} // namespace governance
} // namespace themis
