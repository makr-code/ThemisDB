/**
 * @file ccpa_rules.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "governance/policy_manager.h"
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>

namespace themis {
namespace governance {

/// Abstract base interface for compliance rule evaluators.
/// Implementations evaluate whether a PolicyRule satisfies a specific
/// regulatory requirement (CCPA, GDPR, HIPAA, etc.).
class IComplianceRule {
public:
    virtual ~IComplianceRule() = default;

    /// Unique identifier for this rule (e.g., "ccpa_right_to_know")
    [[nodiscard]] virtual std::string id() const = 0;

    /// Regulatory framework this rule belongs to (e.g., "CCPA")
    [[nodiscard]] virtual std::string framework() const = 0;

    /// Human-readable description of what this rule checks
    [[nodiscard]] virtual std::string description() const = 0;

    /// Evaluate whether the given PolicyRule satisfies this compliance rule.
    /// @return true if the rule is compliant, false otherwise
    [[nodiscard]] virtual bool evaluate(const PolicyRule& rule) const = 0;
};

/// Result of a CCPA compliance rule evaluation for a single PolicyRule.
struct CcpaRuleEvalResult {
    std::string rule_id;          ///< ID of the evaluated PolicyRule
    std::string ccpa_check_id;    ///< ID of the CCPA rule that was evaluated
    bool compliant = false;       ///< Whether the rule is CCPA-compliant
    std::string description;      ///< Human-readable result description
    std::string recommendation;   ///< Remediation recommendation if not compliant

    nlohmann::json toJson() const;
};

/// Context passed when evaluating CCPA compliance at query time.
struct CcpaQueryContext {
    std::string subject_id;                    ///< Data subject / user ID
    bool opted_out_of_sale = false;            ///< Subject has opted out of data sale
    std::vector<std::string> data_categories;  ///< Categories of personal data involved
    std::string action;                        ///< Query action: "read", "write", "export"
};

/// Represents a data subject rights request (right-to-know, right-to-delete, etc.)
struct DataSubjectRequest {
    std::string request_id;
    std::string subject_id;
    std::string request_type;  ///< "right_to_know" | "right_to_delete" | "opt_out_of_sale" | "data_portability"
    int64_t timestamp = 0;
    std::string status;        ///< "pending" | "fulfilled" | "denied"
    std::string denial_reason; ///< Populated when status == "denied"

    nlohmann::json toJson() const;
};

// ============================================================================
// CCPA Concrete Rule Evaluators
// ============================================================================

/// CCPA Right to Know: verifies that a PolicyRule enables audit access so
/// consumers can discover what personal data is collected about them.
class RightToKnow final : public IComplianceRule {
public:
    std::string id() const override { return "ccpa_right_to_know"; }
    std::string framework() const override { return "CCPA"; }
    std::string description() const override {
        return "CCPA §1798.100: Consumer right to know what personal data is "
               "collected and how it is used. Requires audit_access=true on "
               "rules covering personal data resources.";
    }
    /// A PolicyRule satisfies right-to-know when audit access is enabled,
    /// allowing operators to produce the data inventory required by CCPA.
    bool evaluate(const PolicyRule& rule) const override;
};

/// CCPA Right to Delete: verifies that a PolicyRule does not prevent data
/// deletion, enabling consumers to request erasure of their personal data.
class RightToDelete final : public IComplianceRule {
public:
    std::string id() const override { return "ccpa_right_to_delete"; }
    std::string framework() const override { return "CCPA"; }
    std::string description() const override {
        return "CCPA §1798.105: Consumer right to delete personal data. "
               "Requires that the rule does not block deletion (retention_days "
               "must allow deletion upon request; rule must audit changes).";
    }
    /// A PolicyRule satisfies right-to-delete when it audits changes (so
    /// deletion can be tracked) and the retention period is finite (not
    /// indefinite – indicated by retention_days > 0 and not exceeding 3650 days
    /// without special justification).
    bool evaluate(const PolicyRule& rule) const override;
};

/// CCPA Opt-Out of Sale: verifies that a PolicyRule does not allow unrestricted
/// data export for rules covering personal data in contexts where sale/sharing
/// with third parties must be opt-in.
class OptOutOfSale final : public IComplianceRule {
public:
    std::string id() const override { return "ccpa_opt_out_of_sale"; }
    std::string framework() const override { return "CCPA"; }
    std::string description() const override {
        return "CCPA §1798.120: Consumer right to opt out of sale of personal "
               "data. Rules with allow_export=true on personal data resources "
               "must enforce opt-out preference before sharing with third parties.";
    }
    /// A PolicyRule satisfies opt-out-of-sale when export is disabled by
    /// default (allow_export=false) for classified/sensitive resources, or
    /// when the rule requires a signature (consent) before export.
    bool evaluate(const PolicyRule& rule) const override;
};

/// CCPA Data Portability: verifies that a PolicyRule provides a path for
/// consumers to receive a portable copy of their personal data.
class DataPortability final : public IComplianceRule {
public:
    std::string id() const override { return "ccpa_data_portability"; }
    std::string framework() const override { return "CCPA"; }
    std::string description() const override {
        return "CCPA §1798.100(d): Consumer right to receive personal data in "
               "a portable, machine-readable format. A rule satisfies this "
               "requirement when allow_export=true (automated export) OR "
               "audit_access=true (data is discoverable for manual fulfilment). "
               "A rule with both flags false leaves no path to honour a portability "
               "request and is non-compliant.";
    }
    /// A PolicyRule satisfies data portability when export is allowed OR
    /// audit access enables manual discovery and fulfilment of the request.
    bool evaluate(const PolicyRule& rule) const override;
};

// ============================================================================
// CcpaRuleSet
// ============================================================================

/// Aggregates all CCPA/CPRA compliance rule evaluators and provides:
///   1. Per-rule compliance evaluation against the full CCPA rule set.
///   2. Query-time opt-out check via an in-memory subject registry.
///   3. CCPA/HIPAA conflict detection helpers used by PolicyValidator.
class CcpaRuleSet {
public:
    CcpaRuleSet();

    // ---- Subject opt-out registry ----------------------------------------

    /// Register a data subject as having opted out of data sale.
    /// Thread-safe; may be called from any thread.
    void addOptOut(const std::string& subject_id);

    /// Remove a data subject from the opt-out registry (opt back in).
    void removeOptOut(const std::string& subject_id);

    /// Return true if the given subject has opted out of data sale.
    bool isOptedOut(const std::string& subject_id) const;

    /// Replace the entire opt-out registry with a new set.
    void setOptOutRegistry(const std::unordered_set<std::string>& subjects);

    /// Return the current number of opted-out subjects.
    size_t optOutCount() const;

    // ---- Rule evaluation -------------------------------------------------

    /// Evaluate all CCPA rules against a single PolicyRule.
    /// @return A list of evaluation results, one per CCPA rule.
    std::vector<CcpaRuleEvalResult> evaluateRule(const PolicyRule& rule) const;

    /// Return true if the PolicyRule is fully CCPA-compliant (all checks pass).
    bool isRuleCompliant(const PolicyRule& rule) const;

    /// Check whether the given rule has CCPA/HIPAA conflicts.
    /// HIPAA mandates disclosure (audit_access) which may conflict with CCPA
    /// right-to-delete if the rule simultaneously prohibits audit_changes.
    /// @return List of conflict descriptions (empty == no conflicts).
    std::vector<std::string> detectHipaaConflicts(const PolicyRule& rule) const;

    // ---- Data subject requests -------------------------------------------

    /// Record a data subject rights request (for audit/reporting purposes).
    void recordRequest(const DataSubjectRequest& request);

    /// Retrieve all recorded requests for a given subject.
    std::vector<DataSubjectRequest> getRequestsForSubject(const std::string& subject_id) const;

    /// Retrieve all requests of a given type within a time window.
    std::vector<DataSubjectRequest> getRequestsByType(
        const std::string& request_type,
        int64_t start_time = 0,
        int64_t end_time = INT64_MAX
    ) const;

    /// Return the total number of opt-out requests recorded.
    int countOptOutRequests(int64_t start_time = 0, int64_t end_time = INT64_MAX) const;

    /// Expose the list of rule evaluators (for external iteration/reporting)
    const std::vector<std::shared_ptr<IComplianceRule>>& rules() const { return rules_; }

private:
    std::vector<std::shared_ptr<IComplianceRule>> rules_;

    mutable std::mutex opt_out_mutex_;
    std::unordered_set<std::string> opt_out_subjects_;

    mutable std::mutex requests_mutex_;
    std::vector<DataSubjectRequest> requests_;
};

} // namespace governance
} // namespace themis
