/**
 * @file pci_dss_rules.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
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
#include <unordered_set>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>

namespace themis {
namespace governance {

// ============================================================================
// Result types
// ============================================================================

struct PciDssRuleEvalResult {
    std::string rule_id;          ///< ID of the evaluated PolicyRule
    std::string pci_dss_check_id; ///< ID of the PCI-DSS rule that was evaluated
    std::string requirement;      ///< PCI-DSS requirement number (e.g., "req_3")
    bool compliant = false;       ///< Whether the rule is PCI-DSS compliant
    std::string description;      ///< Human-readable result description
    std::string recommendation;   ///< Remediation recommendation if not compliant

    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
};

// ============================================================================
// PCI-DSS Concrete Rule Evaluators
// ============================================================================

class CardholderDataIsolation final : public IComplianceRule {
public:
    std::string id() const override { return "pci_dss_req_1_isolation"; }
    std::string framework() const override { return "PCI-DSS"; }
    std::string description() const override {
        return "PCI-DSS Req 1: Network controls and data isolation. Rules covering "
               "cardholder data resources must restrict access via required_roles "
               "(not open to anonymous or unconstrained callers) and must not allow "
               "unrestricted wildcard resource matching without encryption.";
    }
    bool evaluate(const PolicyRule& rule) const override;
};

class CardholderDataEncryption final : public IComplianceRule {
public:
    std::string id() const override { return "pci_dss_req_3_encryption"; }
    std::string framework() const override { return "PCI-DSS"; }
    std::string description() const override {
        return "PCI-DSS Req 3: Protect stored account data. Rules covering stored "
               "cardholder data must enable require_encryption=true so that Primary "
               "Account Numbers (PANs) and sensitive authentication data are rendered "
               "unreadable in storage.";
    }
    bool evaluate(const PolicyRule& rule) const override;
};

class TransmissionEncryption final : public IComplianceRule {
public:
    std::string id() const override { return "pci_dss_req_4_transmission"; }
    std::string framework() const override { return "PCI-DSS"; }
    std::string description() const override {
        return "PCI-DSS Req 4: Protect cardholder data in transit. Rules that permit "
               "export of cardholder data must require encryption (require_encryption=true) "
               "so that data is only transmitted over encrypted channels. Rules with "
               "allow_export=true and require_encryption=false are non-compliant.";
    }
    bool evaluate(const PolicyRule& rule) const override;
};

class AccessControlLeastPrivilege final : public IComplianceRule {
public:
    std::string id() const override { return "pci_dss_req_7_least_privilege"; }
    std::string framework() const override { return "PCI-DSS"; }
    std::string description() const override {
        return "PCI-DSS Req 7: Restrict access to system components and cardholder "
               "data by business need. Rules must define at least one required_role "
               "to enforce least-privilege access; rules with an empty required_roles "
               "list grant unrestricted access and are non-compliant.";
    }
    bool evaluate(const PolicyRule& rule) const override;
};

class CardholderDataAuditTrail final : public IComplianceRule {
public:
    std::string id() const override { return "pci_dss_req_10_audit"; }
    std::string framework() const override { return "PCI-DSS"; }
    std::string description() const override {
        return "PCI-DSS Req 10: Log and monitor all access to cardholder data. "
               "Rules must enable audit_access=true AND audit_changes=true. "
               "Additionally, retention_days must be >= 365 (PCI-DSS Req 10.7 "
               "mandates 12 months of audit log availability).";
    }
    bool evaluate(const PolicyRule& rule) const override;
};

// ============================================================================
// PciDssRuleSet
// ============================================================================

class PciDssRuleSet {
public:
    PciDssRuleSet();

    /**
     * @brief ---- Rule evaluation ------------------------------------------------
     * @param[in] rule Input parameter.
     * @return Return value.
     */

    std::vector<PciDssRuleEvalResult> evaluateRule(const PolicyRule& rule) const;

    /**
     * @brief Is Rule Compliant.
     * @param[in] rule Input parameter.
     * @return True when the operation succeeds.
     */
    bool isRuleCompliant(const PolicyRule& rule) const;

    /**
     * @brief Detect Gdpr Conflicts.
     * @param[in] rule Input parameter.
     * @return Return value.
     */
    std::vector<std::string> detectGdprConflicts(const PolicyRule& rule) const;

    const std::vector<std::shared_ptr<IComplianceRule>>& rules() const {
        return rules_;
    }

private:
    std::vector<std::shared_ptr<IComplianceRule>> rules_;
};

} // namespace governance
} // namespace themis
