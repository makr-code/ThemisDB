# Governance Module — Public Headers

**Version:** 1.0
**Last Updated:** 2026-04-06
**Module Path:** `include/governance/`

---

## Purpose

This directory contains the public C++ header files (`.h`) that define the stable API for ThemisDB's governance layer. Consumers of the governance module (query engine, server layer, export pipeline, compliance tooling) include only these headers — they never depend on internal implementation details in `src/governance/` directly.

## Key Headers

| Header | Role |
|---|---|
| `policy_engine.h` | Core policy evaluation: `PolicyEngine`, `PolicyDecision`, `ClassificationProfile`, `SimulationResult`, `FieldMaskingPolicy` |
| `policy_manager.h` | Policy lifecycle management: load, validate, activate, deactivate; `PolicyRule` struct |
| `policy_manager_versioned.h` | Versioned policy management with rollback, conflict detection, and circular-dependency analysis |
| `policy_coordinator.h` | Coordination of policy decisions across distributed nodes; `PolicyCoordinator` |
| `policy_validator.h` | Syntactic and semantic policy validation; `PolicyValidator`, `ValidationReport` |
| `policy_validation.h` | Rule conflict detection (contradictory, overlapping, circular); `PolicyConflict`, `PolicyViolation` |
| `policy_template.h` | Built-in policy templates (least-privilege, GDPR, HIPAA, SOC 2, time-based, lifecycle); `PolicyTemplateManager` |
| `policy_version_history.h` | Policy change history and rollback support; `PolicyVersionHistory` |
| `policy_file_watcher.h` | inotify/FSEvents-based hot-reload of policy files; `PolicyFileWatcher` |
| `policy_review.h` | Policy review workflow (draft → review → approve → activate); `PolicyReview` |
| `review_scheduler.h` | Scheduled policy review reminders; `ReviewScheduler` |
| `compliance_reporter.h` | GDPR/HIPAA/CCPA/PCI-DSS/SOC 2 compliance reports, bias audit reports; `ComplianceReporter` |
| `compliance_reporting.h` | Report generation engine (PDF, JSON, HTML, CSV); `ReportRenderer` |
| `soc2_controls.h` | SOC 2 Trust Services Controls and evidence collection (CC6.1, CC7.2, CC8.1, A1.1, C1.1, PI1.2); `Soc2Controls` |
| `ccpa_rules.h` | CCPA/CPRA data subject rights evaluators (RightToKnow, RightToDelete, OptOutOfSale, DataPortability); `CcpaRuleSet` |
| `pci_dss_rules.h` | PCI-DSS data isolation and compliance rules; `PciDssRuleSet` |
| `data_masker.h` | Field-level data masking strategies (REDACT, TOKENIZE, TRUNCATE, HASH); `DataMasker`, `FieldMaskingPolicy` |
| `data_lineage.h` | Data lineage tracking for governed datasets; `DataLineageTracker`, `LineageEvent` |
| `cross_tenant_policy_inheritance.h` | Cross-tenant hierarchical policy composition (most-restrictive-wins, cycle detection); `CrossTenantPolicyInheritance` |
| `model_governance.h` | AI/ML model training governance, bias auditing, training data lineage; `ModelGovernancePolicy`, `BiasAuditReport` |
| `opa_adapter.h` | Open Policy Agent integration for Rego-based policy evaluation; `OpaAdapter`, `OpaAdapter::Config` |

## See Also

- [`src/governance/README.md`](../../src/governance/README.md) — Implementation code and development guide
- [`src/governance/ARCHITECTURE.md`](../../src/governance/ARCHITECTURE.md) — Architecture guide with component diagram and data flow
- [`src/governance/ROADMAP.md`](../../src/governance/ROADMAP.md) — Development roadmap and production readiness checklist
- [`src/governance/FUTURE_ENHANCEMENTS.md`](../../src/governance/FUTURE_ENHANCEMENTS.md) — Planned features with performance targets and IEEE references
- [`docs/de/governance/README.md`](../../docs/de/governance/README.md) — German secondary documentation

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

## Usage

Include the relevant headers from this module:

```cpp
#include "governance/module_header.h"
```

See [`ARCHITECTURE.md`](ARCHITECTURE.md) and [`ROADMAP.md`](ROADMAP.md) for details.
