---
Author: "ThemisDB Contributors"
Created: "2026-09-09"
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

# Governance Module Architecture

This module groups policy decisions and lifecycle management, compliance
evaluation/reporting, data-governance controls, and audit-oriented support
interfaces. The code does not, by itself, enforce every decision at downstream
operation boundaries; consuming callers must apply the returned result.

## Runtime paths

```text
Policy YAML ──> PolicyEngine ──> native decision ─────────┐
                         └──> attached evaluator (OPA) ──┴─> PolicyDecision
                                                                    │
                                             consuming caller applies gates

PolicyManager / version history / approval / change manager
                         └──> lifecycle, review, rollback result surfaces

Compliance rules/mappings ──> validators and reporters ──> structured reports

DataMasker ──> transformed JSON (only when caller invokes it)
DataLineageTracker ──> bounded in-memory records ──> optional audit forwarding
OperationalAuditLogger ──> bounded event storage / export / correlation helpers
```

The diagram represents independent module surfaces, not an automatic pipeline.
In particular, calling `PolicyEngine::checkQueryPermission()` does not itself
mask a response or execute/cancel a query. OPA REST failure can produce
`std::nullopt`, after which `PolicyEngine` uses native evaluation. The built-in
WASM stub is not policy evaluation and must not be used for authorization; see
[`TRUST.md`](TRUST.md).

## Component ownership

- **Policy engine:** `policy_engine.cpp` loads supported YAML sections, evaluates
  classification/route context, and returns a decision. Hot reload is exposed
  separately; see [`IMPLEMENTATION.md`](IMPLEMENTATION.md).
- **Policy lifecycle:** managers, version history, review/approval, conflict
  detection, and change-management types are separate interfaces. Their
  existence does not imply durable storage or atomicity across external systems.
- **Compliance:** framework mappings, rule/control evaluators, validation, and
  report generation produce software-level results from configured inputs.
  Reports are not independent certification or legal conclusions.
- **Data governance:** masking, lineage, subject-rights, cross-border transfer,
  tenant inheritance, and model-export checks have distinct APIs. The caller
  owns applying their outcomes.
- **Audit-related support:** `governance_audit_integrity.*`,
  `audit_batch_writer.*`, and `operational_audit.*` are separate implementations.
  Do not assume one shared persistent store or a durable end-to-end audit
  pipeline from their class names.

## Implementation file map

The following 35 translation units are grouped by their primary code area. A
filename grouping describes repository organization, not an automatic runtime
call graph.

| Area | Implementation files |
|---|---|
| Policy evaluation, lifecycle, and integration | `policy_engine.cpp`, `opa_adapter.cpp`, `policy_manager.cpp`, `policy_manager_versioned.cpp`, `policy_version_history.cpp`, `policy_approval_workflow.cpp`, `policy_change_manager.cpp`, `policy_conflict_detector.cpp`, `policy_coordinator.cpp`, `policy_file_watcher.cpp`, `policy_review.cpp`, `policy_template.cpp`, `policy_validation.cpp`, `policy_validator.cpp`, `review_scheduler.cpp` |
| Compliance rules, mapping, validation, and reports | `ccpa_rules.cpp`, `compliance_framework.cpp`, `compliance_mapping.cpp`, `compliance_reporter.cpp`, `compliance_reporting.cpp`, `compliance_validator.cpp`, `gdpr_subject_rights.cpp`, `hipaa_rules.cpp`, `iso27001_rules.cpp`, `pci_dss_rules.cpp`, `soc2_controls.cpp` |
| Data governance | `cross_border_transfer.cpp`, `cross_tenant_policy_inheritance.cpp`, `data_lineage.cpp`, `data_masker.cpp`, `model_governance.cpp` |
| Audit and diagnostics | `audit_batch_writer.cpp`, `governance_audit_integrity.cpp`, `governance_diagnostics.cpp`, `operational_audit.cpp` |

## Failure and storage boundaries

- YAML load failures return `false`; inspect the active configuration path and
  do not infer that a failed reload replaced policy state.
- OPA REST failures can fall back to native evaluation. This is not equivalent
  to evaluating the external policy.
- Without an injected WASM evaluator, the current built-in WASM stub can return
  a permissive observe-mode result if a bundle file exists and the real runtime
  is unavailable. Maintainer confirmation of any replacement target remains
  pending.
- `DataLineageTracker` and `OperationalAuditLogger` retain bounded in-process
  data; they are not documented here as durable stores.
- Compliance and audit result types depend on caller-provided evidence, keys,
  and storage/configuration. Durable retention, key lifecycle, and deployment
  controls must be verified outside these module declarations.

## References

- [Module overview](README.md) · [Public API](API.md)
- [Implementation/configuration](IMPLEMENTATION.md) · [Trust boundaries](TRUST.md)
- [Public-header inventory](../../include/governance/README.md)
- [Test registration](../../tests/governance/CMakeLists.txt)
- [Benchmark registration](../../benchmarks/governance/CMakeLists.txt)
