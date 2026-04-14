/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            pci_dss_rules.cpp                                  ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-04-13 20:31:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     207                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • f07ba6be34  2026-02-25  feat(governance): implement PCI-DSS data isolation rules ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "governance/pci_dss_rules.h"
#include "utils/logger.h"

namespace themis {
namespace governance {

// ============================================================================
// PciDssRuleEvalResult
// ============================================================================

nlohmann::json PciDssRuleEvalResult::toJson() const {
    return {
        {"rule_id",        rule_id},
        {"pci_dss_check_id", pci_dss_check_id},
        {"requirement",    requirement},
        {"compliant",      compliant},
        {"description",    description},
        {"recommendation", recommendation}
    };
}

// ============================================================================
// CardholderDataIsolation – PCI-DSS Req 1
// ============================================================================

bool CardholderDataIsolation::evaluate(const PolicyRule& rule) const {
    if (!rule.enabled) {
        return true; // Disabled rules are not in force; exempt from active checks.
    }
    // A rule satisfies isolation when either:
    //   1. required_roles is non-empty (access is restricted to specific roles), OR
    //   2. require_encryption is true (unauthenticated plaintext access is blocked).
    return !rule.required_roles.empty() || rule.require_encryption;
}

// ============================================================================
// CardholderDataEncryption – PCI-DSS Req 3
// ============================================================================

bool CardholderDataEncryption::evaluate(const PolicyRule& rule) const {
    if (!rule.enabled) {
        return true;
    }
    // Stored cardholder data must be encrypted at rest (require_encryption=true).
    return rule.require_encryption;
}

// ============================================================================
// TransmissionEncryption – PCI-DSS Req 4
// ============================================================================

bool TransmissionEncryption::evaluate(const PolicyRule& rule) const {
    if (!rule.enabled) {
        return true;
    }
    // Transmission is safe when either:
    //   1. Export is disabled altogether (no data leaves the system), OR
    //   2. Encryption is required (data can only be transmitted encrypted).
    // The violation pattern is: allow_export=true AND require_encryption=false.
    return !rule.allow_export || rule.require_encryption;
}

// ============================================================================
// AccessControlLeastPrivilege – PCI-DSS Req 7
// ============================================================================

bool AccessControlLeastPrivilege::evaluate(const PolicyRule& rule) const {
    if (!rule.enabled) {
        return true;
    }
    // Least-privilege is satisfied when at least one role is explicitly required,
    // ensuring that access is restricted to known, authorised callers.
    return !rule.required_roles.empty();
}

// ============================================================================
// CardholderDataAuditTrail – PCI-DSS Req 10
// ============================================================================

bool CardholderDataAuditTrail::evaluate(const PolicyRule& rule) const {
    if (!rule.enabled) {
        return true;
    }
    // PCI-DSS Req 10 requires:
    //   - audit_access=true  (read access events logged)
    //   - audit_changes=true (write/modify events logged)
    //   - retention_days >= 365 (12-month log availability; Req 10.7)
    const int kPciDssMinRetentionDays = 365;
    return rule.audit_access &&
           rule.audit_changes &&
           rule.retention_days >= kPciDssMinRetentionDays;
}

// ============================================================================
// PciDssRuleSet
// ============================================================================

PciDssRuleSet::PciDssRuleSet() {
    rules_.push_back(std::make_shared<CardholderDataIsolation>());
    rules_.push_back(std::make_shared<CardholderDataEncryption>());
    rules_.push_back(std::make_shared<TransmissionEncryption>());
    rules_.push_back(std::make_shared<AccessControlLeastPrivilege>());
    rules_.push_back(std::make_shared<CardholderDataAuditTrail>());

    THEMIS_DEBUG("PciDssRuleSet initialized with {} rule evaluators", rules_.size());
}

std::vector<PciDssRuleEvalResult> PciDssRuleSet::evaluateRule(const PolicyRule& rule) const {
    std::vector<PciDssRuleEvalResult> results;
    results.reserve(rules_.size());

    for (const auto& pci_rule : rules_) {
        PciDssRuleEvalResult res;
        res.rule_id          = rule.id;
        res.pci_dss_check_id = pci_rule->id();
        res.requirement      = pci_rule->id(); // same as id for display
        res.compliant        = pci_rule->evaluate(rule);

        if (res.compliant) {
            res.description = "Rule '" + rule.name + "' satisfies " + pci_rule->id();
        } else {
            res.description    = "Rule '" + rule.name + "' does not satisfy " + pci_rule->id();
            res.recommendation = pci_rule->description();
        }
        results.push_back(std::move(res));
    }

    return results;
}

bool PciDssRuleSet::isRuleCompliant(const PolicyRule& rule) const {
    for (const auto& pci_rule : rules_) {
        if (!pci_rule->evaluate(rule)) {
            return false;
        }
    }
    return true;
}

std::vector<std::string> PciDssRuleSet::detectGdprConflicts(const PolicyRule& rule) const {
    std::vector<std::string> conflicts;

    if (!rule.enabled) {
        return conflicts;
    }

    // PCI-DSS Req 10.7 mandates that audit logs are retained for at least 12
    // months (365 days).  GDPR Article 5(1)(e) requires data to be kept "no
    // longer than is necessary", which can conflict with the PCI-DSS minimum
    // retention floor when retention_days is set below 365 to comply with GDPR
    // storage-limitation obligations.
    const int kPciDssMinRetentionDays = 365;
    if (rule.audit_access &&
        rule.retention_days > 0 &&
        rule.retention_days < kPciDssMinRetentionDays) {
        conflicts.push_back(
            "Rule '" + rule.id + "': retention_days=" + std::to_string(rule.retention_days) +
            " is below the PCI-DSS Req 10.7 minimum of " +
            std::to_string(kPciDssMinRetentionDays) +
            " days. GDPR storage-limitation (Art. 5(1)(e)) may justify a shorter "
            "retention for non-cardholder personal data, but PCI-DSS-scoped audit "
            "logs must be retained for at least 12 months."
        );
    }

    // PCI-DSS Req 3 mandates encryption of stored cardholder data.
    // GDPR Article 25 (data protection by design) supports this requirement,
    // so there is no direct conflict — but a rule that enables export without
    // encryption violates both frameworks simultaneously: flag this overlap so
    // the operator can address both compliance obligations in a single fix.
    if (rule.allow_export && !rule.require_encryption) {
        conflicts.push_back(
            "Rule '" + rule.id + "': allow_export=true with require_encryption=false "
            "violates both PCI-DSS Req 4 (encrypted transmission) and GDPR Art. 32 "
            "(appropriate technical measures). Enable require_encryption to satisfy "
            "both frameworks simultaneously."
        );
    }

    return conflicts;
}

} // namespace governance
} // namespace themis
