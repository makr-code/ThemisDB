/**
 * @file iso27001_rules.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
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

/// A single piece of compliance evidence for an ISO 27001 Annex A audit.
/// Each evidence item links a specific control to a policy-rule decision
/// and captures the metadata an auditor needs to verify the control.
struct Iso27001EvidenceItem {
    std::string evidence_id;      ///< Unique evidence identifier (uuid-like)
    std::string control_id;       ///< ISO 27001 control this evidences (e.g., "A.9.1.2")
    std::string evidence_type;    ///< "policy_rule", "access_log", "encryption_status", "retention_policy"
    int64_t     timestamp_ms = 0; ///< Collection time (Unix epoch milliseconds)
    std::string resource;         ///< Resource covered by this evidence
    std::string principal;        ///< Principal (user/role) involved, if applicable
    std::string action;           ///< Action performed, if applicable
    bool        control_met = false; ///< Whether the control was satisfied
    std::string detail;           ///< Human-readable evidence description
    nlohmann::json metadata;      ///< Additional structured metadata

    nlohmann::json toJson() const;
};

// ============================================================================
// Control evaluation result
// ============================================================================

/// Result of evaluating a single ISO 27001 Annex A control against a PolicyRule.
struct Iso27001ControlResult {
    std::string control_id;                      ///< ISO 27001 control ID (e.g., "A.9.1.2")
    std::string annex_section;                   ///< Annex A section (e.g., "A.9")
    std::string title;                           ///< Short control title
    bool        compliant = false;               ///< Overall compliance status
    std::string description;                     ///< Evaluation summary
    std::string recommendation;                  ///< Remediation guidance when non-compliant
    std::vector<std::string> missing_controls;   ///< Specific control gaps
    std::vector<Iso27001EvidenceItem> evidence;  ///< Evidence items supporting this result

    nlohmann::json toJson() const;
};

// ============================================================================
// ISO 27001 audit report
// ============================================================================

/// Aggregated ISO 27001 compliance report for a PolicyRule or a full PolicyManager.
struct Iso27001AuditReport {
    std::string report_id;                              ///< Unique report identifier
    int64_t     generated_at_ms = 0;                   ///< Report generation time
    std::string scope;                                  ///< Audit scope description
    int         total_controls = 0;                     ///< Number of controls evaluated
    int         controls_met = 0;                       ///< Number of controls satisfied
    double      compliance_score = 0.0;                 ///< Score 0–100
    std::vector<Iso27001ControlResult> results;         ///< Per-control results
    std::vector<Iso27001EvidenceItem> evidence_items;   ///< All collected evidence

    nlohmann::json toJson() const;
};

// ============================================================================
// IIso27001Control – base interface
// ============================================================================

/// Abstract base interface for ISO 27001 Annex A control evaluators.
/// Implementations evaluate whether a PolicyRule satisfies a specific ISO 27001
/// control requirement and produce structured evidence for auditors.
class IIso27001Control {
public:
    virtual ~IIso27001Control() = default;

    /// Short control identifier (e.g., "A.9.1.2")
    [[nodiscard]] virtual std::string id() const = 0;

    /// Annex A section (e.g., "A.9", "A.12")
    [[nodiscard]] virtual std::string annexSection() const = 0;

    /// One-line title (e.g., "Access Control Policy")
    [[nodiscard]] virtual std::string title() const = 0;

    /// Detailed description of what this control verifies
    [[nodiscard]] virtual std::string description() const = 0;

    /// Evaluate the control against a single PolicyRule.
    /// Populates the returned Iso27001ControlResult with compliance status and evidence.
    [[nodiscard]] virtual Iso27001ControlResult evaluate(const PolicyRule& rule) const = 0;
};

// ============================================================================
// Concrete control evaluators
// ============================================================================

/// A.9.1.2 – Access Control Policy: least-privilege access control.
/// Verifies that rules restrict access via required_roles, enforcing the
/// principle of least privilege as required by ISO 27001 Annex A control A.9.1.2.
class Iso27001A912Control final : public IIso27001Control {
public:
    std::string id()            const override { return "A.9.1.2"; }
    std::string annexSection()  const override { return "A.9"; }
    std::string title()         const override {
        return "Access Control Policy – Least Privilege";
    }
    std::string description()   const override {
        return "ISO 27001 A.9.1.2: An access control policy shall be established, "
               "documented, and reviewed based on business and information security "
               "requirements. Rules must define at least one required_role to enforce "
               "least-privilege access and prevent unauthorised access to information.";
    }
    Iso27001ControlResult evaluate(const PolicyRule& rule) const override;
};

/// A.10.1.1 – Cryptography Policy: use of cryptographic controls.
/// Verifies that rules protecting sensitive or classified resources require
/// encryption, as mandated by ISO 27001 Annex A control A.10.1.1.
class Iso27001A1011Control final : public IIso27001Control {
public:
    std::string id()            const override { return "A.10.1.1"; }
    std::string annexSection()  const override { return "A.10"; }
    std::string title()         const override {
        return "Cryptography Policy – Encryption of Sensitive Resources";
    }
    std::string description()   const override {
        return "ISO 27001 A.10.1.1: A policy on the use of cryptographic controls "
               "for protection of information shall be developed and implemented. "
               "Rules covering classified or sensitive resources must enforce "
               "require_encryption=true to protect data at rest and in transit.";
    }
    Iso27001ControlResult evaluate(const PolicyRule& rule) const override;
};

/// A.12.4.1 – Event Logging: logging of user activities and events.
/// Verifies that rules enable audit_access and audit_changes so that all
/// access and modification events are captured (ISO 27001 A.12.4.1).
class Iso27001A1241Control final : public IIso27001Control {
public:
    std::string id()            const override { return "A.12.4.1"; }
    std::string annexSection()  const override { return "A.12"; }
    std::string title()         const override {
        return "Event Logging – Access and Change Audit Trail";
    }
    std::string description()   const override {
        return "ISO 27001 A.12.4.1: Event logs recording user activities, exceptions, "
               "faults, and information security events shall be produced, kept, and "
               "regularly reviewed. Rules must enable audit_access=true and "
               "audit_changes=true to capture all access and modification events.";
    }
    Iso27001ControlResult evaluate(const PolicyRule& rule) const override;
};

/// A.12.4.2 – Protection of Log Information: audit log retention.
/// Verifies that retention_days >= 90 days to preserve audit log information
/// against tampering and accidental destruction (ISO 27001 A.12.4.2).
class Iso27001A1242Control final : public IIso27001Control {
public:
    std::string id()            const override { return "A.12.4.2"; }
    std::string annexSection()  const override { return "A.12"; }
    std::string title()         const override {
        return "Protection of Log Information – Retention Period";
    }
    std::string description()   const override {
        return "ISO 27001 A.12.4.2: Logging facilities and log information shall be "
               "protected against tampering and unauthorised access. Rules must define "
               "retention_days >= 90 to ensure that audit logs are preserved for a "
               "sufficient period for forensic investigation and compliance review.";
    }
    Iso27001ControlResult evaluate(const PolicyRule& rule) const override;
};

/// A.13.2.3 – Electronic Messaging: information transfer protection.
/// Verifies that export is either disabled or requires encryption before
/// data is transmitted (ISO 27001 A.13.2.3).
class Iso27001A1323Control final : public IIso27001Control {
public:
    std::string id()            const override { return "A.13.2.3"; }
    std::string annexSection()  const override { return "A.13"; }
    std::string title()         const override {
        return "Electronic Messaging – Transmission Security";
    }
    std::string description()   const override {
        return "ISO 27001 A.13.2.3: Information involved in electronic messaging shall "
               "be appropriately protected. Rules that permit export of information must "
               "require encryption (require_encryption=true) so that data is only "
               "transmitted over appropriately protected channels. Rules with "
               "allow_export=true and require_encryption=false are non-compliant.";
    }
    Iso27001ControlResult evaluate(const PolicyRule& rule) const override;
};

/// A.18.1.3 – Protection of Records: records retention and availability.
/// Verifies that retention_days > 0 to protect records against loss,
/// destruction, and falsification (ISO 27001 A.18.1.3).
class Iso27001A1813Control final : public IIso27001Control {
public:
    std::string id()            const override { return "A.18.1.3"; }
    std::string annexSection()  const override { return "A.18"; }
    std::string title()         const override {
        return "Protection of Records – Retention and Availability";
    }
    std::string description()   const override {
        return "ISO 27001 A.18.1.3: Records shall be protected from loss, destruction, "
               "falsification, unauthorised access, and unauthorised release. Rules must "
               "define a positive retention period (retention_days > 0) to ensure that "
               "governed information assets are retained according to legal and regulatory "
               "requirements.";
    }
    Iso27001ControlResult evaluate(const PolicyRule& rule) const override;
};

// ============================================================================
// Iso27001ControlSet
// ============================================================================

/// Aggregates all ISO 27001 Annex A control evaluators and provides:
///   1. Per-PolicyRule compliance evaluation against the full ISO 27001 control set.
///   2. Evidence collection integrated with policy decisions.
///   3. Full ISO 27001 audit report generation.
class Iso27001ControlSet {
public:
    Iso27001ControlSet();

    // ---- Rule evaluation -------------------------------------------------

    /// Evaluate all ISO 27001 controls against a single PolicyRule.
    /// @return A list of evaluation results, one per ISO 27001 control.
    std::vector<Iso27001ControlResult> evaluateRule(const PolicyRule& rule) const;

    /// Return true only if every ISO 27001 control passes for the given rule.
    bool isRuleCompliant(const PolicyRule& rule) const;

    /// Evaluate all rules in a PolicyManager and produce a full audit report.
    /// Uses PolicyManager::listRules() to iterate over all registered rules.
    /// @param policy_mgr   Source of PolicyRules to evaluate.
    /// @param scope        Human-readable scope description for the report.
    Iso27001AuditReport generateReport(
        const PolicyManager& policy_mgr,
        const std::string& scope = "All active policy rules"
    ) const;

    // ---- Evidence collection ---------------------------------------------

    /// Collect evidence for a single policy decision (called at query time).
    /// Records an Iso27001EvidenceItem for the most relevant control.
    /// Thread-safe; may be called from any thread.
    void collectEvidence(
        const std::string& resource,
        const std::string& action,
        const std::string& principal,
        bool access_granted,
        bool encrypted
    );

    /// Return all evidence items collected since the last reset.
    std::vector<Iso27001EvidenceItem> getEvidence() const;

    /// Clear all collected evidence items.
    void clearEvidence();

    /// Expose the list of control evaluators (for external iteration/reporting).
    const std::vector<std::shared_ptr<IIso27001Control>>& controls() const {
        return controls_;
    }

private:
    std::vector<std::shared_ptr<IIso27001Control>> controls_;

    mutable std::mutex evidence_mutex_;
    std::vector<Iso27001EvidenceItem> evidence_items_;
    int64_t evidence_counter_ = 0; ///< Used to generate unique evidence IDs
};

} // namespace governance
} // namespace themis
