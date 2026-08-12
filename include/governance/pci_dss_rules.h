/**
 * @file pci_dss_rules.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
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
#include <unordered_set>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>

namespace themis {
namespace governance {

// ============================================================================
// Result types
// ============================================================================

/// Result of a PCI-DSS compliance rule evaluation for a single PolicyRule.
struct PciDssRuleEvalResult {
    std::string rule_id;          ///< ID of the evaluated PolicyRule
    std::string pci_dss_check_id; ///< ID of the PCI-DSS rule that was evaluated
    std::string requirement;      ///< PCI-DSS requirement number (e.g., "req_3")
    bool compliant = false;       ///< Whether the rule is PCI-DSS compliant
    std::string description;      ///< Human-readable result description
    std::string recommendation;   ///< Remediation recommendation if not compliant

    nlohmann::json toJson() const;
};

// ============================================================================
// PCI-DSS Concrete Rule Evaluators
// ============================================================================

/// PCI-DSS Requirement 1: Install and maintain network controls.
/// Verifies that a PolicyRule scopes access to cardholder data resources
/// by requiring at least one role restriction (not open to all callers).
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
    /// A PolicyRule satisfies isolation when:
    ///   - It requires at least one role (required_roles is non-empty), OR
    ///   - Encryption is required (blocking unauthenticated access paths).
    bool evaluate(const PolicyRule& rule) const override;
};

/// PCI-DSS Requirement 3: Protect stored account data.
/// Verifies that a PolicyRule enforces encryption at rest for all
/// cardholder data resources.
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
    /// A PolicyRule satisfies stored-data protection when require_encryption=true.
    bool evaluate(const PolicyRule& rule) const override;
};

/// PCI-DSS Requirement 4: Protect cardholder data with strong cryptography
/// during transmission over open, public networks.
/// Verifies that a PolicyRule prevents unencrypted export of cardholder data.
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
    /// A PolicyRule satisfies transmission security when either:
    ///   - Export is disabled (allow_export=false), OR
    ///   - Encryption is required (require_encryption=true).
    bool evaluate(const PolicyRule& rule) const override;
};

/// PCI-DSS Requirement 7: Restrict access to system components and cardholder
/// data by business need to know (least privilege).
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
    /// A PolicyRule satisfies least-privilege when required_roles is non-empty,
    /// ensuring that access is restricted to identified roles only.
    bool evaluate(const PolicyRule& rule) const override;
};

/// PCI-DSS Requirement 10: Log and monitor all access to network resources
/// and cardholder data.
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
    /// A PolicyRule satisfies the audit requirement when:
    ///   - audit_access=true   (access events are recorded), AND
    ///   - audit_changes=true  (modification events are recorded), AND
    ///   - retention_days >= 365 (logs available for at least 12 months).
    bool evaluate(const PolicyRule& rule) const override;
};

// ============================================================================
// PciDssRuleSet
// ============================================================================

/// Aggregates all PCI-DSS compliance rule evaluators and provides:
///   1. Per-rule compliance evaluation against the full PCI-DSS rule set.
///   2. Summary compliance status over a PolicyManager's rule set.
///   3. PCI-DSS / GDPR conflict detection helpers.
class PciDssRuleSet {
public:
    PciDssRuleSet();

    // ---- Rule evaluation ------------------------------------------------

    /// Evaluate all PCI-DSS rules against a single PolicyRule.
    /// @return A list of evaluation results, one per PCI-DSS rule.
    std::vector<PciDssRuleEvalResult> evaluateRule(const PolicyRule& rule) const;

    /// Return true if the PolicyRule satisfies all PCI-DSS checks.
    bool isRuleCompliant(const PolicyRule& rule) const;

    /// Detect PCI-DSS / GDPR conflicts for a single rule.
    ///
    /// PCI-DSS Req 10.7 requires 12 months of audit log retention.
    /// GDPR Article 5(1)(e) (storage limitation) requires minimising
    /// retention periods, which can conflict with the PCI-DSS minimum.
    ///
    /// @return List of conflict descriptions (empty == no conflicts).
    std::vector<std::string> detectGdprConflicts(const PolicyRule& rule) const;

    /// Expose the list of rule evaluators (for external iteration/reporting).
    const std::vector<std::shared_ptr<IComplianceRule>>& rules() const {
        return rules_;
    }

private:
    std::vector<std::shared_ptr<IComplianceRule>> rules_;
};

} // namespace governance
} // namespace themis
