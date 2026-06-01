> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/governance/ARCHITECTURE.md -->

# Governance Module — Public Header Architecture

**Module Path:** `include/governance/`  
**Implementation:** `../../src/governance/`  
**Canonical architecture doc:** [`../../src/governance/ARCHITECTURE.md`](../../src/governance/ARCHITECTURE.md)

---

## 1. Overview

`include/governance/` defines the **public data governance, regulatory compliance (GDPR, HIPAA, CCPA, PCI-DSS, SOC2, ISO27001), policy engine, data lineage, model governance, and OPA integration API contract** for ThemisDB.

For runtime composition and implementation internals see:
→ [`../../src/governance/ARCHITECTURE.md`](../../src/governance/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Policy Engine

| Header | Public Type | Purpose |
|--------|------------|---------|
| `policy_engine.h` | `PolicyEngine` | Core policy evaluation engine |
| `policy_manager.h` | `PolicyManager` | Policy lifecycle management |
| `policy_manager_versioned.h` | `PolicyManagerVersioned` | Versioned policy management |
| `policy_coordinator.h` | `PolicyCoordinator` | Cross-tenant policy coordination |
| `policy_file_watcher.h` | `PolicyFileWatcher` | Hot-reload watcher for policy files |
| `policy_template.h` | `PolicyTemplate` | Policy template library |
| `policy_validation.h` | `PolicyValidation` | Policy syntax and semantic validation |
| `policy_validator.h` | `PolicyValidator` | Policy evaluation validator |
| `policy_version_history.h` | `PolicyVersionHistory` | Audit trail for policy changes |
| `policy_review.h` | `PolicyReview` | Workflow for policy review and approval |
| `review_scheduler.h` | `ReviewScheduler` | Scheduled policy review triggers |
| `opa_adapter.h` | `OPAAdapter` | Open Policy Agent integration adapter |
| `cross_tenant_policy_inheritance.h` | `CrossTenantPolicyInheritance` | Policy inheritance across tenant boundaries |
### 2.2 Regulatory Compliance

| Header | Public Type | Purpose |
|--------|------------|---------|
| `gdpr_subject_rights.h` | `GDPRSubjectRights` | GDPR data-subject rights (erasure, access, portability) |
| `hipaa_rules.h` | `HIPAARules` | HIPAA safeguard rule enforcement |
| `ccpa_rules.h` | `CCPARules` | CCPA consumer rights enforcement |
| `pci_dss_rules.h` | `PCIDSSRules` | PCI-DSS cardholder data controls |
| `soc2_controls.h` | `SOC2Controls` | SOC 2 Trust Services Criteria controls |
| `iso27001_rules.h` | `ISO27001Rules` | ISO 27001 information-security controls |
| `cross_border_transfer.h` | `CrossBorderTransfer` | Cross-border data transfer compliance checks |
| `compliance_reporter.h` | `ComplianceReporter` | Compliance evidence collection and reporting |
| `compliance_reporting.h` | `ComplianceReporting` | Structured compliance report generation |
### 2.3 Data Lineage and Masking

| Header | Public Type | Purpose |
|--------|------------|---------|
| `data_lineage.h` | `DataLineage` | Column/row-level data lineage tracking |
| `data_masker.h` | `DataMasker` | Dynamic data masking for policy-controlled access |
| `model_governance.h` | `ModelGovernance` | ML model governance and audit trails |

---

## 3. Namespace Layout

All public types reside in the `themis::governance` namespace (or a sub-namespace).

---

## 4. Contract Notes

- Headers in `include/governance/` expose the **stable public API**; internal types live in `src/governance/`.
- Clients depend only on types declared here; implementation details in `src/` may change without notice.
- For breaking-change policy see [`../../VERSIONING.md`](../../VERSIONING.md).
- Layer association: **Graph**.
