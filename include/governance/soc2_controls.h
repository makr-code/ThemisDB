/**
 * @file soc2_controls.h
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
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>

namespace themis {
namespace governance {

// ============================================================================
// Evidence artifact
// ============================================================================

/// A single piece of compliance evidence for a SOC 2 audit.
/// Each evidence item links a specific control to a policy-rule decision
/// and captures the metadata an auditor needs to verify the control.
struct Soc2EvidenceItem {
    std::string evidence_id;     ///< Unique evidence identifier (uuid-like)
    std::string control_id;      ///< SOC 2 control this evidences (e.g., "CC6.1")
    std::string evidence_type;   ///< "policy_rule", "access_log", "encryption_status", "retention_policy"
    int64_t     timestamp_ms = 0;///< Collection time (Unix epoch milliseconds)
    std::string resource;        ///< Resource covered by this evidence
    std::string principal;       ///< Principal (user/role) involved, if applicable
    std::string action;          ///< Action performed, if applicable
    bool        control_met = false; ///< Whether the control was satisfied
    std::string detail;          ///< Human-readable evidence description
    nlohmann::json metadata;     ///< Additional structured metadata

    nlohmann::json toJson() const;
};

// ============================================================================
// Control evaluation result
// ============================================================================

/// Result of evaluating a single SOC 2 control against a PolicyRule.
struct Soc2ControlResult {
    std::string control_id;                      ///< SOC 2 control ID (e.g., "CC6.1")
    std::string criteria;                        ///< Trust Services Criteria category (e.g., "CC6")
    std::string title;                           ///< Short control title
    bool        compliant = false;               ///< Overall compliance status
    std::string description;                     ///< Evaluation summary
    std::string recommendation;                  ///< Remediation guidance when non-compliant
    std::vector<std::string> missing_controls;  ///< Specific control gaps
    std::vector<Soc2EvidenceItem> evidence;     ///< Evidence items supporting this result

    nlohmann::json toJson() const;
};

// ============================================================================
// SOC 2 audit report
// ============================================================================

/// Aggregated SOC 2 compliance report for a PolicyRule or a full PolicyManager.
struct Soc2AuditReport {
    std::string report_id;                         ///< Unique report identifier
    int64_t     generated_at_ms = 0;               ///< Report generation time
    std::string scope;                             ///< Audit scope description
    int         total_controls = 0;                ///< Number of controls evaluated
    int         controls_met = 0;                  ///< Number of controls satisfied
    double      compliance_score = 0.0;            ///< Score 0–100
    std::vector<Soc2ControlResult> results;       ///< Per-control results
    std::vector<Soc2EvidenceItem> evidence_items; ///< All collected evidence

    nlohmann::json toJson() const;
};

// ============================================================================
// ISoc2Control – base interface
// ============================================================================

/// Abstract base interface for SOC 2 Trust Services Criteria control evaluators.
/// Implementations evaluate whether a PolicyRule satisfies a specific SOC 2
/// control requirement and produce structured evidence for auditors.
class ISoc2Control {
public:
    virtual ~ISoc2Control() = default;

    /// Short control identifier (e.g., "CC6.1")
    [[nodiscard]] virtual std::string id() const = 0;

    /// Trust Services Criteria category (e.g., "CC6", "A1", "C1")
    [[nodiscard]] virtual std::string criteria() const = 0;

    /// One-line title (e.g., "Logical Access Security – Encryption")
    [[nodiscard]] virtual std::string title() const = 0;

    /// Detailed description of what this control verifies
    [[nodiscard]] virtual std::string description() const = 0;

    /// Evaluate the control against a single PolicyRule.
    /// Populates the returned Soc2ControlResult with compliance status and evidence.
    [[nodiscard]] virtual Soc2ControlResult evaluate(const PolicyRule& rule) const = 0;
};

// ============================================================================
// Concrete control evaluators
// ============================================================================

/// CC6.1 – Logical Access Security: field-level encryption requirement.
/// Verifies that rules protecting sensitive resources require encryption so
/// that designated confidential data is protected at rest (SOC 2 CC6.1).
class Soc2Cc6Control final : public ISoc2Control {
public:
    std::string id()          const override { return "CC6.1"; }
    std::string criteria()    const override { return "CC6"; }
    std::string title()       const override {
        return "Logical Access Controls – Field-Level Encryption";
    }
    std::string description() const override {
        return "SOC 2 CC6.1: The entity implements logical access security "
               "measures to protect against unauthorized access. Rules covering "
               "classified or sensitive resources must enforce encryption "
               "(require_encryption=true) and restrict role-based access.";
    }
    Soc2ControlResult evaluate(const PolicyRule& rule) const override;
};

/// CC7.2 – System Operations: detection of unauthorized changes.
/// Verifies that rules enable change auditing so that unauthorized or
/// unexpected modifications to protected resources are detectable (SOC 2 CC7.2).
class Soc2Cc7Control final : public ISoc2Control {
public:
    std::string id()          const override { return "CC7.2"; }
    std::string criteria()    const override { return "CC7"; }
    std::string title()       const override {
        return "System Operations – Change Detection and Audit Logging";
    }
    std::string description() const override {
        return "SOC 2 CC7.2: The entity monitors system components and data "
               "for unauthorized changes. Rules must enable audit_access and "
               "audit_changes to ensure all access and modifications are logged.";
    }
    Soc2ControlResult evaluate(const PolicyRule& rule) const override;
};

/// CC8.1 – Change Management: authorized change procedures.
/// Verifies that rules governing critical resources enforce change-management
/// controls (signature requirement, change auditing) (SOC 2 CC8.1).
class Soc2Cc8Control final : public ISoc2Control {
public:
    std::string id()          const override { return "CC8.1"; }
    std::string criteria()    const override { return "CC8"; }
    std::string title()       const override {
        return "Change Management – Authorized Change Procedures";
    }
    std::string description() const override {
        return "SOC 2 CC8.1: The entity authorizes, designs, develops, and "
               "implements changes to infrastructure, data, software, and "
               "procedures. Rules must require signatures (require_signature=true) "
               "and audit changes (audit_changes=true) for critical resources.";
    }
    Soc2ControlResult evaluate(const PolicyRule& rule) const override;
};

/// A1.1 – Availability: data retention and recovery commitments.
/// Verifies that rules define a finite retention period consistent with
/// availability commitments (SOC 2 A1.1).
class Soc2A1Control final : public ISoc2Control {
public:
    std::string id()          const override { return "A1.1"; }
    std::string criteria()    const override { return "A1"; }
    std::string title()       const override {
        return "Availability – Data Retention and Recovery";
    }
    std::string description() const override {
        return "SOC 2 A1.1: The entity maintains, monitors, and evaluates "
               "current processing capacity and use of system components to "
               "manage capacity demand. Rules must define a finite, positive "
               "retention period (retention_days > 0) to honour availability "
               "commitments.";
    }
    Soc2ControlResult evaluate(const PolicyRule& rule) const override;
};

/// C1.1 – Confidentiality: identification and protection of confidential data.
/// Verifies that rules covering confidential resources enforce strict data
/// controls: encryption, no unrestricted export, appropriate redaction
/// (SOC 2 C1.1).
class Soc2C1Control final : public ISoc2Control {
public:
    std::string id()          const override { return "C1.1"; }
    std::string criteria()    const override { return "C1"; }
    std::string title()       const override {
        return "Confidentiality – Data Classification and Protection";
    }
    std::string description() const override {
        return "SOC 2 C1.1: The entity identifies and maintains confidential "
               "information to meet the entity's objectives. Rules with "
               "classification levels above 'offen' (vs-nfd, geheim, "
               "streng-geheim) must enforce encryption, restrict export, "
               "and apply a non-trivial redaction level.";
    }
    Soc2ControlResult evaluate(const PolicyRule& rule) const override;
};

/// PI1.2 – Processing Integrity: completeness and accuracy of processing.
/// Verifies that rules ensure data processing is complete, valid, accurate,
/// and timely through audit logging (SOC 2 PI1.2).
class Soc2Pi1Control final : public ISoc2Control {
public:
    std::string id()          const override { return "PI1.2"; }
    std::string criteria()    const override { return "PI1"; }
    std::string title()       const override {
        return "Processing Integrity – Audit Trail Completeness";
    }
    std::string description() const override {
        return "SOC 2 PI1.2: System processing is complete, valid, accurate, "
               "timely, and authorized. Rules must enable audit_access so that "
               "every access event is captured, enabling verification that "
               "processing is complete and tamper-evident.";
    }
    Soc2ControlResult evaluate(const PolicyRule& rule) const override;
};

// ============================================================================
// Soc2ControlSet
// ============================================================================

/// Aggregates all SOC 2 Trust Services Criteria control evaluators and provides:
///   1. Per-PolicyRule compliance evaluation against the full SOC 2 control set.
///   2. Evidence collection integrated with policy decisions.
///   3. Full SOC 2 audit report generation.
class Soc2ControlSet {
public:
    Soc2ControlSet();

    // ---- Rule evaluation -------------------------------------------------

    /// Evaluate all SOC 2 controls against a single PolicyRule.
    /// @return A list of evaluation results, one per SOC 2 control.
    std::vector<Soc2ControlResult> evaluateRule(const PolicyRule& rule) const;

    /// Return true only if every SOC 2 control passes for the given rule.
    bool isRuleCompliant(const PolicyRule& rule) const;

    /// Evaluate all rules in a PolicyManager and produce a full audit report.
    /// Uses PolicyManager::listRules() to iterate over all registered rules.
    /// @param policy_mgr   Source of PolicyRules to evaluate.
    /// @param scope        Human-readable scope description for the report.
    Soc2AuditReport generateReport(
        const PolicyManager& policy_mgr,
        const std::string& scope = "All active policy rules"
    ) const;

    // ---- Evidence collection ---------------------------------------------

    /// Collect evidence for a single policy decision (called at query time).
    /// Records an Soc2EvidenceItem for the most relevant control.
    /// Thread-safe; may be called from any thread.
    void collectEvidence(
        const std::string& resource,
        const std::string& action,
        const std::string& principal,
        bool access_granted,
        bool encrypted
    );

    /// Return all evidence items collected since the last reset.
    std::vector<Soc2EvidenceItem> getEvidence() const;

    /// Clear all collected evidence items.
    void clearEvidence();

    /// Expose the list of control evaluators (for external iteration/reporting)
    const std::vector<std::shared_ptr<ISoc2Control>>& controls() const {
        return controls_;
    }

private:
    std::vector<std::shared_ptr<ISoc2Control>> controls_;

    mutable std::mutex evidence_mutex_;
    std::vector<Soc2EvidenceItem> evidence_items_;
    int64_t evidence_counter_ = 0; ///< Used to generate unique evidence IDs
};

} // namespace governance
} // namespace themis
