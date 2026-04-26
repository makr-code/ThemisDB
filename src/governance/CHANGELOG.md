> ⚠️ **Historisches Changelog** – Einträge beschreiben den Stand zum Zeitpunkt der Erstellung.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Governance Module

All notable changes to the Governance module are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]
*(All planned features implemented — see `FUTURE_ENHANCEMENTS.md` for long-horizon research items.)*

## [1.9.0] — 2026-04-09
### Added
- `iso27001_rules.cpp`: ISO 27001 Annex A control evaluators — `Iso27001A912Control` (A.9.1.2 access control policy), `Iso27001A1011Control` (A.10.1.1 cryptography policy), `Iso27001A1241Control` (A.12.4.1 event logging), `Iso27001A1242Control` (A.12.4.2 protection of log information), `Iso27001A1323Control` (A.13.2.3 electronic messaging), `Iso27001A1813Control` (A.18.1.3 protection of records); `Iso27001ControlSet` with `evaluateRule()`, `isRuleCompliant()`, `generateReport()`, and evidence collection
- `hipaa_rules.cpp`: HIPAA Security Rule evaluators — `HipaaAccessControl` (§164.312(a)(1)), `HipaaEncryption` (§164.312(a)(2)(iv)), `HipaaAuditControls` (§164.312(b)), `HipaaIntegrityControls` (§164.312(c)(1)), `HipaaTransmissionSecurity` (§164.312(e)(2)(ii)), `HipaaRetention` (§164.530(j)); `HipaaRuleSet` with `evaluateRule()` and `isRuleCompliant()`
- `tests/test_iso27001_rules.cpp`: 19 focused unit tests for all 6 ISO 27001 controls plus `Iso27001ControlSet` aggregate evaluation
- `tests/test_hipaa_rules.cpp`: 19 focused unit tests for all 6 HIPAA rules plus `HipaaRuleSet` aggregate evaluation

## [1.8.0] — 2026-03-21
### Added
- Registered 9 previously unregistered governance test targets in `tests/CMakeLists.txt`: `CcpaRulesFocusedTests`, `CrossTenantPolicyInheritanceFocusedTests`, `DataLineageFocusedTests`, `DataMaskerFocusedTests`, `PciDssRulesFocusedTests`, `PolicyReviewFocusedTests`, `PolicyTemplateFocusedTests`, `PolicyVersioningFocusedTests`, `Soc2ControlsFocusedTests`
- Created dedicated CI workflow `governance-module-ci.yml` covering all 19 governance test suites across two jobs: unit tests and policy engine tests

## [1.7.0] — 2026-03-09
### Added
- `CrossTenantPolicyInheritance`: tenant hierarchy management with cycle detection, most-restrictive-wins merge semantics, `evaluateEffectivePolicy()` merging from full ancestor chain, `resolveEffectiveRules()` returning flattened rule list (Issue #1772)
- Unit tests for hierarchy registration, cycle prevention, merge semantics, and edge cases
- Compliance reporter for structured evidence collection (`src/governance/compliance_reporter.cpp`, `src/governance/compliance_reporting.cpp`) (Issue #1767)

## [1.6.0] — 2026-02-10
### Added
- AI/ML model governance: training data lineage, bias auditing (`src/governance/model_governance.cpp`) (Issue #1771)
- SOC 2 compliance controls and evidence collection (Issue #1769)
- PCI-DSS data isolation rules (`src/governance/pci_dss_rules.cpp`) (Issue #1770)
- Automated data masking for sensitive fields in query results: `DataMasker` (`src/governance/data_masker.cpp`) (Issue #1773)
- Policy conflict detection and resolution reporting: `PolicyManager::detectConflicts()` (Issue #1760)
- CCPA compliance rule set: `CcpaRuleSet` (`src/governance/ccpa_rules.cpp`) (Issue #1761)

## [1.5.0] — 2026-01-10
### Added
- OPA (Open Policy Agent) integration for policy-as-code evaluation (`src/governance/opa_adapter.cpp`) (Issue #1768)
- Policy simulation / dry-run mode: `PolicyEngine::simulateDecision()` (Issue #1766)
- CCPA/CPRA data subject rights enforcement: right-to-know, right-to-delete, opt-out-of-sale, data portability (Issue #1764)
- Data lineage tracking for governed datasets (`src/governance/data_lineage.cpp`) (Issue #1765)
- Policy hot-reload on configuration file change without service restart (Issue #1762)
- Policy conflict detection for overlapping access control rules (Issue #1763)

## [1.4.0] — 2025-12-01
### Added
- Policy versioning with rollback support (`src/governance/policy_manager_versioned.cpp`) (Issue #1780)
- Policy template library for common governance patterns (`src/governance/policy_template.cpp`)
- Policy review workflow (`src/governance/policy_review.cpp`)
- Policy file watcher for hot-reload trigger (`src/governance/policy_file_watcher.cpp`)
- Policy coordinator for multi-tenant policy distribution (`src/governance/policy_coordinator.cpp`)

## [1.0.0] — 2024-01-01
### Added
- Policy engine for attribute-based data access control (`src/governance/policy_engine.cpp`)
- GDPR and HIPAA compliance rule evaluation at query time
- Automated data retention policy enforcement with configurable TTLs
- Data classification and labeling for PII, PHI, and confidential fields
- Audit trail integration for all governance enforcement events
- Policy manager (`src/governance/policy_manager.cpp`)
