/**
 * @file hipaa_rules.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

struct HipaaRuleEvalResult {
    std::string rule_id;           ///< ID of the evaluated PolicyRule
    std::string hipaa_check_id;    ///< ID of the HIPAA rule that was evaluated
    std::string requirement;       ///< HIPAA requirement reference (e.g., "§164.312(a)(1)")
    bool compliant = false;        ///< Whether the rule is HIPAA compliant
    std::string description;       ///< Human-readable result description
    std::string recommendation;    ///< Remediation recommendation if not compliant

    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
};

// ============================================================================
// HIPAA Concrete Rule Evaluators
// ============================================================================

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
    bool evaluate(const PolicyRule& rule) const override;
};

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
    bool evaluate(const PolicyRule& rule) const override;
};

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
    bool evaluate(const PolicyRule& rule) const override;
};

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
    bool evaluate(const PolicyRule& rule) const override;
};

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
    bool evaluate(const PolicyRule& rule) const override;
};

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
    bool evaluate(const PolicyRule& rule) const override;
};

// ============================================================================
// HipaaRuleSet
// ============================================================================

class HipaaRuleSet {
public:
    HipaaRuleSet();


    /**
     * @brief Evaluate Rule.
     * @param[in] rule Input parameter.
     * @return Return value.
     */
    std::vector<HipaaRuleEvalResult> evaluateRule(const PolicyRule& rule) const;

    /**
     * @brief Is Rule Compliant.
     * @param[in] rule Input parameter.
     * @return True when the operation succeeds.
     */
    bool isRuleCompliant(const PolicyRule& rule) const;

    const std::vector<std::shared_ptr<IComplianceRule>>& rules() const {
        return rules_;
    }

private:
    std::vector<std::shared_ptr<IComplianceRule>> rules_;
};

} // namespace governance
} // namespace themis
