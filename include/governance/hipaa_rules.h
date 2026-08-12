/**
 * @file hipaa_rules.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "governance/ccpa_rules.h"
#include "governance/policy_manager.h"
#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>

namespace themis {
namespace governance {

// ============================================================================
// Result types
// ============================================================================

/// Result of a HIPAA compliance rule evaluation for a single PolicyRule.
struct HipaaRuleEvalResult {
    std::string rule_id;           ///< ID of the evaluated PolicyRule
    std::string hipaa_check_id;    ///< ID of the HIPAA rule that was evaluated
    std::string requirement;       ///< HIPAA requirement reference (e.g., "§164.312(a)(1)")
    bool compliant = false;        ///< Whether the rule is HIPAA compliant
    std::string description;       ///< Human-readable result description
    std::string recommendation;    ///< Remediation recommendation if not compliant

    nlohmann::json toJson() const;
};

// ============================================================================
// HIPAA Concrete Rule Evaluators
// ============================================================================

/// HIPAA §164.312(a)(1) — Access Control: unique user identification and
/// access control mechanisms for PHI systems.
/// Verifies that required_roles is non-empty, ensuring that only identified
/// users with assigned roles may access PHI resources.
class HipaaAccessControl final : public IComplianceRule {
public:
    std::string id() const override { return "hipaa_164_312_a1_access_control"; }
    std::string framework() const override { return "HIPAA"; }
    std::string description() const override {
        return "HIPAA §164.312(a)(1): Implement technical policies and procedures "
               "for electronic information systems that maintain electronic protected "
               "health information (ePHI) to allow access only to those persons or "
               "software programs that have been granted access rights. Rules must "
               "define at least one required_role to enforce user identification and "
               "access control for PHI systems.";
    }
    /// Compliant when required_roles is non-empty (at least one role restricts access).
    bool evaluate(const PolicyRule& rule) const override;
};

/// HIPAA §164.312(a)(2)(iv) — Encryption and Decryption: mechanism to
/// encrypt and decrypt ePHI.
/// Verifies that require_encryption=true for PHI resources.
class HipaaEncryption final : public IComplianceRule {
public:
    std::string id() const override { return "hipaa_164_312_a2iv_encryption"; }
    std::string framework() const override { return "HIPAA"; }
    std::string description() const override {
        return "HIPAA §164.312(a)(2)(iv): Implement a mechanism to encrypt and "
               "decrypt electronic protected health information. Rules covering PHI "
               "resources must enable require_encryption=true to ensure that ePHI "
               "is rendered unreadable and indecipherable to unauthorised individuals.";
    }
    /// Compliant when require_encryption=true.
    bool evaluate(const PolicyRule& rule) const override;
};

/// HIPAA §164.312(b) — Audit Controls: hardware, software, and procedural
/// mechanisms that record and examine activity in information systems containing ePHI.
/// Verifies that audit_access=true.
class HipaaAuditControls final : public IComplianceRule {
public:
    std::string id() const override { return "hipaa_164_312_b_audit_controls"; }
    std::string framework() const override { return "HIPAA"; }
    std::string description() const override {
        return "HIPAA §164.312(b): Implement hardware, software, and procedural "
               "mechanisms that record and examine activity in information systems "
               "that contain or use electronic protected health information. Rules "
               "must enable audit_access=true so that all access to ePHI is recorded "
               "for investigation and compliance verification.";
    }
    /// Compliant when audit_access=true.
    bool evaluate(const PolicyRule& rule) const override;
};

/// HIPAA §164.312(c)(1) — Integrity Controls: protect ePHI from improper
/// alteration or destruction.
/// Verifies that audit_changes=true to detect unauthorised modifications.
class HipaaIntegrityControls final : public IComplianceRule {
public:
    std::string id() const override { return "hipaa_164_312_c1_integrity"; }
    std::string framework() const override { return "HIPAA"; }
    std::string description() const override {
        return "HIPAA §164.312(c)(1): Implement policies and procedures to protect "
               "electronic protected health information from improper alteration or "
               "destruction. Rules must enable audit_changes=true so that all "
               "modifications to ePHI are recorded, enabling detection of improper "
               "alteration or destruction.";
    }
    /// Compliant when audit_changes=true.
    bool evaluate(const PolicyRule& rule) const override;
};

/// HIPAA §164.312(e)(2)(ii) — Transmission Security: encryption of ePHI
/// in transit over electronic communications networks.
/// Verifies that allow_export=true requires require_encryption=true.
class HipaaTransmissionSecurity final : public IComplianceRule {
public:
    std::string id() const override { return "hipaa_164_312_e2ii_transmission"; }
    std::string framework() const override { return "HIPAA"; }
    std::string description() const override {
        return "HIPAA §164.312(e)(2)(ii): Implement a mechanism to encrypt electronic "
               "protected health information whenever deemed appropriate. Rules that "
               "permit export of ePHI (allow_export=true) must require encryption "
               "(require_encryption=true) so that data transmitted over networks is "
               "protected from unauthorised interception. Rules with allow_export=true "
               "and require_encryption=false are non-compliant.";
    }
    /// Compliant when either export is disabled OR encryption is required.
    bool evaluate(const PolicyRule& rule) const override;
};

/// HIPAA §164.530(j) — Documentation and Retention: retain required
/// documentation for 6 years from creation or when last in effect.
/// Verifies that retention_days >= 2190 (6 years).
class HipaaRetention final : public IComplianceRule {
public:
    std::string id() const override { return "hipaa_164_530_j_retention"; }
    std::string framework() const override { return "HIPAA"; }
    std::string description() const override {
        return "HIPAA §164.530(j): A covered entity must retain documentation "
               "required by this subpart for 6 years from the date of its creation "
               "or the date when it last was in effect, whichever is later. Rules "
               "must define retention_days >= 2190 (6 years) to satisfy the HIPAA "
               "minimum documentation retention requirement.";
    }
    /// Compliant when retention_days >= 2190.
    bool evaluate(const PolicyRule& rule) const override;
};

// ============================================================================
// HipaaRuleSet
// ============================================================================

/// Aggregates all HIPAA Security Rule evaluators and provides:
///   1. Per-rule compliance evaluation against the full HIPAA rule set.
///   2. Summary compliance status over a PolicyManager's rule set.
class HipaaRuleSet {
public:
    HipaaRuleSet();

    // ---- Rule evaluation ------------------------------------------------

    /// Evaluate all HIPAA rules against a single PolicyRule.
    /// @return A list of evaluation results, one per HIPAA rule.
    std::vector<HipaaRuleEvalResult> evaluateRule(const PolicyRule& rule) const;

    /// Return true if the PolicyRule satisfies all HIPAA checks.
    bool isRuleCompliant(const PolicyRule& rule) const;

    /// Expose the list of rule evaluators (for external iteration/reporting).
    const std::vector<std::shared_ptr<IComplianceRule>>& rules() const {
        return rules_;
    }

private:
    std::vector<std::shared_ptr<IComplianceRule>> rules_;
};

} // namespace governance
} // namespace themis
