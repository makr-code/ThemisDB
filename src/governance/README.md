---
Author: "ThemisDB Contributors"
Created: "2026-10-30"
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

# ThemisDB Governance Module

## Purpose

The module contains policy evaluation and lifecycle APIs, compliance rules and
reporting, data-governance controls, audit-related utilities, and OPA integration.
Its public declarations are under `include/governance/`; implementation is under
`src/governance/`. Returned policy/control decisions require enforcement by
consuming callers.

## Implementation inventory

The current top-level source set contains 35 `.cpp` files:

`audit_batch_writer.cpp`, `ccpa_rules.cpp`, `compliance_framework.cpp`,
`compliance_mapping.cpp`, `compliance_reporter.cpp`, `compliance_reporting.cpp`,
`compliance_validator.cpp`, `cross_border_transfer.cpp`,
`cross_tenant_policy_inheritance.cpp`, `data_lineage.cpp`, `data_masker.cpp`,
`gdpr_subject_rights.cpp`, `governance_audit_integrity.cpp`,
`governance_diagnostics.cpp`, `hipaa_rules.cpp`, `iso27001_rules.cpp`,
`model_governance.cpp`, `opa_adapter.cpp`, `operational_audit.cpp`,
`pci_dss_rules.cpp`, `policy_approval_workflow.cpp`, `policy_change_manager.cpp`,
`policy_conflict_detector.cpp`, `policy_coordinator.cpp`, `policy_engine.cpp`,
`policy_file_watcher.cpp`, `policy_manager.cpp`, `policy_manager_versioned.cpp`,
`policy_review.cpp`, `policy_template.cpp`, `policy_validation.cpp`,
`policy_validator.cpp`, `policy_version_history.cpp`, `review_scheduler.cpp`,
and `soc2_controls.cpp`.

The public header inventory is maintained in
[`../../include/governance/README.md`](../../include/governance/README.md).
The list above is a file inventory, not evidence that every API is linked into
every binary or automatically used by a downstream module.

## Runtime boundaries

- `PolicyEngine` reads supported YAML sections and returns structured decisions;
  callers apply restrictions.
- OPA REST failure can fall back to native evaluation. Explicit WASM mode uses
  only an injected evaluator; without one it returns an enforced strict deny
  and does not fall back. A bundle file alone does not produce an allow.
- Masking, lineage, model export, subject-rights, transfer, tenant-inheritance,
  compliance, and audit APIs are distinct interfaces, not an automatic pipeline.
- Lineage and operational-audit records include bounded/in-memory behavior;
  durable retention must be configured and verified separately.
- Compliance reports summarize supplied policies/evidence and do not establish
  certification.

## Developer documentation

| Document | Purpose |
|---|---|
| [Architecture](ARCHITECTURE.md) | Runtime boundaries and data flow |
| [API](API.md) and [implementation](IMPLEMENTATION.md) | Public contracts and configuration behavior |
| [Module guide](MODULE_GUIDE.md) | Test registration and change workflow |
| [Runbook](RUNBOOK.md) and [operations](OPERATIONS.md) | Diagnosis and operational considerations |
| [Security](SECURITY.md) and [trust](TRUST.md) | Security limits and trust assumptions |
| [Performance expectations](PERFORMANCE_EXPECTATIONS.md) | Benchmark source mapping and limits |

Focused tests are defined by
[`../../tests/governance/CMakeLists.txt`](../../tests/governance/CMakeLists.txt);
two existing suites are currently skipped in its module-focused registration.
Optional benchmark targets are described in
[`../../benchmarks/governance/README.md`](../../benchmarks/governance/README.md).

## Source references

- Public interface index: [`../../include/governance/README.md`](../../include/governance/README.md)
- Implementation sources: this directory
- [Canonical module roadmap](ROADMAP.md) · [Historical changelog](CHANGELOG.md)
