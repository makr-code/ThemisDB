<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ROADMAP.md · ../../src/governance/ -->

# Governance Module — Public Header Architecture
**Version:** 1.8.0
**Module Path:** `include/governance/`
**Implementation:** `../../src/governance/`

---

## Overview

The Governance module provides public headers for policy management, compliance reporting (GDPR, CCPA, PCI-DSS, SOC 2), data masking, data lineage tracking, model governance, and OPA (Open Policy Agent) integration.

## Design Principles

- **Policy Engine Core** — `policy_engine.h` is the central evaluator; all compliance rules plug in via `IComplianceRule`.
- **Multi-Framework Compliance** — CCPA, PCI-DSS, and SOC 2 rule sets ship as first-class headers.
- **Tenant Policy Hierarchy** — `cross_tenant_policy_inheritance.h` supports tenant hierarchy with most-restrictive-wins merge semantics and cycle detection.
- **Versioned Policies** — `policy_version_history.h` tracks all policy mutations with audit timestamps.

## Interface Inventory

| Header | Classes / Structs | Purpose |
|--------|-------------------|---------|
| `policy_engine.h` | `PolicyEngine` | Core policy evaluation engine |
| `policy_manager.h` | — | Policy lifecycle management (CRUD) |
| `policy_manager_versioned.h` | — | Versioned policy manager |
| `policy_version_history.h` | — | Policy change audit trail |
| `policy_validator.h` | — | Policy syntax and semantic validation |
| `policy_validation.h` | — | Validation result types |
| `policy_coordinator.h` | — | Multi-node policy coordination |
| `policy_review.h` | — | Policy review workflow |
| `review_scheduler.h` | — | Scheduled policy review reminders |
| `policy_template.h` | — | Reusable policy templates |
| `policy_file_watcher.h` | — | Hot-reload policy files on change |
| `compliance_reporter.h` | `ComplianceReporter`, `PolicyCoverageAnalyzer`, `ComplianceGapDetector` | Compliance report generation |
| `compliance_reporting.h` | `IComplianceReport`, `ComplianceReport`, `RuleEvaluationEntry`, `TimeWindowReport` | Compliance report types |
| `ccpa_rules.h` | `CcpaRuleSet`, `RightToKnow`, `RightToDelete`, `OptOutOfSale`, `DataPortability` | CCPA compliance rules |
| `pci_dss_rules.h` | — | PCI-DSS compliance rules |
| `soc2_controls.h` | — | SOC 2 Trust Service Criteria controls |
| `data_masker.h` | — | Field-level data masking |
| `data_lineage.h` | — | Data lineage tracking and graph |
| `model_governance.h` | — | ML model governance and audit |
| `opa_adapter.h` | — | Open Policy Agent integration |
| `cross_tenant_policy_inheritance.h` | `DistributedGraphManager` (policy hierarchy) | Tenant policy inheritance with cycle detection |

## References

- Implementation details: `../../src/governance/`
- Compliance guide: `../../src/governance/README.md`
