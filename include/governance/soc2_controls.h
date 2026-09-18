/**
 * @file soc2_controls.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
};

// ============================================================================
// Control evaluation result
// ============================================================================

struct Soc2ControlResult {
    std::string control_id;                      ///< SOC 2 control ID (e.g., "CC6.1")
    std::string criteria;                        ///< Trust Services Criteria category (e.g., "CC6")
    std::string title;                           ///< Short control title
    bool        compliant = false;               ///< Overall compliance status
    std::string description;                     ///< Evaluation summary
    std::string recommendation;                  ///< Remediation guidance when non-compliant
    std::vector<std::string> missing_controls;  ///< Specific control gaps
    std::vector<Soc2EvidenceItem> evidence;     ///< Evidence items supporting this result

    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
};

// ============================================================================
// SOC 2 audit report
// ============================================================================

struct Soc2AuditReport {
    std::string report_id;                         ///< Unique report identifier
    int64_t     generated_at_ms = 0;               ///< Report generation time
    std::string scope;                             ///< Audit scope description
    int         total_controls = 0;                ///< Number of controls evaluated
    int         controls_met = 0;                  ///< Number of controls satisfied
    double      compliance_score = 0.0;            ///< Score 0–100
    std::vector<Soc2ControlResult> results;       ///< Per-control results
    std::vector<Soc2EvidenceItem> evidence_items; ///< All collected evidence

    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
};

// ============================================================================
// ISoc2Control – base interface
// ============================================================================

class ISoc2Control {
public:
    /**
     * @brief ISoc2 Control.
     * @return Return value.
     */
    virtual ~ISoc2Control() = default;

    [[nodiscard]] virtual std::string id() const = 0;

    [[nodiscard]] virtual std::string criteria() const = 0;

    [[nodiscard]] virtual std::string title() const = 0;

    [[nodiscard]] virtual std::string description() const = 0;

    [[nodiscard]] virtual Soc2ControlResult evaluate(const PolicyRule& rule) const = 0;
};

// ============================================================================
// Concrete control evaluators
// ============================================================================

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

class Soc2ControlSet {
public:
    Soc2ControlSet();

    /**
     * @brief ---- Rule evaluation -------------------------------------------------
     * @param[in] rule Input parameter.
     * @return Return value.
     */

    std::vector<Soc2ControlResult> evaluateRule(const PolicyRule& rule) const;

    /**
     * @brief Is Rule Compliant.
     * @param[in] rule Input parameter.
     * @return True when the operation succeeds.
     */
    bool isRuleCompliant(const PolicyRule& rule) const;

    Soc2AuditReport generateReport(
        const PolicyManager& policy_mgr,
        const std::string& scope = "All active policy rules"
    ) const;

    /**
     * @brief ---- Evidence collection ---------------------------------------------
     * @param[in] resource Input parameter.
     * @param[in] action Input parameter.
     * @param[in] principal Input parameter.
     * @param[in] access_granted Input parameter.
     * @param[in] encrypted Input parameter.
     */

    void collectEvidence(
        const std::string& resource,
        const std::string& action,
        const std::string& principal,
        bool access_granted,
        bool encrypted
    );

    /**
     * @brief Get Evidence.
     * @return Return value.
     */
    std::vector<Soc2EvidenceItem> getEvidence() const;

    /**
     * @brief Clear Evidence.
     */
    void clearEvidence();

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
