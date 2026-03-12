<!-- Status: current | validated: 2026-03-12 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Governance Module

All notable changes to the Governance module are documented here.  
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]
*(All planned features implemented — see `FUTURE_ENHANCEMENTS.md` for long-horizon research items.)*

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
