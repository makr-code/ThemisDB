---
Author: "ThemisDB Contributors"
Created: "2026-06-01"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Related: "Issue #7349"
---

Exact model backend not verifiable from final document text alone.

# Governance Public-Header Architecture

This page describes the organization of declarations under `include/governance/`.
It is not a module dependency graph. Runtime flows and ownership boundaries are
summarized in [`../../src/governance/ARCHITECTURE.md`](../../src/governance/ARCHITECTURE.md).

## Public header groups

| Group | Headers / representative declared types |
|---|---|
| Policy decisions and management | `policy_engine.h` (`PolicyEngine`, `PolicyDecision`); `policy_manager.h` (`PolicyManager`, `PolicyRule`, `PolicyState`); `policy_manager_versioned.h` (`PolicyManagerWithVersioning`); `policy_conflict_detector.h` (`PolicyConflictDetector`); `policy_version_history.h` (`PolicyVersionHistory`); `policy_change_manager.h` (`PolicyChangeManager`); `policy_approval_workflow.h` (`PolicyApprovalWorkflow`); `policy_coordinator.h`; `policy_file_watcher.h`; `policy_template.h`; `policy_validation.h`; `policy_validator.h`; `policy_review.h`; `review_scheduler.h` |
| Compliance | `compliance_framework.h` (`ComplianceFrameworkRegistry`, `ComplianceRequirement`, `ComplianceControl`); `compliance_mapping.h`; `compliance_validator.h`; `compliance_reporter.h`; `compliance_reporting.h`; `ccpa_rules.h` (`CcpaRuleSet`); `gdpr_subject_rights.h` (`GdprSubjectRightsManager`); `hipaa_rules.h` (`HipaaRuleSet`); `iso27001_rules.h` (`Iso27001ControlSet`); `pci_dss_rules.h` (`PciDssRuleSet`); `soc2_controls.h` (`Soc2ControlSet`); `cross_border_transfer.h` (`CrossBorderTransferPolicy`) |
| Data governance and evaluator integration | `data_masker.h` (`DataMasker`); `data_lineage.h` (`DataLineageTracker`); `cross_tenant_policy_inheritance.h` (`CrossTenantPolicyInheritance`); `model_governance.h` (`ModelGovernancePolicy`); `opa_adapter.h` (`OpaAdapter`) |
| Audit, diagnostics, and contract types | `governance_audit_integrity.h` (`AuditSigner`, `AuditIntegrityManager`, `AuditTamperDetector`, `AuditRetentionManager`); `audit_batch_writer.h` (`AuditBatchWriter`); `operational_audit.h` (`OperationalAuditLogger`, `EventCorrelationEngine`, `ComplianceEvidenceCollector`); `governance_diagnostics.h`; `governance_api_contract.h` |

The inventory is based on the current header declarations. Type spellings are
case-sensitive; older names such as `GDPRSubjectRights`, `OPAAdapter`,
`ComplianceReporting`, or `DataLineage` are not the declared class names in the
referenced headers.

## Contract and limits

- The headers expose caller-facing C++ interfaces; this page does not certify
  ABI stability or promise that every declared surface has production evidence.
- The policy engine returns structured decisions; downstream callers are
  responsible for applying decision fields.
- The operational-audit and lineage surfaces include bounded/in-memory state;
  consult their headers and implementations before assuming durable retention.
- `OpaAdapter` supports REST and a WASM-related API hook, but its built-in WASM
  fallback is not real policy evaluation. See
  [`TRUST.md`](../../src/governance/TRUST.md).

## References

- [Complete public-header inventory](README.md)
- [Governance public API summary](../../src/governance/API.md)
- [Implementation and configuration behavior](../../src/governance/IMPLEMENTATION.md)
- [Module runtime architecture](../../src/governance/ARCHITECTURE.md)
